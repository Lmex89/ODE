# ODE — Symbolic ODE Solver via Differential Evolution

A C++ research prototype that searches for **symbolic (closed-form) solutions** to second-order ordinary differential equations (ODEs) using a Differential Evolution / genetic-programming hybrid loop.

Based on the article: [Solving Ordinary Differential Equations Using Differential Evolution](https://www.hindawi.com/journals/ddns/2015/910316/) (Hindawi, 2015).

## How it works

The program evolves a population of candidate expressions (parse trees) to find a function `y(x)` that satisfies a given ODE together with initial/boundary conditions.

### Representation

Each individual in the population is a `vector<long double>` of integer token codes in **prefix notation**. The token alphabet is:

| Tokens | Meaning |
|--------|---------|
| `1`–`10` | Constants 1.0–10.0 |
| `11` | Variable `x` |
| `12` | Negation |
| `13` | sin |
| `14` | cos |
| `15` | exp |
| `16` | log |
| `17` | sqrt |
| `18` | Addition (+) |
| `19` | Multiplication (*) |
| `20` | Division (/) |
| `21` | Power (^) |

### Evaluation with 2nd-order dual numbers

Expressions are evaluated using **second-order dual-number arithmetic** ("2dual"), which simultaneously computes `y(x)`, `y'(x)`, and `y''(x)` in a single forward pass. This avoids symbolic differentiation of the trees and lets the fitness function directly access derivatives needed by the ODE.

### Fitness function

The default fitness sums `(y'' + y)⁴` over a grid of 100 equally spaced points in `[0,1]` (step `1/99 ≈ 0.0101`, per the paper; testing how close the candidate is to satisfying `y'' + y = 0`), plus penalties for violating initial conditions `y(0) ≠ 1` and `y'(0) ≠ 1`. Trees that fail to parse or contain no `x` variable receive infinite fitness.

`ODE_CASE=1` switches to **Case 1 of the paper** (eq. (18)): residual `x·y'' + y' − cos(x)` and initial conditions `y(0) = 0`, `y'(0) = 1` over the same 100-point grid `[0,1]`, with the same weights and stop threshold. Its exact solution is `Si(x) = ∫₀ˣ (sin t)/t dt` (not elementary); the paper reports the approximation `y_s(x) = x·cos(x/(5·cos(cos 9)))`, which scores ≈ `1.5e-8` under this quartic fitness. Note: evaluating expressions like this one needs the operand-consumption semantics of issue #10 (`REVIEW.md`, B1); until that lands, the Case 1 search cannot reach the threshold.

### Evolutionary loop

Single-population DE/rand/1/bin:

1. Initialise a random population
2. For each target individual: pick three distinct random individuals as base vectors, form the mutant `v = x_r1 + F*(x_r2 - x_r3)` (one random `F` in `[0,1]` per mutation), build the trial by binomial crossover with `Cr = 0.2` (one forced index always comes from the mutant), and keep the trial only if its fitness is lower than the target's (greedy one-to-one selection)
3. Track the best individual of the generation
4. Repeat until convergence (fitness < 1e-5) or generation limit

The expected solution for the default ODE `y'' + y = 0` with `y(0) = y'(0) = 1` is `y = cos(x) + sin(x)`.

## Build & run

### Prerequisites

- A C++17 compiler (`g++` or `icpc`)

### Compiling

**Without the Makefile** (plain `g++`):

```bash
g++ -std=c++17 -Wall -O3 dualfunctions.cpp semilla.cpp parse_tree.cpp -o main.exe
```

**With the Makefile** (recommended):

```bash
make        # build ./main.exe (incremental)
make run    # build if needed, then execute
make test   # build and run the unit tests (dual numbers, seed, mutation, crossover, grid)
make clean  # remove object files and binaries
```

The Makefile builds with `g++` by default; use `make CXX=icpc` where Intel's
compiler is available.

### Running

```bash
./main.exe                  # stochastic: seed from the clock
ODE_SEED=42 ./main.exe      # reproducible: fixed seed, identical run
ODE_CASE=1 ODE_SEED=42 ./main.exe   # paper's Case 1 instead of the default ODE
```

`ODE_SEED` fixes the `srand()` seed and the chosen value is printed in the
startup log line (`[INFO ] de_run_start | seed=N ...`). Without the variable
the clock is used, as before. `ODE_CASE` selects the problem (`0` or unset:
default toy ODE; `1`: paper's Case 1, `x·y'' + y' − cos(x) = 0` with `y(0)=0`,
`y'(0)=1`); the same line carries `case=N`.

The program logs one structured line per event (`[  0.12s] [INFO ] gen=1
best=... best_ever=... expr=...`), plus `new_best` records, a final `solution`
line and a `de_run_end` summary. It typically converges in seconds but may run
up to 100,000 generations. Output can be very verbose — consider piping through
`tail`:

```bash
./main.exe | tail -30
```

### Testing

```bash
make test
```

`test_dual.cpp` asserts the dual-number evaluation pipeline (token arities, `parsv()` on known functions, NaN for incomplete expressions, `tan2dual()` derivatives). `test_semilla.cpp` asserts the `ODE_SEED` resolution rule. `test_mutation.cpp` asserts the DE/rand/1/bin mutation operator (`v = x_r1 + F*(x_r2 - x_r3)` with `F` in `[0,1]`, token rounding/clamping to `1..21`, distinct random base individuals, vector-size safety). `test_crossover.cpp` asserts the binomial crossover (`gen_binomial()` decision per gene, trial size and leftover tokens, forced `j_rand` index, mutant-gene fraction ≈ `Cr`, `x`-token repair). `test_grid.cpp` asserts the evaluation grid (`x_rango()` returns exactly `npoints` equally spaced points with both endpoints included, `FunbObj::fitness()` sums the paper's 100 points, and the Case 1 (`ODE_CASE=1`) residual/weights). They link only the modules under test and do not run the evolutionary loop; `test_mutation.cpp`, `test_crossover.cpp` and `test_grid.cpp` need `parse_tree.cpp` and are built with `-DODE_NO_MAIN` so the test's own `main()` runs (`test_grid.cpp` includes `parse_tree.cpp` directly to reach the header-less `FunbObj`).

The SageMath notebook validation is a separate target because it needs a real
SageMath kernel:

```bash
make sage-test
```

`make sage-test` uses a native `sage` executable when available. Otherwise it
uses Docker and the pinned SageMath image documented in
[docs/sage-dual-validation.md](docs/sage-dual-validation.md). It executes the
complete notebook, then checks the stored cell output for 48 passing
expression/point cases and the `PASS: 48/48 casos` summary.

### A/B experiment

`scripts/ab_experiment.sh` builds the pre-fix revision and `HEAD` with the same
flags, runs both with identical `ODE_SEED` values, and writes a metrics table
(fitness, vector, generations, times). Results and conclusions are documented in
[docs/ab-experiment.md](docs/ab-experiment.md).

```bash
scripts/ab_experiment.sh --seed 42 --timeout 300
```

## Files

| File | Description |
|------|-------------|
| `parse_tree.cpp` | `main()` and the DE loop (single population, mutation, binomial crossover, greedy selection) |
| `dualfunctions.h` | Header for token interpreter and 2nd-order dual-number arithmetic |
| `dualfunctions.cpp` | Implementation of 2dual operations (sin, cos, exp, log, pow, etc.) and tree evaluation |
| `semilla.h` / `semilla.cpp` | Run-seed control: resolves `ODE_SEED`, falling back to the clock |
| `parse_tree_bckp.cpp` | Reference backup — **do not compile** (also defines `main()`) |
| `Makefile` | Build targets: `make`, `make run`, `make test`, `make clean` (`g++` by default) |
| `test_dual.cpp` | Dual-number evaluation tests, run with `make test` |
| `test_mutation.cpp` | DE/rand/1/bin mutation tests, run with `make test` |
| `test_crossover.cpp` | Binomial crossover tests, run with `make test` |
| `test_semilla.cpp` | `ODE_SEED` resolution tests, run with `make test` |
| `test_grid.cpp` | Evaluation-grid tests, run with `make test` |
| `scripts/ab_experiment.sh` | Reproducible A/B experiment (pre-fix vs post-fix, same seed) |
| `docs/ab-experiment.md` | A/B comparison report and conclusions |
| `scripts/run_sage_dual_validation.sh` | Execute the complete notebook in SageMath and verify its output |
| `scripts/check_sage_dual_validation.py` | Check the executed notebook's 48-case PASS/FAIL result |
| `docs/sage-dual-validation.md` | SageMath command, environment, and captured evidence |
| `Untitled.ipynb` | SageMath scratchpad for reference ODE/solution (not part of the build) |

## Caveats

- Search is stochastic by default (`srand(time(NULL))`); set `ODE_SEED=N` for reproducible runs and `ODE_CASE=1` for the paper's Case 1
- No CI; dual-number, seed, mutation, crossover and grid tests run with `make test`
- Code comments are in Spanish
- `dualfunctions.h` requires STL headers (`vector`, `string`) to be included before it
- The SageMath notebook's fitness formula is stale; the C++ code is the source of truth
