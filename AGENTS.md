# ODE agent notes

## Code navigation

- This repository has a `.codegraph/` index. Before investigating or changing
  code, use `codegraph_explore` (or `codegraph explore`) rather than manually
  searching source. Use `Read`/`Grep` only for unindexed files such as this
  file, `README.md`, `GLOSSARY.md`, notebooks, or files CodeGraph flags as stale.
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
- `ctest --test-dir build` builds and runs the 9 C++ tests registered in
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
  notebook's validation cells, no SageMath needed) and
  `python3 scripts/check_run_log.py` (runs the solver in its short
  deterministic mode and validates the structured log).
- `scripts/run_sage_dual_validation.sh` is the Sage counterpart: it executes
  the entire notebook (`docs/legacy/Untitled.ipynb`) using a local `sage`
  installation or Docker with the pinned image. It is appropriate for changes
  to dual-number math or the notebook.
- The search is stochastic and its default 5,000 generations can be noisy.
  Use a fixed seed and a short cap for a reproducible smoke run:
  `ODE_SEED=42 ODE_GEN=200 timeout 60 ./build/ode-solver | tail -30`.
  `ODE_CASE=1` selects the paper case, `ODE_CASE=2` the hyperbolic case
  (unset/0: the toy ODE). `ODE_POP`, `ODE_DIM`, `ODE_GEN`, and
  `ODE_WARM_START=1` override the defaults (100 individuals, dimension 20,
  5,000 generations); warm start (20-token seeds) activates only when the
  dimension is 20 (the default).
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
- `tests/test_grid.cpp`, `tests/test_population.cpp`, and
  `tests/test_funbobj_const.cpp` include `parse_tree.cpp` directly because
  `FunbObj` has no header. Their CMake rules intentionally compile with
  `-DODE_NO_MAIN` and keep `parse_tree.cpp` off the compile line; do not
  "simplify" this into compiling that source twice.
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
