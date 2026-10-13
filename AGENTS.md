# AGENTS.md

C++ research prototype that searches for symbolic solutions of 2nd-order ODEs
using a Differential Evolution / genetic-programming loop (paper linked in
`README.md`). No tests, no CI, no package manager.

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
  `make run` compiles if needed and executes it; `make clean` removes the
  objects and the binary. Flags: `-std=c++17 -Wall -O3`.
- Never add `parse_tree_bckp.cpp` to a build: it is a reference backup that
  also defines `main()` (duplicate symbol). The Makefile does not compile it.
- `.vscode/tasks.json` builds only the active file, which cannot link — the two
  translation units must be compiled together.
- `dualfunctions.h` is not self-contained: it uses `vector`/`string` without
  including them. Include STL headers before it (as `parse_tree.cpp` does) or
  it fails to compile alone.
- The search is stochastic (`srand(time(NULL))`), so runs are not reproducible.
  It usually converges in seconds but can run to 100000 generations with very
  verbose stdout (the committed `out` file is a 3.4 MB captured run). Prefer
  `timeout 60 ./main.exe | tail -30` over running bare.

## How it works

- `parse_tree.cpp` — `main()` and the DE loop (population, mutation,
  recombination, sorting/elitism). `dualfunctions.{h,cpp}` — token interpreter
  and 2nd-order dual-number ("2dual") arithmetic; `parsv()` evaluates a tree to
  `[y(x), y'(x), y''(x)]`.
- An individual is a `vector<long double>` of integer token codes in **prefix
  notation**. Alphabet (decoded in `parse_tree()`, arity in `narg()`):
  `1`-`10` constants 1.0-10.0, `11` x, `12` negation, `13` sin, `14` cos,
  `15` exp, `16` log, `17` sqrt, `18` +, `19` *, `20` /, `21` ^.
- Token codes are duplicated in `parse_tree`, `narg`, `OperF`, `OperF2`, and
  `vector_population` (`rand()%21+1`); keep them in sync when changing the
  alphabet.
- `FunbObj::fitness()` sums `(y'' + y)^4` over the grid from `x_rango(0, 1, 0.1)`
  plus penalties for `y(0) != 1` and `y'(0) != 1`; trees that do not fully parse
  or contain no `x` get `INFINITY`. Fitness < 1e-5 stops the run; the expected
  winner renders like `(cos(x))+(sin(x))` (solution of `y'' + y = 0` with
  `y(0) = y'(0) = 1`).
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
