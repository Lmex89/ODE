#!/usr/bin/env python3
"""Ejecuta las celdas nuevas de validacion sin necesitar SageMath."""

import json
from pathlib import Path


def main():
    notebook = Path(__file__).resolve().parents[1] / "Untitled.ipynb"
    cells = json.loads(notebook.read_text())["cells"]
    marker = "Validacion de numeros duales"
    start = next(
        index
        for index, cell in enumerate(cells)
        if marker in "".join(cell.get("source", []))
    )

    # Como `exec` de un modulo Python, ejecuta solo las celdas nuevas y omite
    # las celdas SageMath historicas. No hay equivalente de Sage que importar.
    namespace = {"__name__": "__main__"}
    for cell in cells[start + 1 :]:
        if cell.get("cell_type") != "code":
            continue
        source = "".join(cell.get("source", []))
        if source:
            exec(compile(source, str(notebook), "exec"), namespace)


if __name__ == "__main__":
    main()
