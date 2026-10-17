# Known Limitations

This document tracks ODE cases that do not converge to J < 1e-5 within reasonable computational budgets, following the paper's approach of reporting approximate solutions.

## Cases

### Case 4: Forced Oscillator (y'' + y = e^x)

**Status**: Documented limitation

**Best achieved fitness**: J ≈ 2.97e-4 (at generation 2284 of 20000)

**Configuration used**:
- Population: 300
- Dimension: 30
- Generations: 20000
- STALE_LIMIT: 500 (default)
- RESTART_FRACTION: 0.8 (default)
- Seed: 42

**Observation**: The search shows slow improvement but does not reach the convergence threshold J < 1e-5 within the allocated budget. This is consistent with the paper's findings that complex ODEs with 11-token solutions require extensive search.

**Approximate solution found**: `(x^2)*(((x)+(3.0))/(6.0))`

**Note**: Further tuning of restart parameters (STALE_LIMIT ∈ {150, 300, 500} × RESTART_FRACTION ∈ {0.5, 0.8, 0.95}) may improve convergence, but the computational cost increases significantly.

### Case 5: Resonance (y'' - y = e^x)

**Status**: Documented limitation

**Best achieved fitness**: J ≈ 5.46e-3 (at generation 1069 of 20000)

**Configuration used**:
- Population: 300
- Dimension: 30
- Generations: 20000
- STALE_LIMIT: 500 (default)
- RESTART_FRACTION: 0.8 (default)
- Seed: 42

**Observation**: The search shows very slow improvement. The exact solution has 15 tokens `(2x·e^x - e^x + e^(-x))/4`, which is at the upper boundary of what the grammar can express efficiently. The search gets stuck in approximate solutions far from the convergence threshold.

**Approximate solution found**: `(x^2)*(((e)+(x^2))/(5.0))`

**Note**: This case may require significantly more generations or different restart parameter tuning to approach convergence. The paper (Mex et al. 2015) reports similar challenges with complex resonance equations.

## Documentation Format

Each limitation is documented with:
- Case number
- ODE equation
- Best achieved fitness (J)
- Configuration used (pop/dim/gen)
- Approximate solution expression
- Reason for limitation (if known)

## Update Policy

This file is updated when:
1. A case consistently fails to converge after parameter tuning
2. A case is documented as a known limitation with its best J
3. New evidence shows a case cannot reach the convergence threshold

The convergence threshold (eps = 1e-5) is NOT relaxed silently. If a case cannot reach it, it is documented here with its best achieved fitness.
