#!/usr/bin/env python3
"""Verifica el destino de la salida del solver de punta a punta (issue #113).

Ejecuta build/ode-solver como subproceso en dos directorios temporales
distintos, con una corrida corta y deterministica y sin ODE_LOG/ODE_LOG_FILE
heredados del shell:

1. Sin variables de salida: la corrida crea `ode-solver.log` (no vacio) en su
   directorio de trabajo y deja stdout vacio.
2. `ODE_LOG=terminal`: la corrida imprime el log completo en stdout y no crea
   ningun archivo.

Es el complemento de proceso de tests/test_salida.cpp (issue #112): alli se
prueba el resolver puro; aqui, el ejecutable real. En Python seria un test de
integracion de pytest que corre el entry point y observa archivos y stdout.
"""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


BINARIO = Path(__file__).resolve().parents[1] / "build" / "ode-solver"

# Nombre del log por defecto (contrato de resolver_salida, issue #112): el
# literal es la especificacion, no se importa del codigo C++.
ARCHIVO_POR_DEFECTO = "ode-solver.log"

# Corrida corta y determinista, igual que check_run_log.py.
ENTORNO_CORRIDA = {"ODE_SEED": "42", "ODE_CASE": "1", "ODE_GEN": "1"}

TIMEOUT = 60


def entorno_limpio():
    """Copia os.environ sin las variables de salida, para que el modo por
    defecto no dependa del shell que invoca el checker. En Python seria
    `dict(os.environ) | corrida`; pop() sobre la copia no muta os.environ."""
    entorno = dict(os.environ)
    entorno.pop("ODE_LOG", None)
    entorno.pop("ODE_LOG_FILE", None)
    entorno.update(ENTORNO_CORRIDA)
    return entorno


def correr_en(directorio, variables_salida):
    """Corre el solver con cwd=directorio. En Python seria
    subprocess.run([BINARIO], cwd=..., env=..., capture_output=True)."""
    entorno = entorno_limpio()
    entorno.update(variables_salida)
    return subprocess.run(
        [BINARIO],
        cwd=directorio,
        env=entorno,
        capture_output=True,
        text=True,
        timeout=TIMEOUT,
        check=False,
    )


def caso_archivo_por_defecto():
    """Caso 1: sin variables, el log va al archivo y stdout queda vacio.

    Devuelve la lista de incumplimientos (vacia si todo bien)."""
    errores = []
    with tempfile.TemporaryDirectory(prefix="ode-log-archivo-") as temporal:
        corrida = correr_en(temporal, {})
        if corrida.returncode != 0:
            errores.append(f"el solver salio con codigo {corrida.returncode}")

        archivo = Path(temporal) / ARCHIVO_POR_DEFECTO
        if not archivo.exists():
            errores.append(f"no se creo {ARCHIVO_POR_DEFECTO}")
        elif archivo.stat().st_size == 0:
            errores.append(f"{ARCHIVO_POR_DEFECTO} quedo vacio")

        if corrida.stdout:
            primeras = corrida.stdout.splitlines()[:2]
            errores.append(
                f"stdout deberia estar vacio en modo archivo: {primeras!r}"
            )
    return errores


def caso_terminal():
    """Caso 2: ODE_LOG=terminal imprime en stdout y no crea archivos."""
    errores = []
    with tempfile.TemporaryDirectory(prefix="ode-log-terminal-") as temporal:
        corrida = correr_en(temporal, {"ODE_LOG": "terminal"})
        if corrida.returncode != 0:
            errores.append(f"el solver salio con codigo {corrida.returncode}")

        if not corrida.stdout.strip():
            errores.append("stdout vacio con ODE_LOG=terminal")

        creados = sorted(ruta.name for ruta in Path(temporal).iterdir())
        if creados:
            errores.append(
                f"ODE_LOG=terminal no deberia crear archivos: {creados}"
            )
    return errores


def main():
    if not BINARIO.exists():
        print(f"check_log_redirect: FAIL | falta {BINARIO}", file=sys.stderr)
        return 1

    casos = (
        ("sin ODE_LOG (archivo por defecto)", caso_archivo_por_defecto),
        ("ODE_LOG=terminal", caso_terminal),
    )

    fallos = 0
    for nombre, caso in casos:
        try:
            errores = caso()
        except subprocess.TimeoutExpired:
            errores = [f"el solver excedio los {TIMEOUT}s"]
        except FileNotFoundError:
            errores = [f"no se encontro {BINARIO}"]

        if errores:
            fallos += 1
            print(f"check_log_redirect: FAIL | {nombre}", file=sys.stderr)
            for error in errores:
                print(f"    {error}", file=sys.stderr)
        else:
            print(f"check_log_redirect: PASS | {nombre}")

    if fallos:
        print(
            f"check_log_redirect: FAIL | {fallos}/{len(casos)} casos",
            file=sys.stderr,
        )
        return 1

    print(
        f"check_log_redirect: PASS | {len(casos)}/{len(casos)} casos "
        f"(archivo por defecto y terminal)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
