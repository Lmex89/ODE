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

The default fitness sums `(y'' + y)⁴` over a grid (testing how close the candidate is to satisfying `y'' + y = 0`), plus penalties for violating initial conditions `y(0) ≠ 1` and `y'(0) ≠ 1`. Trees that fail to parse or contain no `x` variable receive infinite fitness.

### Evolutionary loop

1. Initialise a random population
2. Evaluate fitness for each individual
3. Select via elitism (sort + keep best)
4. Mutate and recombine to produce offspring
5. Repeat until convergence (fitness < 1e-5) or generation limit

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
make test   # build and run the unit tests (dual numbers + seed)
make clean  # remove object files and binaries
```

The Makefile builds with `g++` by default; use `make CXX=icpc` where Intel's
compiler is available.

### Running

```bash
./main.exe                # stochastic: seed from the clock
ODE_SEED=42 ./main.exe    # reproducible: fixed seed, identical run
```

`ODE_SEED` fixes the `srand()` seed and the chosen value is printed as
`Seed: N` at startup. Without the variable the clock is used, as before.

The program prints the best-fitting expression and its fitness each generation. It typically converges in seconds but may run up to 100,000 generations. Output can be very verbose — consider piping through `tail`:

```bash
./main.exe | tail -30
```

### Testing

```bash
make test
```

`test_dual.cpp` asserts the dual-number evaluation pipeline (token arities, `parsv()` on known functions, NaN for incomplete expressions, `tan2dual()` derivatives). `test_semilla.cpp` asserts the `ODE_SEED` resolution rule. `test_mutation.cpp` asserts the DE/rand/1/bin mutation operator (`v = x_r1 + F*(x_r2 - x_r3)` with `F` in `[0,1]`, token rounding/clamping to `1..21`, distinct random base individuals, vector-size safety). They link only the modules under test and do not run the evolutionary loop; `test_mutation.cpp` needs `parse_tree.cpp` and is built with `-DODE_NO_MAIN` so the test's own `main()` runs.

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
| `parse_tree.cpp` | `main()` and the DE loop (population, mutation, recombination, elitism) |
| `dualfunctions.h` | Header for token interpreter and 2nd-order dual-number arithmetic |
| `dualfunctions.cpp` | Implementation of 2dual operations (sin, cos, exp, log, pow, etc.) and tree evaluation |
| `semilla.h` / `semilla.cpp` | Run-seed control: resolves `ODE_SEED`, falling back to the clock |
| `parse_tree_bckp.cpp` | Reference backup — **do not compile** (also defines `main()`) |
| `Makefile` | Build targets: `make`, `make run`, `make test`, `make clean` (`g++` by default) |
| `test_dual.cpp` | Dual-number evaluation tests, run with `make test` |
| `test_mutation.cpp` | DE/rand/1/bin mutation tests, run with `make test` |
| `test_semilla.cpp` | `ODE_SEED` resolution tests, run with `make test` |
| `scripts/ab_experiment.sh` | Reproducible A/B experiment (pre-fix vs post-fix, same seed) |
| `docs/ab-experiment.md` | A/B comparison report and conclusions |
| `Untitled.ipynb` | SageMath scratchpad for reference ODE/solution (not part of the build) |

## Caveats

- Search is stochastic by default (`srand(time(NULL))`); set `ODE_SEED=N` for reproducible runs
- No CI; dual-number and seed tests run with `make test`
- Code comments are in Spanish
- `dualfunctions.h` requires STL headers (`vector`, `string`) to be included before it
- The SageMath notebook's fitness formula is stale; the C++ code is the source of truth
