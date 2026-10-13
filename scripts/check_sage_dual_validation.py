#!/usr/bin/env python3
"""Verifica la salida de una ejecucion real de Untitled.ipynb en SageMath."""

import json
import sys
from pathlib import Path


EXPECTED_CASES = 48
VALIDATION_CALL = "validar_bateria_dual()"


def validation_output(notebook: dict) -> list[str]:
    """Devuelve las lineas producidas por la celda publica de validacion."""
    cells = notebook.get("cells", [])
    matches = [
        cell
        for cell in cells
        if VALIDATION_CALL in "".join(cell.get("source", []))
    ]
    if len(matches) != 1:
        raise AssertionError(
            f"se esperaba una celda con {VALIDATION_CALL!r}, encontradas {len(matches)}"
        )

    lines: list[str] = []
    for output in matches[0].get("outputs", []):
        if output.get("output_type") != "stream":
            continue
        lines.extend("".join(output.get("text", [])).splitlines())
    return lines


def main() -> int:
    if len(sys.argv) != 2:
        print(f"uso: {Path(sys.argv[0]).name} EJECUTADO.ipynb", file=sys.stderr)
        return 2

    notebook_path = Path(sys.argv[1])
    notebook = json.loads(notebook_path.read_text())
    lines = validation_output(notebook)
    case_lines = [line for line in lines if line.startswith("PASS ")]
    summary = f"PASS: {EXPECTED_CASES}/{EXPECTED_CASES} casos"

    if len(case_lines) != EXPECTED_CASES:
        raise AssertionError(
            f"la bateria produjo {len(case_lines)} PASS; se esperaban {EXPECTED_CASES}"
        )
    if summary not in lines:
        raise AssertionError(f"no se encontro el resumen esperado: {summary}")
    if any(line.startswith("FAIL ") for line in lines):
        raise AssertionError("la bateria contiene al menos un FAIL")

    print("SageMath notebook validation: PASS")
    print("executed_notebook=<temporary>")
    print("validation_output:")
    print("\n".join(lines))
    print(f"validated_cases={len(case_lines)}/{EXPECTED_CASES}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, json.JSONDecodeError) as error:
        print(f"SageMath notebook validation: FAIL | {error}", file=sys.stderr)
        raise SystemExit(1)
