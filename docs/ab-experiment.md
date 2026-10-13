# A/B Experiment: before vs after the issue #2 bug fixes

Report for [#4](https://github.com/Lmex89/ODE/issues/4): are the existing
results in `out` / `out.txt` trustworthy, or do they need to be regenerated
after the fixes from issue #2?

## Versions compared

| Version | Revision | Content |
|---------|----------|---------|
| A ("antes") | `bc70e95` | Code **before** the issue #2 fixes (the `parsv()` `\|\|` bug, token 12 arity, `tan2dual()`) |
| B ("despues") | `feat/ab-experiment` (PR #8 merged) | Code **after** the fixes, plus the `ODE_SEED` hook |

Both were compiled with the same compiler and the same flags as the Makefile
(`g++`, `-std=c++17 -Wall -O3`). Neither revision accepted a fixed seed
(`srand(time(NULL))`), so version A was patched with the *same* seed hook that
version B carries (`scripts/ab_seed_hook.patch` + `semilla.{h,cpp}`): the patch
only changes the seed source, it does not touch the dual-number math or the DE
loop. A and B therefore differ exactly in the issue #2 fixes.

## Method

```
scripts/ab_experiment.sh --seed <S> --timeout 300
```

For every seed in {1, 42, 2026}: both binaries run from to a temporary
directory with `ODE_SEED=<S>` and a 300 s `timeout`, and each log is parsed for
the best fitness, the last-improvement expression/vector, the generations
reached and the wall time. `best fitness` is the minimum over all printed
`BestCurrent` values, which equals `Bestfit` at exit (it only decreases). The
expression/vector shown for a run that did not converge belong to the last
*improvement block*, not necessarily to the individual with the best fitness:
the loop only dumps the winner vector when it converges. CPU time
(`Execution Time`) is only printed when the loop terminates, so timed-out runs
show wall time only.

Seed reproducibility was checked separately: two `ODE_SEED=7` runs of B produced
byte-identical output up to the point where the `timeout` cut them off
(35 lines compared, only `Execution Time` and the cut point differ).

## Results

All six runs ended by timeout (no convergence to fitness < 1e-5):

| Version | Seed | Converged | Generations | Best fitness | Best individual (from the `BestCurrent` log) |
|---------|------|-----------|-------------|--------------|----------------------------------------------|
| A | 1 | no | 232 | 0.0401972 | `(cos(x))+(sin(sin(x)))` |
| B | 1 | no | 234 | 0.0401972 | `(cos(x))+(sin(sin(x)))` |
| A | 42 | no | 232 | 2.5333 | `(x)+(sin(cos(x)))` |
| B | 42 | no | 235 | 2.5333 | `(cos(x))+(x)` |
| A | 2026 | no | 233 | 1.13369 | `(sin(cos(x)))+(log(((10.0)**(sin(log(-1.0*(5.0)))))**((9.0)/(cos(6.0)))))` |
| B | 2026 | no | 236 | 0.0401972 | `(cos(x))+(sin(sin(x)))` |

Reading of the table:

- **A and B diverge on every seed.** Seed 1 reaches the same individual in
  both; seeds 42 and 2026 reach different best individuals and different
  fitness values, with B finding the better near-solution in seed 2026
  (0.0401972 vs 1.13369).
- The winner vector in both versions has the shape
  `[18, 14, 11, 13, 13, 11, 1, ..., 1]` = `cos(x) + sin(sin(x))`, fitness
  0.0401972. It fits the initial conditions but is not the solution of the ODE.
- `Execution Time` was never printed (no run terminated), so only wall time
  (300 s per run) is recorded.

## Where A and B evaluate differently

The two `parsv()` fixes change the fitness of trees that the DE actually
generates:

- **Token 12 (negation).** With `x = 0.5`, the complete tree `-sin(x)`
  (`[12, 13, 11, 1...1]`) evaluates to `[-0.5, -1, 0]` in A (it negates the
  input point) and to `[-0.479426, -0.877583, 0.479426]` in B (it negates the
  subexpression). Since `rand() % 21 + 1` picks token 12 about 5 % of the
  time, this alone re-ranks a large part of every population.
- **The `||` initialization bug.** In A every token (including operators and
  functions) starts with a numeric value equal to its token code, so tree
  slots that a correct evaluation leaves `NaN` are fabricated in A. A sweep of
  200 000 uniformly random 20-token trees (the same distribution as
  `vector_population()`) shows **20 356 of them (10.2 %) evaluate differently**:
  444/75 462 (0.6 %) of the trees *without* negation and 19 912/124 538 (16 %)
  of the trees *with* negation — the two fixes above account for every
  difference.

For complete trees without negation (e.g. `cos(x) + sin(sin(x))`) both
versions agree, which is why the same individual can win in both.

A separate observation, **present in both A and B**: the right-to-left
operand-consumption scheme of `parsv()` does not always match the expression
that `parse_tree()` renders. With `x = 0.5`, the 20-token tree `x + sin(x)`
renders as `(x)+(sin(x))` but evaluates to `[1, 2, 0]` (the correct values
would be `[0.979426, 1.877583, -0.479426]`). That is outside the scope of
issue #2, but it limits how far the DE fitness can approach the real residual
and belongs to the Phase-3 alignment (issue #1).

## Are `out` / `out.txt` trustworthy?

**No.** They must be treated as stale artifacts:

- Their transcript format (`y(x):= ...`) does not exist anywhere in the
  repository history: the first commit `e1f52e0` already printed `y(x)= `.
  Both files were committed in that first commit as captures of an earlier,
  pre-repo build, so they cannot correspond to any revision here.
- `out` is a truncated capture: 3.3 M lines, no `Execution Time`, ending
  mid-run at generation 1197 with `BestCurrent1 :5.44721 ... cos((2.0)+(2.0))`.
  `cos((2.0)+(2.0))` is a constant with fitness ≈ 5.4, i.e. a fabricated
  evaluation, not a solution.
- `out.txt` claims convergence in **generation 1 with fitness 0** for
  `y(x) = x`. Neither revision reproduces anything like that: with the same
  seeds, A needs 232 generations to reach 0.0401972 at best, and under B
  `y(x) = x` scores 302.53 (initial-condition penalty plus residual). The
  fitness-0 shortcut came from the pre-repo code, not from `bc70e95`.

## New baseline (corrected code)

With B (the fixed pipeline) and a fixed seed, the new reproducible baseline is
`ODE_SEED=1` or `ODE_SEED=2026` → best `cos(x) + sin(sin(x))`, fitness
`0.0401972`, in under 300 s. It is a *near* solution: the initial conditions
hold, but it is not the exact solution of `y'' + y = 0` with `y(0) = y'(0) = 1`
(that is `cos(x) + sin(x)`, fitness 0). Beating this plateau is the job of the
Phase-3 DE alignment, not of the bug fixes.

## Reproduce

```bash
make test                                                  # unit tests
scripts/ab_experiment.sh --seed "1 42 2026" --timeout 300  # this report
```
