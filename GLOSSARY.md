# Glossary

Domain-specific terms used in this codebase.

## Mathematics

| Term | Definition |
|------|-----------|
| **ODE** | Ordinary Differential Equation — an equation involving a function of one variable and its derivatives |
| **2nd-order ODE** | An ODE involving up to the second derivative of the unknown function, e.g. `y'' + y = 0` |
| **Initial conditions** | Values of the unknown function and its derivatives at a specific point, e.g. `y(0) = 1, y'(0) = 1` |
| **Boundary conditions** | Constraints on the solution at the boundaries of the domain |
| **Closed-form solution** | An exact symbolic expression (e.g. `cos(x) + sin(x)`) rather than a numerical approximation |

## Evolutionary computation

| Term | Definition |
|------|-----------|
| **Differential Evolution (DE)** | A population-based stochastic optimiser that evolves candidate solutions through mutation, crossover, and selection |
| **Population** | The current set of candidate solutions being evolved |
| **Individual** | One candidate solution in the population — here, a vector of integer token codes representing a parse tree |
| **Fitness** | A scalar score measuring how well a candidate satisfies the ODE and its conditions; lower is better |
| **Elitism** | Strategy of always preserving the best individuals from one generation to the next |
| **Mutation** | Random perturbation of an individual's token vector to explore new expressions; here `v = x_r1 + F*(x_r2 - x_r3)` (paper eq. (7)) |
| **DE/rand/1/bin** | The DE variant used here: a random base individual `x_r1` plus one scaled difference vector `F*(x_r2 - x_r3)`, followed by binomial crossover |
| **Dither** | Drawing the scaling factor `F` randomly (here once per mutation) instead of keeping it fixed |
| **Trial vector** | The offspring produced by mutation for one target individual, before selection decides whether it replaces it |
| **Recombination / Crossover** | Combining tokens from two or more parent individuals to produce offspring |
| **Generation** | One iteration of the evolutionary loop |
| **Convergence** | When fitness drops below a threshold (1e-5), indicating a sufficiently good solution has been found |

## Expression representation

| Term | Definition |
|------|-----------|
| **Parse tree** | A tree structure representing a mathematical expression; here encoded as a flat vector of integer tokens |
| **Prefix notation** | Operators precede their operands (e.g. `+ 1 2` instead of `1 + 2`); used for the token encoding |
| **Token** | An integer code representing a constant, variable, or operation (see token table in README) |
| **Arity** | The number of operands an operator takes (0 for constants/variables, 1 for unary ops, 2 for binary ops) |
| **Alphabet** | The complete set of available tokens (21 tokens in this implementation) |

## Dual-number arithmetic

| Term | Definition |
|------|-----------|
| **Dual number** | A number of the form `a + bε` where `ε² = 0`; extends real numbers to carry derivative information |
| **2nd-order dual number (2dual)** | An extension carrying value, first derivative, and second derivative: `[f, f', f'']` |
| **Automatic differentiation** | Computing derivatives alongside function evaluation using algebraic rules on dual numbers |
| **Forward pass** | Evaluating an expression tree from leaves to root, propagating dual-number components at each step |
| **`parsv()`** | The main evaluation function — takes a token vector and a dual-number input `[x, y', y'']` and returns `[y(x), y'(x), y''(x)]` |

## Codebase-specific

| Term | Definition |
|------|-----------|
| **`FunbObj`** | The C++ class representing a candidate solution — holds the token vector, evaluates the parse tree, and computes fitness |
| **`x_rango()`** | Generates the evaluation grid (array of `[x, 1.0, 0.0]` triples) over which fitness is measured |
| **`OperF` / `OperF2`** | Functions that dispatch unary (1-arg) and binary (2-arg) operations on dual-number vectors |
| **`narg()`** | Returns the arity (number of arguments) for a given token code |
| **`vector_population`** | 2D vector holding all individuals; each is a `vector<long double>` of token codes |
