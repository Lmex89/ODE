# AGENTS.md

C++ research prototype that searches for symbolic solutions of 2nd-order ODEs
using a Differential Evolution / genetic-programming loop (paper linked in
`README.md`). No CI, no package manager; tests run with `make test`.

## Mandatory: codegraph first

**MANDATORY RULE.** This repo is indexed (`.codegraph/`). For any question about
the code, or any edit to it, `codegraph_explore` is your **FIRST** tool call —
before your own `Read`, `Grep`, `glob` or search-based shell calls. One call
returns the verbatim line-numbered source of every relevant symbol **plus** the
call path between them and the blast radius: treat that output as already `Read`
and do not re-open those files to "verify" it.

- Query it with symbol/file names (`parse_tree narg OperF2 FunbObj`), or with a
  natural-language question. Re-query with narrower names when you need more —
  a second `codegraph_explore` beats a grep loop.
- Name the endpoints when the question is a flow ("how does a token vector
  become `[y, y', y'']`"): it surfaces the path, including hops grep can't follow.
- Reach for `Read`/`Grep` only for what codegraph does not index — `Makefile`,
  `README.md`, `Untitled.ipynb`, `AGENTS.md` itself, plain text in `out` — or
  when its output flags a file as changed since the last sync (index lags ~1s).
- Prefer the MCP tool; `codegraph explore "<names or question>"` on the shell
  gives the same output.

## Build & run

- `Makefile` builds with `g++` by default (`CXX ?= g++`, so `make CXX=icpc`
  still works where Intel's `icpc` exists). `make` only builds `main.exe`;
  `make run` compiles if needed and executes it; `make test` compiles and runs
  the five test binaries: `test_dual.cpp` (dual-number tests; links
  `dualfunctions.cpp`), `test_semilla.cpp` (seed resolution; links
  `semilla.cpp`), `test_mutation.cpp` (DE/rand/1/bin mutation tests),
  `test_crossover.cpp` (binomial crossover tests) and `test_grid.cpp`
  (evaluation-grid tests); the last three need `parse_tree.cpp` and are built
  with `-DODE_NO_MAIN` so the test's `main()` wins (`test_grid.cpp` includes
  `parse_tree.cpp` itself, since `FunbObj` has no header); `make clean` removes
  the objects and binaries. Flags: `-std=c++17 -Wall -O3`.
- Never add `parse_tree_bckp.cpp` to a build: it is a reference backup that
  also defines `main()` (duplicate symbol). The Makefile does not compile it.
- `.vscode/tasks.json` builds only the active file, which cannot link — the two
  translation units must be compiled together.
- `dualfunctions.h` is not self-contained: it uses `vector`/`string` without
  including them. Include STL headers before it (as `parse_tree.cpp` does) or
  it fails to compile alone.
- The search is stochastic by default (`srand(time(NULL))`); `ODE_SEED=N` fixes
  the seed (printed as `seed=N` in the startup log line) and makes runs
  reproducible; `ODE_CASE=1` switches to the paper's Case 1
  (`x*y'' + y' - cos(x) = 0`, `y(0)=0`, `y'(0)=1`) instead of the default toy
  ODE (the same startup line carries `case=N`). It usually converges in seconds
  but can run to 100000 generations with very verbose stdout (the committed
  `out` file is a 3.4 MB captured run). Prefer `timeout 60 ./main.exe | tail -30`
  over running bare.
- `scripts/ab_experiment.sh` runs the pre-fix revision and `HEAD` with the same
  `ODE_SEED` values; its results live in `docs/ab-experiment.md`.

## How it works

- `parse_tree.cpp` — `main()` and the DE loop (single population, mutation,
  binomial crossover, greedy selection). `dualfunctions.{h,cpp}` — token
  interpreter and 2nd-order dual-number ("2dual") arithmetic; `parsv()`
  evaluates a tree to `[y(x), y'(x), y''(x)]`. `semilla.{h,cpp}` — resolves
  `ODE_SEED` for reproducible runs.
- `Mutation()` implements DE/rand/1/bin with dither (paper eq. (7)):
  `v = x_r1 + F*(x_r2 - x_r3)` with one `F` random in `[0,1]` per call, the
  result rounded/clamped to a token in `1..21` by `token_valido()`, and the
  three distinct base individuals chosen by `indices_distintos()`. The helper
  `mutacion_rand1()` is the pure formula (no randomness) and is covered by
  `test_mutation.cpp`. A legacy repair still forces one `x` token when the
  mutation removes them all.
- `Recombination()` implements the paper's binomial crossover (eq. (8)) with
  `Cr = 0.2`: each trial gene comes from the mutant with probability `Cr` and
  from the target otherwise, plus one forced `j_rand` index taken from the
  mutant (REVIEW D2). The pure per-gene core `gen_binomial()` is covered by
  `test_crossover.cpp`, and the same legacy repair forces an `x` token when the
  crossover removes them all.
- The DE loop evolves a single population with greedy one-to-one selection:
  the trial replaces its target only when its fitness is lower. There is no
  second population, no cross-pollination and no sort-based elitism.
- An individual is a `vector<long double>` of integer token codes in **prefix
  notation**. Alphabet (decoded in `parse_tree()`, arity in `narg()`):
  `1`-`10` constants 1.0-10.0, `11` x, `12` negation, `13` sin, `14` cos,
  `15` exp, `16` log, `17` sqrt, `18` +, `19` *, `20` /, `21` ^.
- Token codes are duplicated in `parse_tree`, `narg`, `OperF`, `OperF2`, and
  `vector_population` (`rand()%21+1`); keep them in sync when changing the
  alphabet.
- `FunbObj::fitness()` sums `(y'' + y)^4` over `x_rango(0, 1, 100)` — 100
  equally spaced points in `[0,1]`, step `1/99 ≈ 0.0101` (paper §5) — plus
  penalties for `y(0) != 1` and `y'(0) != 1`; trees that do not fully parse
  or contain no `x` get `INFINITY`. Fitness < 1e-5 stops the run; the expected
  winner renders like `(cos(x))+(sin(x))` (solution of `y'' + y = 0` with
  `y(0) = y'(0) = 1`). With `ODE_CASE=1` the residual and conditions switch to
  the paper's Case 1 (`x*y'' + y' - cos(x)`, `y(0)=0`, `y'(0)=1`, exact
  solution `Si(x)`): the candidate constructor takes the case as a defaulted
  4th argument, so `test_grid.cpp` still builds `FunbObj` with 3. As of
  issue #10 (REVIEW B1) the parser cannot evaluate the expressions Case 1
  needs (`x/2` → `NaN`, `x*cos(x/5)` → `x*x`), so Case 1 cannot reach the
  threshold until #10 lands.
- `Untitled.ipynb` is a SageMath scratchpad for the reference ODE/solution, not
  part of the C++ build; its fitness formula is stale — the C++ code is the
  source of truth.

## Mandatory: comments for Python developers

**MANDATORY RULE.** Whenever you add or modify C++ code in this repo (`*.cpp`,
`*.h`), you MUST add comments explaining it for a backend Python developer.
This is not optional and applies to every new symbol, non-obvious line, and
block — do not wait to be asked.

- Write the comments as Python equivalences wherever one exists, in the form
  `// equivalente a ... en Python`, e.g.:
  - `vector<long double>` → `list[float]` (but `long double` has no Python
    equivalent; it is ~80-bit, closer to `decimal` than to `float`)
  - `srand(time(NULL))` → `random.seed()`
  - `rand()%21+1` → `random.randint(1, 21)`
  - `map`/`unordered_map` → `dict`; `sort` with a lambda →
    `sorted(..., key=lambda ...)`; `push_back` → `.append`
  - raw pointers / references → no direct equivalent; explain ownership instead
- When there is NO Python equivalent (header/`.cpp` split, templates, manual
  memory, `long double`, pass-by-reference, undefined behaviour on
  div-by-zero/`NaN`), say so explicitly and explain what the C++ construct
  does instead, in plain terms.
- Keep the surrounding comment language consistent with the file: existing
  comments are in Spanish, so new ones should be Spanish too, with the Python
  snippet/spelling kept as-is (`# como en Python: ...`).
- Comments must explain the *idiom*, not restate the syntax: say why the code
  uses a reference parameter, what the prefix-notation walk does, etc.

## Mandatory: professional logs

**MANDATORY RULE.** Every line the program prints must follow the logging
conventions below. They are the C++ logging best practices checked against
Context7 on 2026-10-12 (spdlog README/wiki + C++ Core Guidelines) applied to
this repo; treat them as the reference for any new output.

- One event per line, spdlog-style pattern `[<t>s] [NIVEL] evento | clave=valor
  ...`, using the helpers in `parse_tree.cpp` (`log_info()`, `log_warn()`,
  `fmt_ciencia()`, ...) instead of raw `cout`. Level tags are 5 chars wide
  (`[INFO ]`, `[WARN ]`) so columns align; `INFO` is configuration/progress/
  results, `WARN` a run that ended without converging.
- Structured fields: stable, lower-case `key=value` names and values with no
  spaces (`vector=[21,11,14]`, not `[21, 11, 14]`) so a script can split the
  line. The keys `seed`, `gen`, `generations`, `best`, `expr`, `vector`,
  `status` and `elapsed_cpu_s` are a contract with
  `scripts/ab_experiment.sh` — renaming one means updating that awk too.
- The timestamp is CPU seconds since start (`fmt_segundos`), not wall clock:
  runs must stay reproducible under a fixed `ODE_SEED`.
- Numbers are formatted explicitly with `ostringstream` + `<iomanip>`
  (`scientific`/`setprecision(6)` for fitness, 2 decimals for times), never
  `cout`'s default 6-significant-digit output, and the formatting flags must
  not leak into `cout` for the next caller.
- `'\n'` plus an explicit `cout.flush()` instead of `std::endl` (Core
  Guidelines F.3). Flushing every event line is deliberate: output is consumed
  live (`tail`) and a `timeout` can kill the process mid-run (same as spdlog's
  console sink). Inside hot loops (the fitness evaluation) print nothing.
- No decoration (banners, `::::`, `====`): the run header and footer are single
  structured lines. No external logging libraries (spdlog/fmt) either — the
  repo has no package manager and builds with plain `g++`; spdlog is the
  reference, not a dependency.

## Repo notes

- `.gitignore` only excludes the local `.codegraph/` index; `main.exe`, `*.o`,
  `out.txt`, and the `out` transcript are committed — don't treat them as
  source.
- Code comments are in Spanish.
- Verification is manual: build, run, inspect the printed best fit and fitness.

## Commit conventions

Follow [Conventional Commits](https://www.conventionalcommits.org/) and the
commitlint `@commitlint/config-conventional` best practices:

- Shape: `type(scope): subject`, blank line, optional body, optional footers;
  scope is optional (`fix(parse-tree): ...`).
- Types: `build`, `chore`, `ci`, `docs`, `feat`, `fix`, `perf`, `refactor`,
  `revert`, `style`, `test`.
- Subject: lower-case type, imperative mood, no sentence/pascal/upper case, no
  trailing full stop; header at most 100 characters.
- Body: explain what and why (not how), one blank line after the subject, lines
  at most 100 characters (URLs exempt).
- Footers: issue refs (`Refs: #123`, `Closes #123`) and breaking changes
  (`BREAKING CHANGE: ...`, or `!` after the type/scope).
- Keep one logical change per commit and review `git diff --cached` before
  committing; never sweep in unrelated edits.

```text
docs: document make targets in AGENTS.md
fix(parse-tree): reject expressions with no free x
```

## Agent skills

### Issue tracker

Issues are tracked in GitHub via the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Five canonical triage roles using default label strings (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout: `GLOSSARY.md` at root + `docs/adr/`. See `docs/agents/domain.md`.
