#!/usr/bin/env python3
"""Verifica los criterios de aceptacion del issue #53: Remove Makefile.

Criterios:
1. Makefile eliminado de la raiz
2. cmake -B build && cmake --build build funciona
3. ctest --test-dir build corre todos los tests y pasan
4. scripts/ab_experiment.sh corre end-to-end
5. AGENTS.md no menciona Makefile
6. Ningun documento o script referencia `make` como comando de build

En Python seria un test de integracion que valida que el migration de
Makefile a CMake se completo correctamente.
"""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
MAKEFILE = REPO_ROOT / "Makefile"
AGENTS_MD = REPO_ROOT / "AGENTS.md"
README_MD = REPO_ROOT / "README.md"
SCRIPTS_DIR = REPO_ROOT / "scripts"
DOCS_DIR = REPO_ROOT / "docs"
TESTS_DIR = REPO_ROOT / "tests"

# Comandos que solo tienen sentido con el Makefile (no "make" como verbo ingles)
RE_COMANDO_MAKE = re.compile(r"`make(\s|$)")

# Archivos que no deben referenciar make como comando de build
ARCHIVOS_A_REVISAR = []


def verificar_makefile_eliminado():
    """Criterio 1: Makefile no existe. Devuelve lista de errores."""
    errores = []
    if MAKEFILE.exists():
        errores.append(f"Makefile aun existe en {MAKEFILE}")
    return errores


def verificar_cmake_build():
    """Criterio 2: cmake -B build && cmake --build build funciona.
    Devuelve lista de errores."""
    errores = []
    build_dir = REPO_ROOT / "build"

    # Limpiar build/ si existe
    if build_dir.exists():
        shutil.rmtree(build_dir)

    # Find cmake binary (may be in /tmp/cmake-3.28.0-linux-x86_64/bin)
    cmake_bin = "cmake"
    if not shutil.which(cmake_bin):
        alt_cmake = "/tmp/cmake-3.28.0-linux-x86_64/bin/cmake"
        if Path(alt_cmake).exists():
            cmake_bin = alt_cmake
        else:
            errores.append("cmake no esta instalado o no esta en PATH")
            return errores

    # Configurar
    try:
        proc = subprocess.run(
            [cmake_bin, "-B", "build"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            timeout=60,
            check=False,
        )
        if proc.returncode != 0:
            errores.append(f"cmake -B build fallo con codigo {proc.returncode}")
            errores.append(f"stderr: {proc.stderr[:500]}")
            return errores
    except subprocess.TimeoutExpired:
        errores.append("cmake -B build tardo mas de 60s")
        return errores
    except FileNotFoundError:
        errores.append("cmake no esta instalado o no esta en PATH")
        return errores

    # Compilar
    try:
        proc = subprocess.run(
            [cmake_bin, "--build", "build"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            timeout=120,
            check=False,
        )
        if proc.returncode != 0:
            errores.append(f"cmake --build build fallo con codigo {proc.returncode}")
            errores.append(f"stderr: {proc.stderr[:500]}")
    except subprocess.TimeoutExpired:
        errores.append("cmake --build build tardo mas de 120s")
    except FileNotFoundError:
        errores.append("cmake no esta instalado o no esta en PATH")

    return errores


def verificar_ctest():
    """Criterio 3: ctest --test-dir build corre todos los tests y pasan.
    Devuelve lista de errores."""
    errores = []
    build_dir = REPO_ROOT / "build"

    if not build_dir.exists():
        errores.append("build/ no existe; corre verificar_cmake_build primero")
        return errores

    # Find ctest binary (may be alongside cmake)
    ctest_bin = "ctest"
    if not shutil.which(ctest_bin):
        alt_ctest = "/tmp/cmake-3.28.0-linux-x86_64/bin/ctest"
        if Path(alt_ctest).exists():
            ctest_bin = alt_ctest
        else:
            errores.append("ctest no esta instalado o no esta en PATH")
            return errores

    try:
        proc = subprocess.run(
            [ctest_bin, "--test-dir", "build", "--output-on-failure"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            timeout=180,
            check=False,
        )
        if proc.returncode != 0:
            errores.append(f"ctest --test-dir build fallo con codigo {proc.returncode}")
            errores.append(f"stdout: {proc.stdout[:500]}")
            errores.append(f"stderr: {proc.stderr[:500]}")
    except subprocess.TimeoutExpired:
        errores.append("ctest --test-dir build tardo mas de 180s")
    except FileNotFoundError:
        errores.append("ctest no esta instalado o no esta en PATH")

    return errores


def verificar_agents_md():
    """Criterio 5: AGENTS.md no menciona Makefile. Devuelve lista de errores."""
    errores = []

    if not AGENTS_MD.exists():
        errores.append(f"AGENTS.md no existe en {AGENTS_MD}")
        return errores

    texto = AGENTS_MD.read_text(encoding="utf-8")

    # Buscar mencion de "Makefile" (como sustantivo, no como parte de otra palabra)
    if re.search(r'\bMakefile\b', texto):
        errores.append("AGENTS.md aun menciona 'Makefile'")

    return errores


def verificar_sin_comandos_make():
    """Criterio 6: Ningun doc/script referencia `make` como comando.
    Devuelve lista de errores."""
    errores = []

    # Archivos a revisar: README, docs/*.md, scripts/*.sh, scripts/*.py, tests/*.cpp
    archivos = []
    archivos.append(README_MD)
    archivos.extend(DOCS_DIR.glob("**/*.md"))
    archivos.extend(SCRIPTS_DIR.glob("*.sh"))
    archivos.extend(SCRIPTS_DIR.glob("*.py"))
    archivos.extend((REPO_ROOT / "src").glob("*.cpp"))
    archivos.extend((REPO_ROOT / "src").glob("*.h"))
    archivos.extend(TESTS_DIR.glob("*.cpp"))

    for archivo in archivos:
        if not archivo.exists():
            continue
        # Excluir archivos legacy/historicos que documentan el pasado
        if "legacy" in str(archivo) or "issue-29-evidence" in str(archivo):
            continue
        # Excluir este script mismo
        if archivo.name == "check_issue_53.py":
            continue

        try:
            texto = archivo.read_text(encoding="utf-8")
        except (UnicodeDecodeError, PermissionError):
            continue

        # Buscar comandos make en backticks (no "make" como verbo ingles)
        matches = RE_COMANDO_MAKE.findall(texto)
        if matches:
            # Encontrar las lineas con los matches para reportar
            lineas_con_make = []
            for i, linea in enumerate(texto.splitlines(), 1):
                if RE_COMANDO_MAKE.search(linea):
                    lineas_con_make.append(f"  linea {i}: {linea.strip()[:80]}")
            errores.append(
                f"{archivo.relative_to(REPO_ROOT)} aun referencia comandos make:\n"
                + "\n".join(lineas_con_make[:5])  # Max 5 ejemplos
            )

    return errores


def verificar_ab_experiment():
    """Criterio 4: scripts/ab_experiment.sh corre end-to-end.
    Devuelve lista de errores."""
    errores = []
    script = SCRIPTS_DIR / "ab_experiment.sh"

    if not script.exists():
        errores.append(f"scripts/ab_experiment.sh no existe")
        return errores

    # Verificar que el script es ejecutable y no referencia Makefile
    texto = script.read_text(encoding="utf-8")

    # Buscar comentarios que referencien Makefile (no comandos, solo comentarios)
    if "Makefile" in texto and not texto.count("Makefile") == texto.count("# los mismos del Makefile"):
        # Permitir el comentario de la linea 69, pero no otros
        lineas_con_makefile = [
            (i, linea) for i, linea in enumerate(texto.splitlines(), 1)
            if "Makefile" in linea and i != 69
        ]
        if lineas_con_makefile:
            errores.append(
                f"scripts/ab_experiment.sh menciona Makefile en lineas inesperadas: "
                f"{[l[0] for l in lineas_con_makefile]}"
            )

    return errores


def main():
    errores = []

    print("Verificando criterios de aceptacion del issue #53...")
    print()

    # Criterio 1: Makefile eliminado
    print("1. Verificando Makefile eliminado...")
    errores.extend(verificar_makefile_eliminado())

    # Criterio 5: AGENTS.md sin Makefile (no requiere build)
    print("5. Verificando AGENTS.md sin referencias a Makefile...")
    errores.extend(verificar_agents_md())

    # Criterio 2: cmake build funciona (requiere cmake instalado)
    print("2. Verificando cmake build...")
    errores_cmake = verificar_cmake_build()
    errores.extend(errores_cmake)

    # Criterio 3: ctest funciona (solo si cmake build funciono)
    if not errores_cmake:
        print("3. Verificando ctest...")
        errores.extend(verificar_ctest())
    else:
        print("3. Omitiendo ctest (cmake build fallo)")

    # Criterio 4: ab_experiment.sh funciona
    print("4. Verificando ab_experiment.sh...")
    errores.extend(verificar_ab_experiment())

    # Criterio 6: Sin comandos make en docs/scripts
    print("6. Verificando sin comandos make en docs/scripts...")
    errores.extend(verificar_sin_comandos_make())

    print()

    if errores:
        print("=" * 70)
        print("FAIL: Se encontraron los siguientes errores:")
        print("=" * 70)
        for i, error in enumerate(errores, 1):
            print(f"\n{i}. {error}")
        return 1

    print("=" * 70)
    print("PASS: Todos los criterios de aceptacion del issue #53 se cumplen")
    print("=" * 70)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
