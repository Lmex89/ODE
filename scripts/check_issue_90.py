#!/usr/bin/env python3
"""Verifica los criterios de aceptacion del issue #90: sweep de rutas obsoletas
en los ejemplos de docs/*.md.

Parte de #86 (Update remaining stale paths after the reorg). Criterios:

1. Los ejemplos smoke-run de docs/cpp-standards.md, docs/cpp-standards-plan.md
   y docs/ab-experiment.md usan ./build/ode-solver
2. El comando del notebook en docs/sage-dual-validation.md usa
   docs/legacy/Untitled.ipynb
3. docs/issue-29-evidence.md queda intacto (registro historico pre-reorg)
4. Los comandos make de build de estos archivos no se alteran (los posee #53)

Seams de verificacion:

1. Contenido de los ejemplos smoke-run (criterio 1): los tres archivos ya no
   mencionan ./main.exe y al menos un ejemplo apunta a ./build/ode-solver.
   Detecta rutas viejas o un sweep incompleto; no compila ni corre el solver.

2. Ruta del notebook en el comando nbconvert (criterio 2): el comando de
   docs/sage-dual-validation.md referencia docs/legacy/Untitled.ipynb y no
   queda ninguna mencion a un Untitled.ipynb raiz (el nombre de salida
   Untitled.executed.ipynb no cuenta). Detecta referencias raiz; no ejecuta el
   notebook.

3. Evidencia historica (criterio 3): docs/issue-29-evidence.md conserva sus
   ejemplos ./main.exe y no fue reescrito al layout nuevo. Detecta un sweep
   que en realidad no debia tocar ese archivo; no verifica todo su contenido.

4. Comandos make conservados (criterio 4): la linea de make test sigue en
   docs/ab-experiment.md y la de make sage-test en docs/sage-dual-validation.md.
   Detecta que el sweep altero los comandos de #53; no valida que funcionen
   (desde #53 no hay Makefile). Sin backticks alrededor de los comandos para no
   disparar el criterio 6 de check_issue_53.py, que escanea scripts/*.py.

En Python seria un test de integracion sobre el contenido de los docs: no hay
una API que ejercitar, el contrato es el texto de los archivos.
"""

import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DOCS = REPO_ROOT / "docs"

ARCHIVOS_SMOKE_RUN = (
    DOCS / "cpp-standards.md",
    DOCS / "cpp-standards-plan.md",
    DOCS / "ab-experiment.md",
)
DOC_NOTEBOOK = DOCS / "sage-dual-validation.md"
EVIDENCIA_HISTORICA = DOCS / "issue-29-evidence.md"

# Comandos make que #53 posee y que este sweep no debe tocar (archivo, patron
# de la linea esperada sin espacios extremos).
COMANDOS_MAKE_CONSERVADOS = (
    (
        DOCS / "ab-experiment.md",
        re.compile(r"^\s*make\s+test(?:\s|$)", re.MULTILINE),
    ),
    (
        DOCS / "sage-dual-validation.md",
        re.compile(r"^\s*make\s+sage-test(?:\s|$)", re.MULTILINE),
    ),
)

# Ruta nueva del notebook; solo esta prefijacion es valida.
RUTA_NOTEBOOK_NUEVA = "docs/legacy/Untitled.ipynb"


def leer_doc(errores, ruta):
    """Lee un doc; agrega el error y devuelve None si no existe. En Python
    seria una fixture que devuelve el texto o marca el test como error."""
    if not ruta.exists():
        errores.append(f"no existe {ruta.relative_to(REPO_ROOT)}")
        return None
    return ruta.read_text(encoding="utf-8")


def lineas_con(texto, aguja):
    """Lineas (1-based) que contienen la aguja, formateadas para el reporte.
    En Python seria una list comprehension sobre enumerate(splitlines())."""
    return [
        f"    linea {i}: {linea.strip()}"
        for i, linea in enumerate(texto.splitlines(), 1)
        if aguja in linea
    ]


def referencias_notebook_raiz(texto):
    """Lineas con un Untitled.ipynb que no cuelga de docs/legacy/. En Python
    seria una list comprehension que perdona el prefijo valido con replace."""
    return [
        f"    linea {i}: {linea.strip()}"
        for i, linea in enumerate(texto.splitlines(), 1)
        if "Untitled.ipynb" in linea.replace(RUTA_NOTEBOOK_NUEVA, "")
    ]


def verificar_ejemplos_smoke_run():
    """Seam 1 (criterio 1): ejemplos smoke-run apuntan a ./build/ode-solver.
    Devuelve la lista de errores (vacia si todo bien)."""
    errores = []
    for ruta in ARCHIVOS_SMOKE_RUN:
        rel = ruta.relative_to(REPO_ROOT)
        texto = leer_doc(errores, ruta)
        if texto is None:
            continue
        if "./main.exe" in texto:
            errores.append(
                f"{rel} aun menciona ./main.exe:\n"
                + "\n".join(lineas_con(texto, "./main.exe"))
            )
        if "./build/ode-solver" not in texto:
            errores.append(
                f"{rel} no tiene ningun ejemplo con ./build/ode-solver"
            )
    return errores


def verificar_ruta_notebook():
    """Seam 2 (criterio 2): el comando del notebook usa la ruta nueva.
    Devuelve la lista de errores (vacia si todo bien)."""
    errores = []
    rel = DOC_NOTEBOOK.relative_to(REPO_ROOT)
    texto = leer_doc(errores, DOC_NOTEBOOK)
    if texto is None:
        return errores

    if RUTA_NOTEBOOK_NUEVA not in texto:
        errores.append(f"{rel} no referencia {RUTA_NOTEBOOK_NUEVA}")

    raices = referencias_notebook_raiz(texto)
    if raices:
        errores.append(
            f"{rel} aun referencia un Untitled.ipynb raiz:\n"
            + "\n".join(raices)
        )
    return errores


def verificar_evidencia_historica():
    """Seam 3 (criterio 3): la evidencia historica queda intacta.
    Devuelve la lista de errores (vacia si todo bien)."""
    errores = []
    rel = EVIDENCIA_HISTORICA.relative_to(REPO_ROOT)
    texto = leer_doc(errores, EVIDENCIA_HISTORICA)
    if texto is None:
        return errores

    if "./main.exe" not in texto:
        errores.append(
            f"{rel} ya no conserva los ejemplos pre-reorg ./main.exe "
            "(el registro historico no debe tocarse)"
        )
    if "./build/ode-solver" in texto:
        errores.append(
            f"{rel} fue reescrito al layout nuevo; debe quedar intacto"
        )
    return errores


def verificar_comandos_make_conservados():
    """Seam 4 (criterio 4): los comandos make de estos archivos no se alteraron.
    Devuelve la lista de errores (vacia si todo bien)."""
    errores = []
    for ruta, patron in COMANDOS_MAKE_CONSERVADOS:
        rel = ruta.relative_to(REPO_ROOT)
        texto = leer_doc(errores, ruta)
        if texto is None:
            continue
        if not patron.search(texto):
            errores.append(
                f"{rel} ya no conserva el comando make de #53 "
                f"(esperado: {patron.pattern!r})"
            )
    return errores


def main():
    errores = []
    errores.extend(verificar_ejemplos_smoke_run())
    errores.extend(verificar_ruta_notebook())
    errores.extend(verificar_evidencia_historica())
    errores.extend(verificar_comandos_make_conservados())

    if errores:
        for error in errores:
            print(f"check_issue_90: FAIL | {error}", file=sys.stderr)
        return 1

    print(
        "check_issue_90: PASS | los ejemplos de docs/*.md apuntan al layout "
        "nuevo y la evidencia historica queda intacta"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
