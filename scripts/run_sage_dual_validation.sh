#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NOTEBOOK="$ROOT_DIR/Untitled.ipynb"
WORK_DIR="$(mktemp -d)"
EXECUTED_NOTEBOOK="$WORK_DIR/Untitled.executed.ipynb"
SAGE_IMAGE="${SAGE_IMAGE:-sagemath/sagemath@sha256:310dfe23fc786f3a5a06a7b0387c60201ec79b32c0db941b7624505ee5edb565}"
SAGE_TIMEOUT="${SAGE_TIMEOUT:-120}"
# El contenedor usa un usuario distinto al del host; se prepara solo el
# archivo de salida para que Sage pueda escribirlo sin abrir todo el temporal.
chmod 755 "$WORK_DIR"
touch "$EXECUTED_NOTEBOOK"
chmod 666 "$EXECUTED_NOTEBOOK"

cleanup() {
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT

if command -v sage >/dev/null 2>&1; then
    sage --version
    sage -python -m jupyter nbconvert \
        --to notebook \
        --execute \
        --ExecutePreprocessor.kernel_name=sagemath \
        --ExecutePreprocessor.timeout="$SAGE_TIMEOUT" \
        --output "$EXECUTED_NOTEBOOK" \
        "$NOTEBOOK"
else
    if ! command -v docker >/dev/null 2>&1; then
        echo "SageMath validation requires sage or docker" >&2
        exit 127
    fi

    docker run --rm \
        --entrypoint bash \
        -e "SAGE_TIMEOUT=$SAGE_TIMEOUT" \
        -v "$ROOT_DIR:/repo:ro" \
        -v "$WORK_DIR:/out" \
        -w /repo \
        "$SAGE_IMAGE" \
        -lc '
            set -euo pipefail
            sage --version
            sage -python -m jupyter nbconvert \
                --to notebook \
                --execute \
                --ExecutePreprocessor.kernel_name=sagemath \
                --ExecutePreprocessor.timeout="${SAGE_TIMEOUT:-120}" \
                --output /out/Untitled.executed.ipynb \
                Untitled.ipynb
        '
fi

python3 "$ROOT_DIR/scripts/check_sage_dual_validation.py" "$EXECUTED_NOTEBOOK"
