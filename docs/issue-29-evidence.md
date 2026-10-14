# Issue #29: final paper-alignment evidence

This report records the end-to-end evidence for [issue #29](https://github.com/Lmex89/ODE/issues/29)
at commit `0c9ba0e` (`origin/master`, 2026-10-13). The commands below were run
from the issue worktree with `g++` and the Makefile flags
`-std=c++17 -Wall -O3`.

## Decision

The repository now has sufficient evidence to assess the selected paper case
and the numerical pipeline. The result is **not** evidence that this project
reproduces every case and every published run from the paper. The appropriate
scope decision is:

- close this evidence issue once the report is accepted;
- close parent issue #1 only if its success criterion is explicitly understood
  as the documented, selected Case 1 implementation; otherwise keep #1 open
  for the remaining reproduction work.

The deliberate scope boundaries are the 21-token grammar, the selected Case 1
plus the historical toy case, and the absence of the paper's other nine case
studies. These are documented design decisions, not unmeasured claims of full
paper reproduction.

## Acceptance-criteria matrix

| Criterion | Evidence | Result |
|---|---|---|
| Complete C++ and plain-Python suites pass | `make test` | PASS: 236 C++ assertions and 48 Python validation cases |
| SageMath validation evidence exists | `docs/sage-dual-validation-output.txt` | PASS: SageMath 10.10, 48/48 cases |
| Fixed-seed end-to-end run is reproducible | Case 1 CLI run below, executed twice | PASS: deterministic structured result |
| Results are compared with the selected paper case and functional | `test_grid.cpp` and the functional summary below | PASS |
| Remaining deliberate divergences are documented | This report, `README.md`, `REVIEW.md` and `AGENTS.md` | PASS |
| Evidence supports a closure decision for #1 | Decision above | READY for maintainer decision, not a claim of all-paper reproduction |

## C++ and plain-Python suites

Command:

```text
make test
```

The command completed successfully with these assertion counts:

| Binary or check | Result |
|---|---:|
| `test_dual.exe` | 147/147 |
| `test_semilla.exe` | 4/4 |
| `test_mutation.exe` | 21/21 |
| `test_crossover.exe` | 14/14 |
| `test_grid.exe` | 20/20 |
| `test_population.exe` | 30/30 |
| `scripts/check_dual_validation.py` | 48/48 cases |

The C++ total is 236/236 assertions. The plain-Python check validates the
same second-order dual-number formulas against central finite differences at
the three points `x = 0.25, 0.50, 0.75`.

## SageMath evidence

The committed artifact
[`docs/sage-dual-validation-output.txt`](sage-dual-validation-output.txt)
contains the output of the complete notebook execution:

```text
SageMath version 10.10, Release Date: 2026-09-28
SageMath notebook validation: PASS
PASS: 48/48 casos
validated_cases=48/48
```

The documented execution uses the pinned image
`sagemath/sagemath@sha256:310dfe23fc786f3a5a06a7b0387c60201ec79b32c0db941b7624505ee5edb565`.
The local re-run of `make sage-test` could not start in this environment
because no usable `sage` executable or Docker runtime is available. The
committed output is therefore captured validation evidence, while the command
remains the reproducible way to regenerate it on a host with SageMath or
Docker.

## Selected paper case and reference functional

For `ODE_CASE=1`, the implementation evaluates the paper's Case 1 residual

```text
f(x) = x*y''(x) + y'(x) - cos(x)
```

with `y(0) = 0`, `y'(0) = 1`, 100 equally spaced points on `[0, 1]`,
`Delta x = 1/99`, and the paper functional

```text
R = 10*h1^2 + 10*h2^2 + Delta x * sum(f(x_i)^2)
```

The independent expected values in `test_grid.cpp` provide the comparison
against that functional:

| Candidate | Fitness |
|---|---:|
| `y(x) = x` | `0.04545624767792866` |
| `y(x) = sin(x)` | `0.1606370980396136` |
| paper equation (20) approximation | `7.546733118983904e-06` |

The equation (20) approximation is below the program's `1e-5` convergence
threshold, which is the relevant reference result for the selected paper
case. The exact solution is `Si(x) = integral_0^x sin(t)/t dt`; it is not
elementary and is not part of the 21-token grammar.

## Fixed-seed end-to-end run

The short run uses the paper's population and dimension while bounding the
number of generations for a practical evidence run:

```bash
make
ODE_CASE=1 ODE_SEED=42 ODE_POP=100 ODE_DIM=20 ODE_GEN=200 ./main.exe
```

The final structured records from both executions were identical after
removing the intentionally variable CPU timestamp and `elapsed_cpu_s` field:

```text
status=max_generations
generations=200
best=4.545625e-02
expr=x
vector=[11,14,16,18,6,20,14,21,14,6,1,6,4,10,6,13,10,11,1,9]
```

This result is consistent with the independently calculated `R(x)` above.
It did not converge within the deliberately bounded 200 generations; that is
reported as `status=max_generations`, not as a successful paper reproduction.
The fixed seed controls the pseudo-random population and DE decisions. The
timestamp and CPU duration are excluded from the reproducibility comparison
because they measure the current machine, not the algorithmic result.

For comparison, the same fixed seed and paper population/dimension were run
with the full default generation cap:

```bash
ODE_CASE=1 ODE_SEED=42 ODE_POP=100 ODE_DIM=20 ODE_GEN=5000 ./main.exe
```

That run completed all 5,000 generations and produced:

```text
status=max_generations
generations=5000
best=8.952882e-05
expr=(x)*(cos((-1.0*(x))/(3.0)))
vector=[19,11,14,20,12,11,3,21,14,8,5,15,12,11,15,17,18,7,14,11]
```

The full-cap run also remained above `1e-5`. The result is therefore useful
negative evidence about this seed and search budget: the paper approximation
is a valid candidate under the implemented functional, but the stochastic
search did not rediscover it in this end-to-end run.

## Deliberate remaining divergences

The following differences must remain visible when interpreting the evidence:

1. The executable exposes the paper's Case 1 and the historical toy problem,
   not all ten paper case studies.
2. The implementation keeps the repository's 21-token grammar rather than
   importing the paper's 29-symbol grammar. Token mappings are therefore not
   interchangeable with the paper's chromosome examples.
3. The Sage notebook validates dual-number arithmetic and finite-difference
   derivatives. Its historical scratchpad fitness expression is stale; the
   C++ `FunbObj::fitness()` implementation and `test_grid.cpp` are the source
   of truth for the selected paper functional.
4. A stochastic search reaching `max_generations` is a valid reproducible
   outcome, not proof that the reference approximation is absent from the
   grammar. The reference approximation is directly evaluated and pinned by
   the fitness test above.
