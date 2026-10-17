# ODE agent notes

## Code navigation

- **Always use codegraph — never `grep`/`Read` indexed source first.** This repo
  has a `.codegraph/` index over every symbol and file. For any code question —
  a bug, a flow ("how does X reach Y"), or reading a file you plan to edit —
  call `codegraph_explore` (or `codegraph explore`) first: one call returns the
  verbatim, line-numbered source plus who calls it and what depends on it. Fall
  back to `Read`/`Grep` only for files codegraph does not index (this file,
  `README.md`, `GLOSSARY.md`, notebooks, `docs/`) or files codegraph flags as
  stale.
- Read `GLOSSARY.md` before work that uses domain terminology. Its token
  tables are stale (former 21-token alphabet); `token.h` is the source of
  truth and defines the contiguous alphabet `1..29`.

## Build, test, and run

- Build out of source with CMake (C++17, `-Wall`, `-O3`): configure with
  `cmake -B build`, then compile with `cmake --build build`. The executable
  is `build/ode-solver`. Sources live in `src/`, tests in `tests/`, and the
  historical files in `docs/legacy/` (the backup
  `docs/legacy/parse_tree_bckp.cpp` and the notebook
  `docs/legacy/Untitled.ipynb`).
- `ctest --test-dir build` builds and runs the 12 C++ tests registered in
  `CMakeLists.txt`: each test file under `tests/` is its own executable, and
  its exit code is nonzero exactly when an assert fails (the same contract
  as pytest). For a focused test, build its target and run it directly:
  `cmake --build build --target test_mutation` and `./build/test_mutation`.
- `build/` is ignored by git; never commit build artifacts. When adding a
  test, put it under `tests/` and register it in `CMakeLists.txt`; keep this
  file in sync (`scripts/check_agents_md.py` verifies AGENTS.md against the
  repo tree and `CMakeLists.txt`).
- The notebook/log checkers run standalone (they are not registered with
  ctest): `python3 scripts/check_dual_validation.py` (executes the
  notebook's validation cells, no SageMath needed),
  `python3 scripts/check_run_log.py` (runs the solver in terminal mode and
  validates the structured log), and `python3 scripts/check_log_redirect.py`
  (runs the binary in temporary directories to verify the default
  `ode-solver.log` and `ODE_LOG=terminal`).
- `python3 scripts/check_ab_experiment.py` runs `scripts/ab_experiment.sh` in
  its short deterministic mode (`ODE_GEN=200`, seed 42, a 20s timeout per run)
  and validates that `metricas.tsv` extracts metrics for both builds: the
  current build in terminal mode and the historical `bc70e95` build left
  untouched. It compiles both revisions, so it takes around a minute.
- `python3 scripts/check_all_cases.py` runs tests for ODE cases 3-7
  (damped oscillator, forced oscillator, resonance, Legendre, Hermite) in two modes:
  - `--mode smoke` (default): `ODE_GEN=200`, `ODE_SEED=42`. Validates that each
    case completes, produces finite fitness, and improves over the first generation.
  - `--mode convergence`: Uses per-case configs (see CASO_CONFIGS in the script),
    `ODE_SEED=42`. Validates that each case reaches `status=converged` and
    `best < 1e-5`. Cases documented in `docs/limitations.md` print a warning
    but don't fail the script.
  
  Output format (smoke mode):
  ```
  Smoke test: cases [3, 4, 5, 6, 7], gen=200, seed=42
  
  ============================================================
  Summary:
    Case 3: PASS
    Case 4: PASS
    Case 5: PASS
    Case 6: PASS
    Case 7: PASS
  ============================================================
  check_all_cases: PASS
  ```
  Exit code is 0 if all cases pass (or are documented limitations in convergence
  mode), non-zero otherwise.
- `scripts/run_sage_dual_validation.sh` is the Sage counterpart: it executes
  the entire notebook (`docs/legacy/Untitled.ipynb`) using a local `sage`
  installation or Docker with the pinned image. It is appropriate for changes
  to dual-number math or the notebook.
- The solver writes its complete run log to `ode-solver.log` in the current
  working directory by default, truncating it on each run; stdout stays empty.
  `ODE_LOG=terminal` sends the log to stdout and creates no file, while
  `ODE_LOG_FILE=<path>` overrides the file path. If the file cannot be opened,
  the run falls back to stdout and warns with `log_open_failed | path=...`.
- The search is stochastic and its default 5,000 generations can be noisy.
  Use a fixed seed and a short cap for a reproducible smoke run:
  `ODE_SEED=42 ODE_GEN=200 ODE_LOG=terminal timeout 60 ./build/ode-solver | tail -30`.
  `ODE_CASE=1` selects the paper case, `ODE_CASE=2` the hyperbolic case,
  `ODE_CASE=3` the damped oscillator, `ODE_CASE=4` the forced oscillator,
  `ODE_CASE=5` the resonance case, `ODE_CASE=6` the Legendre equation
  (variable coefficients: (1-x²)y'' - 2xy' + 6y = 0, ICs y(0)=-1/2, y'(0)=0,
  solution P₂(x) = (3x²-1)/2), `ODE_CASE=7` the Hermite equation
  (variable coefficients: y'' - 2xy' + 4y = 0, ICs y(0)=-2, y'(0)=0,
  solution H₂(x) = 4x²-2), and unset/0 the toy ODE. `ODE_POP`, `ODE_DIM`,
  `ODE_GEN`, and `ODE_WARM_START=1` override the defaults (100 individuals,
  dimension 20, 5,000 generations); warm start (20-token seeds) activates
  only when the dimension is 20 (the default).
   `ODE_STALE_LIMIT` (default 500) and `ODE_RESTART_FRACTION` (default 0.8) control the restart mechanism: after `stale_limit` generations without improvement, `restart_fraction` of the population is replaced with fresh random individuals while preserving the best solution found.
   `ODE_RESTART_MODE` (`legacy`, the default, or `adaptive`) selects the restart mode and is reported in the `de_run_start` header. In `adaptive` mode each restart keeps the fittest `100% − ODE_RESTART_MIN_FRACTION` of the population (0.4, the record always survives) and replaces only the worst share, whose `ODE_RESTART_MUTATION_SHARE` (0.5) are 1–2-token point mutations of elites and the rest fresh uniform individuals; the `restart |` line reports `trigger=stale`, `fraction=`, `kept=` and `replaced=`. `ODE_RESTART_CAP` (3) and `ODE_DIV_THRESHOLD` (0.05) are resolved with the same invalid-value fallback policy as the other `ODE_*` keys but are not consumed by the search yet (they arrive with the escalation and diversity tickets of #125).
- There is no CI. For behaviorally neutral DE changes, compare fixed-seed runs
  with `scripts/ab_experiment.sh --seed 42 --timeout 300`.

## Architecture and invariants

- `src/parse_tree.cpp` owns `main()` (behind `ODE_NO_MAIN` for tests), the DE
  loop, `FunbObj` fitness, mutation, and crossover. `src/dualfunctions.{h,cpp}`
  interprets prefix token vectors as `[y, y', y'']`; `src/semilla.{h,cpp}`
  resolves `ODE_SEED`; `src/token.h` is the token vocabulary, arity,
  rendering, and random-token source of truth.
  `docs/legacy/parse_tree_bckp.cpp` is a historical copy that also defines
  `main()`: never add it to a build.
- `src/salida.{h,cpp}` owns the output destination: `resolver_salida()` maps
  `ODE_LOG`/`ODE_LOG_FILE` to a mode and path, and `RedireccionSalida` is the
  RAII redirect that swaps `std::cout` once at startup and restores it on
  exit.
- `tests/test_grid.cpp`, `tests/test_population.cpp`,
  `tests/test_funbobj_const.cpp`, and `tests/test_ode_cases.cpp` include
  `parse_tree.cpp` directly because `FunbObj` has no header. Their CMake rules
  intentionally compile with `-DODE_NO_MAIN` and keep `parse_tree.cpp` off the
  compile line; do not "simplify" this into compiling that source twice.
- Interpreter operand blanking to `NAN` is deliberate. Invalid/incomplete or
  x-independent trees receive `INFINITY` fitness rather than throwing, and
  `fitness()` caches per individual: individuals are replaced whole, never
  mutated in place. Preserve those contracts and keep the hot fitness loop
  free of output.
- When extending tokens, keep the alphabet contiguous (append after
  `TOKEN_MAX`, bump `TOKENS_ALFABETO`), update the `src/token.h` helpers
  (render, arity, category predicates) and the evaluator dispatch in
  `src/dualfunctions.cpp`, and cover it with focused token/dual tests. Use
  named token constants and `random_token()` rather than literal bounds.

## C++ and output conventions

- Apply `docs/cpp-standards.md` to new and modified C++ only; do not perform
  unrelated cleanup of legacy code. In particular, use 4-space indentation,
  `underscore_style` for new names, `const` references for nontrivial inputs,
  RAII (no raw owning pointers), and small pure helpers for testable logic.
- C++ comments are Spanish and explain non-obvious C++ idioms for a backend
  Python developer, including a Python equivalent—or explicitly saying when
  none exists. Add or update them with C++ changes.
- All `ode-solver` output must go through `log_info()`/`log_warn()` and the
  structured `[<cpu>s] [LEVEL] event | key=value` format;
  `scripts/check_run_log.py` rejects lines outside it and expects the author
  signature (`signature | author=Lmex`) last. Keep the `seed`, `gen`,
  `generations`, `best`, `expr`, `vector`, `status`, and `elapsed_cpu_s` keys:
  `scripts/ab_experiment.sh` parses them. Use explicit formatting, `\n` plus
  `flush()`, and no raw `cout` in production numerical code.

## Commits and issues

- Use Conventional Commits (`type(scope): imperative subject`); keep a commit
  to one logical change and do not include generated binaries or run output.
- Issues live on GitHub and are worked with `gh`; label and triage conventions
  are in `docs/agents/issue-tracker.md`.
