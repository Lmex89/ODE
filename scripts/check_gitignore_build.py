#!/usr/bin/env python3
"""Verifica el contrato de .gitignore con el layout CMake (issue #49).

Despues de #48, el build va a build/ (out-of-source) y los artefactos de
compilacion (*.o, *.exe) viven dentro de ese directorio. .gitignore debe
ignorar build/ en lugar de listar cada patron individual.

Dos seams de verificacion:

1. Contenido estatico: .gitignore no lista *.o, *.exe, main.exe ni los
   test_*.exe enumerados; tiene build/; preserva los ignores existentes
   (.codegraph/, .vscode/, .ipynb_checkpoints/, out, out.txt, .inteopt.txt).

2. Comportamiento observable: la regla build/ en .gitignore hace que git
   ignore todos los archivos dentro de ese directorio (verificado con
   git status --porcelain).

En Python seria el equivalente a un test de integracion que valida que git
ignore funcione correctamente sin necesidad de ejecutar el build real.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
GITIGNORE = REPO_ROOT / ".gitignore"

# Patrones que NO deben estar (se eliminaron en favor de build/)
PROHIBIDOS = (
    "*.o",
    "*.exe",
    "main.exe",
    "test_dual.exe",
    "test_mutation.exe",
    "test_semilla.exe",
    "test_crossover.exe",
    "test_grid.exe",
    "test_population.exe",
    "test_token.exe",
    "test_funbobj_const.exe",
    "test_header_hygiene.exe",
)

# Patron que DEBE estar (nuevo layout CMake)
REQUERIDO = "build/"

# Patrones que DEBEN preservarse (herramientas, outputs, etc.)
PRESERVADOS = (
    ".codegraph/",
    ".vscode/",
    ".ipynb_checkpoints/",
    "out",
    "out.txt",
    ".inteopt.txt",
)


def contenido_gitignore():
    """Lee .gitignore y devuelve lineas no-vacias ni comentarios."""
    if not GITIGNORE.exists():
        return []
    lineas = GITIGNORE.read_text().splitlines()
    return [
        linea.strip()
        for linea in lineas
        if linea.strip() and not linea.strip().startswith("#")
    ]


def verificar_contenido():
    """Seam 1: contenido estatico de .gitignore. Devuelve lista de errores."""
    errores = []
    contenido = contenido_gitignore()

    # 1. Patrones prohibidos no deben estar
    for patron in PROHIBIDOS:
        if patron in contenido:
            errores.append(f"gitignore aun contiene patron prohibido: {patron!r}")

    # 2. build/ debe estar
    if REQUERIDO not in contenido:
        errores.append(f"gitignore no contiene el patron requerido: {REQUERIDO!r}")

    # 3. Patrones preservados deben seguir estando
    for patron in PRESERVADOS:
        if patron not in contenido:
            errores.append(f"gitignore perdio patron preservado: {patron!r}")

    return errores


def verificar_comportamiento():
    """Seam 2: git ignora build/. Crea build/ con artefactos simulados y verifica
    que git status --porcelain no los muestre. Devuelve lista de errores."""
    errores = []
    build_dir = REPO_ROOT / "build"

    # Crear build/ temporal con artefactos simulados
    try:
        build_dir.mkdir(exist_ok=True)
        (build_dir / "test.o").touch()
        (build_dir / "test.exe").touch()
        (build_dir / "ode-solver").touch()
        (build_dir / "CMakeCache.txt").touch()

        # Verificar que git status --porcelain no muestre build/
        proc = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=True,
        )
        lineas_build = [
            linea for linea in proc.stdout.splitlines()
            if linea.strip() and "build/" in linea
        ]
        if lineas_build:
            errores.append(
                f"git status muestra {len(lineas_build)} entradas de build/ "
                "(deberian estar ignoradas):"
            )
            errores.extend(f"    {linea}" for linea in lineas_build[:10])

        # Verificar con git check-ignore que los archivos de build/ son ignorados
        proc_check = subprocess.run(
            ["git", "check-ignore", "build/test.o", "build/ode-solver"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,  # check-ignore devuelve 1 si no hay coincidencias
        )
        if proc_check.returncode != 0:
            errores.append(
                "git check-ignore no ignora los archivos de build/ "
                "(la regla build/ no funciona)"
            )

    finally:
        # Limpiar build/ temporal
        if build_dir.exists():
            shutil.rmtree(build_dir)

    return errores


def main():
    errores = []

    errores_contenido = verificar_contenido()
    errores.extend(errores_contenido)

    # Solo verificar comportamiento si el contenido esta bien
    if not errores_contenido:
        errores_comportamiento = verificar_comportamiento()
        errores.extend(errores_comportamiento)

    if errores:
        for error in errores:
            print(f"check_gitignore_build: FAIL | {error}", file=sys.stderr)
        return 1

    print(
        "check_gitignore_build: PASS | .gitignore ignora build/ correctamente "
        "y git status no muestra artefactos de build/"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
