# REVIEW.md — Code vs. Paper Review

**Paper.** L. Mex, C. A. Cruz-Villar, F. Peñuñuri, *Closed-Form Solutions to Differential
Equations via Differential Evolution*, Discrete Dynamics in Nature and Society, 2015,
DOI [10.1155/2015/910316](https://doi.org/10.1155/2015/910316). Equations (4), (7)–(9),
(14)–(15), Table 1 and Appendix A are cited below by their paper numbering.

**Code state.** All line numbers in this document refer to commit `bc70e95` (`master`
before the fixes planned in issues #2, #5, #6 and #7). If the code moves, re-verify.

**Method.** Symbols and call paths were explored with codegraph; behavioral claims were
verified with throwaway C++ probes compiled against this exact revision (public functions
only: `parsv`, `narg`, `OperF2`, `pow2dual`, `tan2dual`, `x_rango`, `Mutation`,
`Recombination`, `FunbObj::fitness`). Each finding below states its evidence; the probes
are reproductions, not committed tests (see section 7).

**Sections.** 1. Executive Summary · 2. General Classification · 3. Critical Bugs ·
4. Algorithmic Deviations · 5. Configuration Deviations · 6. Strengths ·
7. Prioritized Correction Plan · 8. Proposed A/B Experiment.

---

## 1. Executive Summary

The code is a **working prototype inspired by the paper, not a faithful implementation
of it**. It evolves expressions for one fixed toy ODE (`y'' + y = 0`, `y(0) = y'(0) = 1`,
solution `cos(x) + sin(x)`) and it does find that solution (`fitness(cos(x)+sin(x)) == 0`
exactly), but it cannot be used to reproduce the paper's results: the DE operators,
the objective functional constants, the evaluation grid, the individual initialisation
and even the grammar numbering all differ from the paper specification.

Four findings deserve immediate attention:

1. **New critical bug (not covered by any open issue): the parse-tree evaluator returns
   `NaN` for perfectly valid expressions** whose binary operator has a leaf operand —
   `x*x`, `sin(x)+x`, `x/2`, `x^2`, `2^x` all evaluate to `NaN` (section 3, B1). The root
   cause is that the C++ port dropped the operand-consumption semantics of the reference
   Fortran `parsv` (Appendix A): consumed operands must be blanked to `NaN` in place.
   It also silently mis-binds operands in unary chains (`sin(cos(x))+2` becomes
   `sin(cos(x))+x`). This invalidates a large part of the search space.
2. **New high-severity bug: out-of-bounds read in `Mutation()`** (section 3, B5):
   one seed individual has 21 tokens while the rest have 20; the loop reads `y[20]`/`z[20]`.
   AddressSanitizer under the project's own `-O3` flags reports a heap-buffer-overflow at
   `parse_tree.cpp:183`; bounds-checked builds abort.
3. The three bugs already tracked in issue #2 are confirmed exactly as described
   (`||` in `parsv`, token 12 arity/semantics, `tan2dual`) (section 3, B2–B4).
4. The second-order dual-number formulas are **mathematically correct** for
   `sin`, `cos`, `exp`, `log`, product, quotient, `sqrt` and `pow` with a positive base:
   16/16 probe checks pass against hand-computed values and central finite differences
   (section 6, S1). The only numeric weak spot is `pow2dual()` at `x0 = 0` and at
   negative bases with fractional exponents (section 3, B8).

Overall classification and the full deviation inventory are in sections 2, 4 and 5;
section 7 maps every finding to a correction phase and to the existing issues, and
section 8 specifies the before/after experiment (issue #4), including its prerequisite:
the current `srand(time(NULL))` makes runs unreproducible, so a fixed seed must be
added before any A/B comparison.

## 2. General Classification

**Verdict: a project inspired by the paper — not a port, and not a reproduction of any
of its results.** The conceptual architecture is shared (integer-grammar → parse tree →
second-order dual numbers → DE/rand/1/bin loop), but every stage has independent
deviations:

| Stage | Paper | This code | Deviation |
|-------|-------|-----------|-----------|
| Problem | 10 case studies (paper §5); Case 1 is `y'' + y'/x − cos(x)/x = 0` | one fixed toy ODE `y'' + y = 0`, `y(0)=y'(0)=1` | different problem |
| Grammar | Table 1: 29 symbols, `12='+'`, `13='*'`, `14='/'`, `15=pow`, `16='-()'`, `17=sin`, … `29=tanh` | 21 symbols, own numbering (`18='+'`, `13=sin`, `12=negation`, …) | different alphabet (section 4, D7) |
| Encoding | prefix integer vector (`x`, eq. (5)) | prefix `vector<long double>` of token codes | equivalent idea |
| Derivatives | second-order dual numbers, eq. (14)–(15) | 2dual `[f, f', f'']`, same composition rule | correct (S1) |
| Parser | `parsv` + `filterx`; operand consumption, valid-subset fallback (Appendix A) | `parsv` + `parse_tree` string check; no `filterx` | porting defects (B1) |
| DE operator | DE/rand/1/bin with dither, `F∈(0,1)` random (eq. (7)–(9)) | ad-hoc integer token arithmetic | different algorithm (D1–D5) |
| Selection | greedy trial-vs-target | random replacement (no comparison) | different algorithm (D3) |
| Populations | one | two, cross-pollinating | different (D4) |
| Fitness | `λ1h1² + λ2h2² + ∫f²`, 100 points, `λ1=λ2=10`, `Cr=0.2` | `Σ(y''+y)⁴ + 300(y(0)−1)⁴ + 700(y'(0)−1)⁴`, 11 points | different constants (C1–C4) |

The SageMath notebook `Untitled.ipynb` is a stale scratchpad for a *third* problem
(`y'' + 2y = 0`, solution `√2/2·sin(√2x) + cos(√2x)`); its fitness formula does not match
the C++ code (AGENTS.md already warns about this). The C++ is the reference for the
current behavior.

## 3. Critical Bugs

### B1 — `OperF2`/`OperF` do not consume operands; valid expressions evaluate to `NaN`

**Where.** `dualfunctions.cpp:273-286` (scan/blank loop inside `OperF2`, blanking at
`dualfunctions.cpp:281`), unary path `dualfunctions.cpp:402-417`, binary call
`dualfunctions.cpp:426-447`.

**What.** The reference parser (Appendix A, Algorithm 1) evaluates right-to-left and
**clears each consumed operand in place** (`operf`/`operf2` "change the real components
of `xval`/`vecval` to NaN"; the Fortran passes `auxv(:,k+1)` / `auxv(:,k+1:length)` by
reference). The C++ port passes **copies** (`xuax` at `dualfunctions.cpp:408-411`; the
`auxvtemp1..3` copies at `dualfunctions.cpp:429-437`), never clears `auxv(k+1)` after a
unary evaluation, and inside `OperF2` it blanks the component **after** the one it just
read (`vecval[0][j+1]=NANf` at `dualfunctions.cpp:281`) while the scan stops one short of
the last component (`j < dim - 1` at `dualfunctions.cpp:273`). The C++ also omits the
reference's early exit on `NaN` results (Algorithm 1).

**Evidence (probe, master).** Expected values are closed-form; observed are from the
code:

| Call | Expected | Observed |
|------|----------|----------|
| `parsv([19,11,11], [2,1,0])` = `x*x` | `[4, 4, 2]` | `[nan, nan, nan]` |
| `parsv([18,13,11,11], [2,1,0])` = `sin(x)+x` | `[2.9093, 0.5839, -0.9093]` | `[nan, nan, nan]` |
| `parsv([20,11,2], [2,1,0])` = `x/2` | `[1, 0.5, 0]` | `[nan, nan, nan]` |
| `parsv([21,2,11], [2,1,0])` = `2^x` | `[4, 2.7726, 1.9218]` | `[nan, nan, nan]` |
| `OperF2(19, [[2,2],[1,1],[0,0]])` | `[4, 4, 2]` | `[nan, nan, nan]` |
| `parsv([18,13,14,11,2], [0.3,1,0])` = `sin(cos(x))+2` | `[2.8165, …]` | `[1.1165, …]` (= `sin(cos(x))+x`, silently wrong) |
| `parsv([18,13,11,14,11], [1,1,0])` = `sin(x)+cos(x)` | matches | matches (control) |

**Impact.** Any expression in which a binary operator receives a leaf (constant or `x`)
as an operand evaluates to `NaN`; `fitness()` then either penalises it (finite `1e6` per
point when all three components are `NaN`, `parse_tree.cpp:92-101`) or returns `INFINITY`.
The search mostly explores a distorted fitness landscape dominated by this artefact.

**Fix.** Restore the reference semantics: evaluate in place so that consumed components
are blanked (`auxv[0][k+1] = NAN` after unary use; both operands after binary use), scan
the full sub-vector in `OperF2` (blank the component that was consumed, `j`, not `j+1`),
and return immediately when an operation yields `NaN`. **Not covered by issue #2 — needs
a new issue.**

### B2 — `||` instead of `&&` in the first loop of `parsv`

**Where.** `dualfunctions.cpp:382` (`if(x[k] >= 1 || x[k] <= 10){`). The condition is true
for every number, so **all** tokens start with their code as a numeric value.

**Evidence.** `parsv([13,18], [0.5,1,0])` — an invalid tree — returns
`[-0.7510, 0, 0]` (= `sin(18.0)`), while the reference returns `NaN` (probe, master).
The mask hides invalid trees and feeds operator codes as operands into evaluations.

**Fix.** `&&` (issue #2). `fitness()`'s `parse_tree(...).find("()")` check
(`parse_tree.cpp:56-63`) does not replace this: `[13,18]` renders as a valid-looking
`sin(18.0)`.

### B3 — Token 12 (negation): arity and semantics

**Where.** `narg(12) = 0` (`dualfunctions.cpp:242`) while `OperF` implements negation as a
unary function (`dualfunctions.cpp:347-351`) and `parse_tree` renders it as `-1.0*()`
(`dualfunctions.cpp:198-199`). Additionally `parsv` initialises token 12 by negating the
**input** `xval` instead of the argument subtree (`dualfunctions.cpp:393-397`).

**Evidence.** `parsv([12,13,11], [1,1,0])` (i.e. `-sin(x)` at `x=1`) returns
`[-1, -1, 0]` instead of `[-0.8415, -0.5403, 0.8415]` (probe, master). Because
`narg(12) == 0`, the second loop never calls `OperF` for token 12.

**Fix.** `narg(12) = 1`, delete the special case at `dualfunctions.cpp:393-397`, let the
unary path evaluate it (issue #2).

### B4 — `tan2dual()` first-derivative formula

**Where.** `dualfunctions.cpp:41`: `f[1] = xval[0]/(pow(cos(xval[0]),2));` — uses the
value `x` instead of its derivative `xval[1]`.

**Evidence.** `tan2dual([2,1,0])` returns `f[1] = 11.5488 = 2/cos²(2)`; the correct value
is `5.7744 = 1/cos²(2)` (probe, master). `f[2]` and `f[0]` are correct.

**Impact.** Currently **dead code**: the C++ grammar has no `tan` token (`OperF`,
`dualfunctions.cpp:334-359`, never dispatches to it), so it is only reachable by direct
call. It is nevertheless a latent bug and the paper's grammar (Table 1, token 22) does
include `tan`. **Fix** (issue #2) and consider adding the missing functions or deleting
the dead code.

### B5 — Out-of-bounds read in `Mutation()` (memory safety)

**Where.** `parse_tree.cpp:183` (`abs(x[i]-y[i])`) and `parse_tree.cpp:188` (`z[i]`),
with `i < x.size()`. One seed individual has **21** tokens (`dos`,
`parse_tree.cpp:152`) while all others have 20 (`parse_tree.cpp:151-155,170-174`). That
individual is `Population2[1]`, which is the `x` operand of the first `Mutation()` call
of generation 1 (`parse_tree.cpp:308`), with `y`,`z` of size 20. The same happens for
`Population1[1]` (`parse_tree.cpp:309`).

**Evidence.** Probe reproducing the real call pattern: with the project's own flags
(`-O3`) plus AddressSanitizer, the program reports
`ERROR: AddressSanitizer: heap-buffer-overflow ... READ of size 16` at
`parse_tree.cpp:183`, 0 bytes after a 320-byte (20 × `long double`) allocation; a
bounds-checked build (`-O0` on this system) aborts with the libstdc++ assertion
`__n < this->size()`. Under the committed `-O3` flags without sanitizers it is a silent
out-of-bounds read (undefined behaviour) that currently does not crash.

**Fix.** Use uniform individual lengths (fix `dos`, or enforce/resize in
`createPopulation`), and bound the loop by `min(x.size(), y.size(), z.size())`.
**Not covered by any open issue — needs a new issue.**

### B6 — `fitness()` is non-const and mutates the individual

**Where.** `parse_tree.cpp:64-75` writes `11` into `_vecval` when no `x` token is present,
inside `fitness()`; `compare` (`parse_tree.cpp:252-254`) calls `fitness()` and is used as
the `std::sort` comparator (`parse_tree.cpp:336-337,344-345,352,357`).

**Impact.** `fitness()` is recomputed on every comparison (expensive) and can mutate the
objects being sorted, which can break `std::sort`'s strict-weak-ordering requirement
(undefined behaviour risk) and makes the "no free `x`" repair depend on evaluation
order. Severity: medium.

### B7 — Inconsistent penalty for `NaN` evaluations

**Where.** `parse_tree.cpp:92-101`: the `||`-condition is almost always true; when it is
false (all three components `NaN`) the point contributes a finite `1e6`; when only some
components are `NaN`, `pow(f2+f0,4)` propagates `NaN` into `res` and the whole individual
becomes `INFINITY` at `parse_tree.cpp:107-110`. Two different penalties for the same
"nothing useful was computed" situation. Severity: low.

### B8 — Numeric weakness in `pow2dual()` (verified)

**Where.** `dualfunctions.cpp:80-120`; `eps` at `dualfunctions.cpp:92`; the three
`if/else` blocks at `dualfunctions.cpp:99-104`, `106-111`, `112-117`.

This is the weakness flagged in the parent spec; the review verified its concrete
failure modes:

* **The complex machinery is mostly inert.** `z1 = pow(x[0], y[0])` at
  `dualfunctions.cpp:93` receives two `long double`s and therefore resolves to the *real*
  `std::pow` overload, not `std::pow(complex, complex)`. `(-2)^3` returns the correct
  real answer `[-8, 12, -12]`, but `(-8)^(1/3)` and `(-4)^(0.5)` return `NaN` (observed),
  i.e. the "complex number handling" never produces a value for negative bases — the
  complex `log(xz)` at `dualfunctions.cpp:95,97` is the only place complex arithmetic
  enters, and it only propagates `NaN`.
* **`x0 = 0` breaks the derivatives even when they exist.** `pow2dual([0,1,0],[2,0,0])`
  returns `[0, nan, nan]` although `d/dx x²` and `d²/dx² x²` are `0` and `2`. The grid
  includes `x = 0` (`parse_tree.cpp:25,42,44`), so `x^2`-like expressions are penalised at
  that point.
* **The `eps`/`if/else` block is dead code.** Both branches assign `real(z)`
  (`dualfunctions.cpp:99-117`), so the tolerance `1e-6` changes nothing.

**Status.** Verified numerically (probe). It requires a policy decision plus tests:
either implement a real-branch power for real inputs (with explicit `NaN` for genuinely
undefined cases, guarding `x0 = 0`), or document `NaN` as intended and test it.

## 4. Algorithmic Deviations

Each row gives the paper specification and the actual code.

| # | Topic | Paper | Code | Where |
|---|-------|-------|------|-------|
| D1 | Mutation (eq. (7)) | `v = x_r0 + F·(x_r1 − x_r2)`, distinct random indices, `F∈(0,1)` **random each generation** (dither) | integer token arithmetic: `\|x−y\|`, then `+z` with `%21` wrap; no `F`; `rand()` at `dualfunctions`-free `parse_tree.cpp:188` is *not* scaled (the wrapped branch is effectively always taken); call sites pass fixed offsets `j+2`,`j+3`, not distinct random individuals | `parse_tree.cpp:179-208`, call sites `parse_tree.cpp:308-309` |
| D2 | Crossover (eq. (8)) | per-component binomial with probability `Cr` and one forced index `j_rand` | swaps a random number of tokens (`num = rand()%(size/2+2)`) among the first `size/2+2` positions only; not per-component; forces an `x` token afterwards | `parse_tree.cpp:210-250` |
| D3 | Selection (eq. (9)) | greedy: keep the trial `u` iff `f(u) ≤ f(x_i)` | no trial-vs-target comparison: offspring replace *random* individuals (`ix`,`iy`) with no fitness test; an ad-hoc "best-so-far" re-injection block does the actual elitism | `parse_tree.cpp:305-317`, `335-359` |
| D4 | Populations | one population `P` (eq. (5)) | two arrays (`Population1`,`Population2`) that mutate each other's members and share best vectors | `parse_tree.cpp:263`, `288`, `308-315`, `349-357` |
| D5 | Dither | `F` random in `(0,1)` (paper §2.1, last paragraph) | no `F` anywhere | whole file |
| D6 | Initialisation (eq. (6)) | uniform real vectors in `[b_L,b_U]`, integer part taken (paper §3) | `rand()%21+1` token soup plus **5 hand-written seed expressions** (one of them 21 tokens long, see B5), not uniformly random | `parse_tree.cpp:117-145`, `148-176` |
| D7 | Grammar | Table 1: 29 symbols — constants `1..10`, `x=11`, `+=12`, `*=13`, `/=14`, `pow=15`, `-()=16`, `sin=17`, `cos=18`, `exp=19`, `log=20`, `sqrt=21`, `tan=22`, `arcsin=23`, `arccos=24`, `arctan=25`, `erf=26`, `sinh=27`, `cosh=28`, `tanh=29` | 21 symbols with different numbers — `x=11`, `negation=12`, `sin=13`, `cos=14`, `exp=15`, `log=16`, `sqrt=17`, `+=18`, `*=19`, `/=20`, `^=21`; the 8 functions `tan…tanh` are missing (paper Table 1; `README.md` token table; `dualfunctions.cpp:163-234,237-246,334-359`; `parse_tree.cpp:117-145`) | as noted |
| D8 | Parser contract | `parsv` falls back to a **valid subset** for malformed vectors and `filterx` extracts it (Appendix A); it aborts on `NaN` results | returns the value at position 0 only; no `filterx` (grep: absent); malformed trees are rejected via the rendered string in `fitness()` instead | `parse_tree.cpp:56-63`; Appendix A |
| D9 | `Cr` | `Cr = 0.2` (probability, paper §5) | `int Cr = 85` compared against `rand() % Population2.size()` (a population-size range, ≈31% of generations with 276 individuals), and a commented-out block using `rand()%100+1` shows the intended semantics | `parse_tree.cpp:290`, `436-438`, `463` |

Discussion. D1–D3 mean the search is **not** DE/rand/1/bin at all: there is no `F`, no
per-component crossover, no greedy selection, so the population dynamics are ad-hoc.
D4–D5 add two undocumented inventions (second population, cross-pollination). D6's fixed
seeds are not random initialisation. D7 is the reason the paper's grammar examples (its
Table 2 chromosome `12,17,11,18,11` = `sin(x)+cos(x)`) translate to different vectors
here (`18,13,11,14,11`). D8 matters because the paper *relies* on the subset behaviour to
keep malformed individuals evaluable; the code's substitute (string-based rejection plus
the `||` bug B2) behaves differently. D9 makes the crossover rate neither 0.2 nor a
per-generation probability.

## 5. Configuration Deviations

| # | Topic | Paper | Code | Where |
|---|-------|-------|------|-------|
| C1 | Evaluation points | 100 equally spaced points on `[0,1]` (paper §5) | `x_rango(0,1,0.1)` gives **11** points `0.0..1.0` (verified). `int final = stop/step` truncates the floating-point quotient (1.0/0.1 ≈ 9.999…); for exactly representable steps (`0.25`) the `+2` overshoots the interval instead (6 points up to `1.25`, verified) | `parse_tree.cpp:19-29` (loop `:24`), constructor `parse_tree.cpp:42` |
| C2 | Boundary weights | `λ1 = λ2 = 10` (paper §5) | `300` and `700` | `parse_tree.cpp:103` |
| C3 | Residual powers | squared terms (eq. (4)) | fourth powers for both the ODE residual and the boundary residuals | `parse_tree.cpp:95`, `103` |
| C4 | Integral | `∫ f² dx` over `[x1,x2]` (eq. (4)) | plain sum over the grid, no `Δx` factor | `parse_tree.cpp:79-102` |
| C5 | Population size | Case 1: 100 individuals (80–500 across cases) | `tam = 275`, but `createPopulation` actually creates **276** (`5` seeds + `tam−4` random; off-by-one) | `parse_tree.cpp:262`, `148-176` (loop `:170`) |
| C6 | Dimension | 20 for Case 1, 15–30 across cases | fixed 20 tokens per individual | `parse_tree.cpp:172` |
| C7 | Generations | Case 1: 5 000 (100–70 000 across cases) | cap `100000` | `parse_tree.cpp:490-493` |
| C8 | Stopping criterion | `R = 0` (eq. (4)) | `eps = 1e-5`, same spirit; absolute values are not comparable because of C2–C4 | `parse_tree.cpp:291`, `480` |
| C9 | Seeding | — | `srand(time(NULL))`: runs are not reproducible and no seed can be supplied; this blocks the A/B experiment (section 8) | `parse_tree.cpp:261` |
| C10 | Problem instance | ten case studies | one toy ODE not present in the paper (closest in shape: none of the 10 cases match `y''+y=0`) | `fitness()` (`parse_tree.cpp:95,103`), `README.md` |

> Status (issue #28): C5–C7 are fixed — population size, individual dimension
> and generation cap are configurable (`ODE_POP`, `ODE_DIM`, `ODE_GEN`) and
> default to the Case 1 values (100 individuals, 20 tokens, 5 000 generations).
> D6 is addressed by keeping the five hand-written expressions as an opt-in
> warm start (`ODE_WARM_START=1`) on top of the uniform random initialisation.
> The code references above describe the reviewed (pre-fix) revision.

> Status (issue #27): C2–C4 are fixed — `fitness()` now evaluates the paper's
> functional (eq. (4)): squared residuals and initial-condition penalties,
> `lambda1 = lambda2 = 10` and the `dx` integration factor over the 100-point
> grid. Independently computed expected values are pinned in `test_grid.cpp`.

Additional details worth knowing:

* The grid point `[0,1,0]` is equivalent to the paper's dual variable `x̃ = {x,1,0}`
  (eq. (14)) — correct (S2) — and the "replace first point with initials" line
  (`parse_tree.cpp:44`) is a no-op when `start = 0` because `x_rango` already produces
  `[0,1,0]`.
* `fitness()` computes `b_end = parsv(..., last point)` (`parse_tree.cpp:77`) but never
  uses it (dead code).
* Every individual is 20 tokens *except* the `dos` seed (21) — see B5.

## 6. Strengths

* **S1 — Dual-number formulas are mathematically correct (verified).** Instantiating the
  paper's composition rule, eq. (15) (`y0(u0)`, `y1(u0)·u1`,
  `y2(u0)·u1² + y1(u0)·u2`), the implementation of `sin2dual` (`dualfunctions.cpp:14-23`),
  `cos2dual` (`:25-34`), `exp2dual` (`:47-56`), `log2dual` (`:58-67`),
  `division2dual` (`:69-78`), `prod2dual` (`:134-143`) and `sqrt2dual` (`:123-132`) is
  correct. Evidence: 16/16 probe checks pass — canonical literals computed by hand
  (`sin2dual([0,1,0]) = [0,1,0]`, `cos2dual([0,1,0]) = [1,0,-1]`, `prod2dual([2,1,0],
  [3,1,0]) = [6,5,2]`, `sqrt2dual([4,1,0]) = [2,1/4,-1/32]`, `pow2dual([2,1,0], [3,0,0])
  = [8,12,12]`) plus arbitrary-`xval` chains checked against central finite differences
  (`sin`, `cos`, `exp`, `log`, product, quotient, `sqrt`, `pow` with positive base). The
  only exceptions are the singular cases in B8.
* **S2 — Dual input convention matches the paper.** Grid points are `[x,1,0]`
  (`parse_tree.cpp:25`), and `x̃ = {x,1,0}` is eq. (14) of the paper.
* **S3 — The token alphabet is internally consistent.** The five places that decode it
  (`parse_tree`, `narg`, `OperF`, `OperF2`, `vector_population`) agree on the same 21
  symbols, and `parse_tree()` renders readable expressions (verified: every seed renders,
  e.g. `(sin((x)**(1/2)))+(cos(cos(x)))`). It is nevertheless not the paper's alphabet
  (D7).
* **S4 — The end-to-end pipeline works for its target problem.** `fitness(cos(x)+sin(x))
  == 0` exactly (probe), and the committed transcript `out` shows convergence to
  `(sin(x))+(cos(x))`. The `fitness(x)` sanity value is finite (`302.53`), consistent
  with the formula.
* **S5 — Fitness structure follows the paper's idea**: sum of the ODE residual over the
  grid plus weighted boundary-condition penalties, with `INFINITY` for unparseable trees
  or trees without `x` (`parse_tree.cpp:56-75,107-110`) — only the constants and the
  missing `Δx` differ (C2–C4).

## 7. Prioritized Correction Plan

**P0 — correctness of the evaluator (blocks any meaningful search; do first).**

1. **B1 — operand consumption in `parsv`/`OperF`/`OperF2`** (new issue). Implement the
   Appendix A semantics: consume/blank operands in place, scan the whole operand
   sub-vector, blank the *consumed* component, propagate the reference's return-on-`NaN`.
   Acceptance: the probe table in B1 flips to the expected values, controls still pass.
2. **B2, B3, B4 — issue #2** (`||` → `&&`, `narg(12)=1` plus removal of the special case,
   `tan2dual` fix). Already scoped with tests in `test_dual.cpp`.

**P1 — robustness and hygiene.**

3. **B5 — out-of-bounds read in `Mutation`** (new issue): make all individuals the same
   length (fix the 21-token seed or enforce in `createPopulation`) and bound the loop by
   the three sizes. Regression: ASan run of a few generations must be clean.
4. **B6 — fitness side effects**: cache fitness per individual per generation, make the
   comparator compare cached values, and move the "no `x`" repair out of `fitness()`.
5. **B7 — unify the `NaN`/penalty path** (one penalty per unevaluable point).

**P2 — paper alignment (algorithm and configuration).**

6. **D1–D5 — rewrite the DE loop** (issues #5, #6): `v = x_r0 + F·(x_r1 − x_r2)` with
   `F∈(0,1)` per generation (dither), per-component binomial crossover with `Cr` and
   `j_rand`, greedy selection, one population. Add the integer-part step after
   recombination (paper §3).
7. **D9 — `Cr` as a probability** (`0.2`, issue #6), and remove the population-size left
   over at `parse_tree.cpp:436-438`.
8. **C1 — grid** (issue #7): explicit 100 equally spaced points, no truncation quirk.
9. **C2–C4 — functional constants** per eq. (4): `λ1=λ2=10`, squared residuals and the
   `Δx` factor (or document the deliberate change).
10. **C5–C7 — make population size, dimension and generation cap configurable** and set
    defaults aligned with a chosen paper case (Case 1: 100 individuals, dimension 20,
    5 000 generations).
11. **D6 — random initialisation** per eq. (6) (keep the seeds only as an optional
    warm start).
12. **C9 — seed parameterisation** (`argv`/env; keep `time(NULL)` as default). Required by
    section 8.

**P3 — optional / documentation.**

13. **D7 — grammar numbering**: either renumber to the paper's Table 1 and add the
    missing functions, or keep the divergence and document it (the current mismatch
    makes paper examples non-transferable).
14. **D8 — subset semantics/`filterx`**: implement the reference fallback or document the
    deliberate difference.
15. **B8 — `pow2dual` policy** and tests (guard `x0=0`; decide real-vs-NaN for negative
    bases). **B4's dead `tan2dual`**: delete or wire it once the grammar grows.
16. **B7/C4 dead code**: remove the unused `b_end` computation.

Out of scope (already agreed in the parent spec): performance work and implementing the
paper's other nine case studies.

## 8. Proposed A/B Experiment

**Question.** Do the P0/P1 fixes change the search's behavior and reliability, or do the
existing results already come from the intended dynamics? (This is issue #4.)

**Prerequisite (C9).** Add a fixed-seed entry point first (`srand(<seed>)`), otherwise no
two runs are comparable. Do not mix this change into the arms; add it to `master` before
branching the arms, or build both arms with the same seed patch.

**Arms.**

* **Arm A (before):** `bc70e95` as is.
* **Arm B (after bugs):** P0 + P1 applied (B1–B6), everything else unchanged.
* **Arm C (optional, paper alignment):** P2 as well (once #5–#7 land).

**Protocol.**

1. For each arm and for seeds `0..N−1` (suggest `N = 30`), run the search with a fixed
   population size, dimension and generation cap (suggest cap `G = 2000`; the P2 arms
   may need the paper's 5 000). Record per run: generations to first `fitness < 1e-5`,
   final best fitness, best expression, wall time.
2. Additionally run a **fixed expression battery** through `parsv` in both arms and log
   `[f, f', f'']` for: `x`, `x*x`, `sin(x)+x`, `x/2`, `2/x`, `x^2`, `2^x`,
   `sin(cos(x))+2`, `-sin(x)`, `sin(x)+cos(x)`, `sqrt(x)`, `log(x)`, `x^3`. This
   isolates B1/B2/B3 from the stochastic search and gives a direct
   "valid-evaluation rate" metric (fraction of battery entries that are finite and
   correct).
3. Run each arm once more under AddressSanitizer with a short cap to catch B5-class
   memory errors (expect Arm A to fail, Arm B to pass).

**Metrics.** Convergence rate (runs reaching the threshold), median generations,
median final fitness, valid-evaluation rate of the battery, and whether Arm A ever
re-finds `cos(x)+sin(x)` within `G`. Preserve raw outputs alongside the report.

**Expected reading.** Arm A's battery fails for every expression with a leaf operand
(B1) and silently mis-evaluates unary chains; its search is therefore mostly a
penalty landscape. Arm B should show a materially higher valid-evaluation rate and equal
or better convergence, without changing the deterministic controls (`sin(x)+cos(x)` and
the dual-number checks). Any *regression* in Arm B (e.g. slower convergence) is itself an
important finding and must be reported before merging P2.

**Confounders to control.** The grid includes `x = 0`, which interacts with B8 for
`x^0..`-style expressions; the fitness scale changes if C2–C4 are adopted (do not compare
absolute fitness between Arm B and Arm C); and `srand`/`rand()` implementation details
make cross-toolchain comparisons unsafe — run all arms with the same compiler and flags
on the same machine.

---

*Verification artifacts (probes, evidence logs, paper snapshots) were produced for this
review and are intentionally not committed; every claim above can be reproduced with
`g++ -std=c++17` against `bc70e95` using only the public functions listed in the header
of this document.*
