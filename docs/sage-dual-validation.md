# SageMath second-order dual-number validation

The validation cell in `docs/legacy/Untitled.ipynb` evaluates 16 expressions at
the three points `x = 0.25, 0.50, 0.75`. For each of the 48 cases it compares
the value, first derivative, and second derivative carried by the Python 2dual
tuple with central finite differences. The tolerance is `1e-5`; the domain
points avoid the non-real or non-finite branches of `log`, `sqrt`, and `2/x`.

The notebook is executed in full, not by extracting only the validation cells.
That keeps the existing ODE and closed-form solution material executable too.
The historical fitness cell now defines `f1` and `f2` from the Sage solution and
uses named substitution (`f(x=ii)`), which is required by SageMath 10.10.

## Reproducible execution

From the repository root, run:

```bash
make sage-test
```

The target uses a local `sage` executable when available. Otherwise
`scripts/run_sage_dual_validation.sh` runs the notebook with
`sagemath/sagemath@sha256:310dfe23fc786f3a5a06a7b0387c60201ec79b32c0db941b7624505ee5edb565`
through Docker. The equivalent kernel execution command is:

```bash
sage -python -m jupyter nbconvert \
  --to notebook \
  --execute \
  --ExecutePreprocessor.kernel_name=sagemath \
  --ExecutePreprocessor.timeout=120 \
  --output Untitled.executed.ipynb \
  docs/legacy/Untitled.ipynb
```

The captured run below was made on 2026-10-13 with SageMath 10.10 (Python
3.12.3) from the pinned Docker image. `check_sage_dual_validation.py` verifies
that the executed validation cell contains 48 `PASS` case lines, no `FAIL`
lines, and the final `PASS: 48/48 casos` summary.

## Result

```text
SageMath notebook validation: PASS
validated_cases=48/48
```

The complete per-expression/per-point output is preserved in
[`sage-dual-validation-output.txt`](sage-dual-validation-output.txt).
