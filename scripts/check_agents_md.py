#!/usr/bin/env python3
"""Verifica el contrato de AGENTS.md con el layout CMake (issue #52), la
salida a log (issue #116) y las claves del reinicio adaptativo (issue #130).

Despues de #48-#51, los fuentes viven en src/, los tests en tests/, los
archivos historicos en docs/legacy/ y el ejecutable es build/ode-solver.
AGENTS.md debe describir ese flujo (cmake/ctest) y no el layout plano
anterior (Makefile, main.exe, test_*.exe en la raiz). Desde #112 el solver
escribe en ode-solver.log por defecto, asi que los ejemplos que canalizan
stdout tambien deben pedir ODE_LOG=terminal.

Cinco seams de verificacion, siempre contra fuentes de verdad externas al
propio documento:

1. Contenido estatico: sin menciones a main.exe ni comandos `make`, con el
   flujo CMake documentado (cmake -B build, cmake --build build,
   ctest --test-dir build, build/ode-solver), las variables de salida
   ODE_LOG/ODE_LOG_FILE y el A/B de reinicios (scripts/restart_ab.sh y su
   checker).

2. Contratos preservados: las secciones y reglas que no debian cambiar
   (navegacion con codegraph, logging estructurado, Conventional Commits).

3. Realidad del repo: cada ruta citada existe en el arbol, y lo que el
   documento afirma del build concuerda con CMakeLists.txt (target
   ode-solver y numero de tests).

4. Ejemplos de corrida: una linea que canaliza build/ode-solver con `|` debe
   forzar ODE_LOG=terminal; en modo archivo (el default) stdout queda vacio
   y el pipe no recibiria nada.

5. Claves del reinicio adaptativo (issue #125/#130): las cinco variables
   nuevas (mas ODE_RESTART_FRACTION, que pasa a ser el tope) y sus defaults
   deben estar documentadas, cada una en la linea que el documento le dedica.

En Python seria el equivalente a un test de contrato: valida que la guia
de agentes y el repo real no se hayan desincronizado, sin ejecutar el
build completo.
"""

import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
AGENTS_MD = REPO_ROOT / "AGENTS.md"
CMAKELISTS = REPO_ROOT / "CMakeLists.txt"

# Subcadenas que NO deben aparecer: nombres y artefactos del layout plano.
PROHIBIDOS = (
    "main.exe",
    "TESTBINS",
    "test_*.exe",
)

# Flujo y rutas que DEBEN aparecer (estado posterior a #48-#51).
REQUERIDOS = (
    "cmake -B build",
    "cmake --build build",
    "ctest --test-dir build",
    "build/ode-solver",
    "src/",
    "tests/",
    "docs/legacy/",
    "ODE_LOG=terminal",
    "ODE_LOG_FILE",
    "ode-solver.log",
    "scripts/check_log_redirect.py",
    "scripts/check_restart_escalation.py",
    "scripts/restart_ab.sh",
    "scripts/check_restart_ab.py",
)

# Contratos que DEBEN preservarse: si falta alguno, el documento perdio
# secciones o reglas que el issue pide mantener (comentarios, logging,
# convenciones de commits, navegacion de agentes).
PRESERVADOS = (
    "## Code navigation",
    "## Build, test, and run",
    "## Architecture and invariants",
    "## C++ and output conventions",
    "## Commits and issues",
    "docs/cpp-standards.md",
    "Conventional Commits",
    "log_info()",
    "log_warn()",
    "[<cpu>s] [LEVEL]",
    "author=Lmex",
    "GLOSSARY.md",
    "ODE_NO_MAIN",
    "random_token()",
)

# Comandos que solo tienen sentido con el Makefile: spans en backticks que
# empiezan por `make` (make, make test, make CXX=icpc, ...). `Makefile` no
# coincide: tras "make" haria falta espacio o fin del span.
RE_COMANDO_MAKE = re.compile(r"make(\s|$)")

# Rutas citadas: src/, tests/, docs/ o scripts/ seguidas de un token de
# ruta (letras, digitos, . / _ - y las llaves de la notacion {h,cpp}).
RE_RUTA = re.compile(r"(?<![\w/])((?:src|tests|docs|scripts)/[\w./{},-]+)")

# Numero de tests que el documento afirma, p. ej. "9 C++ tests".
RE_TESTS_DOC = re.compile(r"(\d+)\s+C\+\+ tests?")

RE_BACKTICKS = re.compile(r"`([^`]+)`")

# Ejemplo de corrida que canaliza stdout: "./build/ode-solver | tail -30".
# Desde #112 el default escribe en ode-solver.log y deja stdout vacio, asi que
# la linea debe forzar ODE_LOG=terminal.
RE_EJEMPLO_PIPE = re.compile(r"build/ode-solver[^|\n]*\|")

# Claves del reinicio adaptativo (issues #125/#130) y su default. El valor por
# defecto es el que resuelve resolver_config (src/parse_tree.cpp) cuando la
# variable falta o es invalida; el documento debe nombrar cada clave y su
# default en la misma linea (estilo "`ODE_RESTART_MIN_FRACTION` (0.4) ...").
CLAVES_REINICIO_ADAPTATIVO = (
    ("ODE_RESTART_MODE", "legacy"),
    ("ODE_RESTART_MIN_FRACTION", "0.4"),
    ("ODE_RESTART_FRACTION", "0.8"),
    ("ODE_RESTART_CAP", "3"),
    ("ODE_RESTART_MUTATION_SHARE", "0.5"),
    ("ODE_DIV_THRESHOLD", "0.05"),
)


def leer_texto(path):
    """Devuelve el contenido de path, o "" si no existe (se reporta luego)."""
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def expandir_llaves(ruta):
    """Expande la notacion {a,b}: src/x.{h,cpp} -> [src/x.h, src/x.cpp].

    En Python seria una reescritura corta; aqui basta un solo grupo de
    llaves, que es el estilo del documento.
    """
    coincidencia = re.fullmatch(r"(.*)\{([^}]*)\}(.*)", ruta)
    if not coincidencia:
        return [ruta]
    prefijo, opciones, sufijo = coincidencia.groups()
    return [prefijo + opcion.strip() + sufijo for opcion in opciones.split(",")]


def verificar_contenido(texto):
    """Seam 1: contenido estatico de AGENTS.md. Devuelve lista de errores."""
    errores = []

    # 1. Nombres ni artefactos del layout plano.
    for patron in PROHIBIDOS:
        if patron in texto:
            errores.append(f"AGENTS.md aun menciona el layout plano: {patron!r}")

    # 2. Comandos make documentados dentro de backticks.
    for span in RE_BACKTICKS.findall(texto):
        if RE_COMANDO_MAKE.match(span.strip()):
            errores.append(f"AGENTS.md aun documenta un comando make: `{span}`")

    # 3. Flujo CMake y rutas nuevas presentes.
    for patron in REQUERIDOS:
        if patron not in texto:
            errores.append(f"AGENTS.md no contiene el elemento requerido: {patron!r}")

    # 4. Contratos preservados.
    for patron in PRESERVADOS:
        if patron not in texto:
            errores.append(f"AGENTS.md perdio el contrato preservado: {patron!r}")

    return errores


def verificar_ejemplos_salida(texto):
    """Seam 4: si un ejemplo canaliza stdout, debe pedir ODE_LOG=terminal.

    Se revisa linea a linea (no por spans de backticks) porque un backtick
    suelto en el documento desplaza el emparejamiento y ocultaria el ejemplo.
    En Python seria `for linea in texto.splitlines(): if "|" in linea ...`.
    """
    errores = []
    for linea in texto.splitlines():
        if RE_EJEMPLO_PIPE.search(linea) and "ODE_LOG=terminal" not in linea:
            errores.append(
                "AGENTS.md canaliza stdout sin ODE_LOG=terminal: "
                f"{linea.strip()!r}"
            )
    return errores


def verificar_rutas(texto):
    """Seam 3a: cada ruta citada (src/, tests/, docs/, scripts/) existe.

    build/ queda fuera del patron: es un artefacto que no vive en el repo.
    Las llaves {h,cpp} se expanden antes de comprobar.
    """
    errores = []
    vistas = set()
    for capturada in RE_RUTA.findall(texto):
        ruta = capturada.rstrip(".,;:)")
        for variante in expandir_llaves(ruta):
            if variante in vistas:
                continue
            vistas.add(variante)
            if "*" in variante:
                continue
            if not (REPO_ROOT / variante).exists():
                errores.append(f"AGENTS.md cita una ruta inexistente: {variante!r}")
    return errores


def verificar_cmake(texto):
    """Seam 3b: lo que el documento afirma del build concuerda con CMakeLists.

    El numero de tests es un acoplamiento deliberado: si alguien anade un
    test nuevo y no actualiza AGENTS.md, el contrato falla. Cada archivo
    tests/test_*.cpp debe estar registrado en CMakeLists.txt por nombre
    (los bucles foreach registran varios a la vez, por eso no basta contar
    llamadas add_test()).
    """
    errores = []
    cmake = leer_texto(CMAKELISTS)
    if not cmake:
        errores.append("no se encontro CMakeLists.txt")
        return errores

    if "add_executable(ode-solver" not in cmake:
        errores.append(
            "CMakeLists.txt no define el target ode-solver que documenta AGENTS.md"
        )

    archivos = sorted((REPO_ROOT / "tests").glob("test_*.cpp"))
    if not archivos:
        errores.append("no hay tests/test_*.cpp en el repo")
    for archivo in archivos:
        if archivo.stem not in cmake:
            errores.append(
                f"tests/{archivo.name} no esta registrado en CMakeLists.txt"
            )

    coincidencia = RE_TESTS_DOC.search(texto)
    if not coincidencia:
        errores.append('AGENTS.md no documenta el numero de tests ("N C++ tests")')
    elif int(coincidencia.group(1)) != len(archivos):
        errores.append(
            f"AGENTS.md declara {coincidencia.group(1)} tests C++ pero hay "
            f"{len(archivos)} archivos tests/test_*.cpp"
        )

    return errores


def verificar_claves_reinicio(texto):
    """Seam 5: cada clave del reinicio adaptativo con su default documentado.

    La guia debe nombrar las cinco claves del #125 (mas ODE_RESTART_FRACTION,
    que cambio de rol a tope) y el default en la misma linea; el estilo del
    documento es "`ODE_RESTART_MIN_FRACTION` (0.4) ...". En Python seria
    `any(clave in linea and default in linea for linea in lineas)`.
    """
    errores = []
    lineas = texto.splitlines()
    for clave, por_defecto in CLAVES_REINICIO_ADAPTATIVO:
        if not any(clave in linea and por_defecto in linea
                   for linea in lineas):
            errores.append(
                f"AGENTS.md no documenta {clave} con su default "
                f"{por_defecto!r} en la misma linea"
            )
    return errores


def main():
    texto = leer_texto(AGENTS_MD)
    if not texto:
        print("check_agents_md: FAIL | no se encontro AGENTS.md", file=sys.stderr)
        return 1

    errores = []
    errores.extend(verificar_contenido(texto))
    errores.extend(verificar_ejemplos_salida(texto))
    errores.extend(verificar_rutas(texto))
    errores.extend(verificar_cmake(texto))
    errores.extend(verificar_claves_reinicio(texto))

    if errores:
        for error in errores:
            print(f"check_agents_md: FAIL | {error}", file=sys.stderr)
        return 1

    print(
        "check_agents_md: PASS | AGENTS.md documenta el flujo CMake "
        "(build/ode-solver), la salida ODE_LOG/ODE_LOG_FILE y sus rutas existen"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
