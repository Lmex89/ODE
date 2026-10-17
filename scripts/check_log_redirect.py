#!/usr/bin/env python3
"""Verifica el destino de la salida de build/ode-solver (issue #111).

Sin ODE_LOG, el solver escribe todo su log a `ode-solver.log` en el
directorio de trabajo (truncado en cada corrida) y no imprime nada en stdout.
Con ODE_LOG=terminal la salida vuelve a stdout y no se crea ningun archivo.
Si ODE_LOG_FILE apunta a una ruta que no se puede abrir, la corrida cae a
terminal con un WARN emitido antes de cualquier otra linea, para no perder
el log.

En Python seria el equivalente a un test de integracion de pytest que ejecuta
el entry point como subproceso en un directorio temporal y observa QUE
escribio y DONDE, sin mirar objetos internos del programa (la resolucion pura
del modo se prueba aparte, en tests/test_log_mode.cpp).
"""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


BINARIO = Path(__file__).resolve().parents[1] / "build" / "ode-solver"

# Corrida corta y determinista: la misma que usa check_run_log.py.
ENTORNO_CORTO = {"ODE_SEED": "42", "ODE_CASE": "1", "ODE_GEN": "1"}

FIRMA_AUTOR = "signature | author=Lmex"


def correr(extra, cwd):
    """Ejecuta build/ode-solver en `cwd` con el entorno corto + `extra`.

    ODE_LOG y ODE_LOG_FILE heredadas se eliminan a proposito: si la shell del
    usuario las exporta, contaminarian los escenarios (el equivalente a armar
    el entorno limpio de un test con `subprocess.run(env={...})`).
    """
    entorno = dict(os.environ)
    entorno.pop("ODE_LOG", None)
    entorno.pop("ODE_LOG_FILE", None)
    entorno.update(ENTORNO_CORTO)
    entorno.update(extra)
    return subprocess.run(
        [BINARIO],
        cwd=cwd,
        env=entorno,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def verificar_defecto():
    """Sin ODE_LOG: el log completo queda en ode-solver.log y stdout vacio."""
    errores = []
    with tempfile.TemporaryDirectory(prefix="check_log_redirect_") as tmp:
        proc = correr({}, tmp)
        if proc.returncode != 0:
            return [f"corrida por defecto salio con codigo {proc.returncode}"]

        archivo = Path(tmp) / "ode-solver.log"
        if not archivo.exists():
            errores.append("no se creo ode-solver.log en el directorio de trabajo")
        else:
            texto = archivo.read_text()
            if not texto.strip():
                errores.append("ode-solver.log quedo vacio")
            elif FIRMA_AUTOR not in texto:
                errores.append(
                    "ode-solver.log no contiene la corrida completa "
                    f"(falta {FIRMA_AUTOR!r})"
                )
        if proc.stdout.strip():
            errores.append("la salida por defecto no deberia escribir en stdout")
    return errores


def verificar_terminal():
    """ODE_LOG=terminal: stdout con el log completo y ningun archivo."""
    errores = []
    with tempfile.TemporaryDirectory(prefix="check_log_redirect_") as tmp:
        proc = correr({"ODE_LOG": "terminal"}, tmp)
        if proc.returncode != 0:
            return [f"corrida en terminal salio con codigo {proc.returncode}"]

        if FIRMA_AUTOR not in proc.stdout:
            errores.append("stdout no contiene la corrida completa (falta la firma)")
        if (Path(tmp) / "ode-solver.log").exists():
            errores.append("ODE_LOG=terminal no debe crear ode-solver.log")
    return errores


def verificar_fallo_de_archivo():
    """Ruta inabrible: WARN antes que nada y la corrida sigue por terminal."""
    errores = []
    with tempfile.TemporaryDirectory(prefix="check_log_redirect_") as tmp:
        # El directorio padre no existe, asi que apuntar ODE_LOG_FILE ahi
        # hace fallar la apertura: es la ruta mala tipica sin depender de
        # permisos ni de la plataforma.
        ruta = Path(tmp) / "sin_directorio" / "ode-solver.log"
        proc = correr({"ODE_LOG_FILE": str(ruta)}, tmp)
        if proc.returncode != 0:
            return [f"corrida con ruta inabrible salio con codigo {proc.returncode}"]

        if FIRMA_AUTOR not in proc.stdout:
            errores.append("la corrida no cayo a terminal (falta la firma en stdout)")

        lineas = [linea for linea in proc.stdout.splitlines() if linea.strip()]
        if not lineas:
            errores.append("la corrida con ruta inabrible no produjo salida")
        else:
            primera = lineas[0]
            if "[WARN ]" not in primera:
                errores.append(f"la primera linea no es el aviso WARN: {primera!r}")
            if "log_file_unavailable" not in primera:
                errores.append(f"la primera linea no es el aviso esperado: {primera!r}")
            if str(ruta) not in primera:
                errores.append(f"el aviso no menciona la ruta pedida: {primera!r}")
        if ruta.exists():
            errores.append("no deberia existir el archivo en una ruta inabrible")
    return errores


def main():
    try:
        errores = []
        errores.extend(verificar_defecto())
        errores.extend(verificar_terminal())
        errores.extend(verificar_fallo_de_archivo())
    except subprocess.TimeoutExpired:
        print("check_log_redirect: build/ode-solver excedio los 60s", file=sys.stderr)
        return 1

    if errores:
        for error in errores:
            print(f"check_log_redirect: FAIL | {error}", file=sys.stderr)
        return 1

    print(
        "check_log_redirect: PASS | por defecto escribe ode-solver.log, "
        "ODE_LOG=terminal usa stdout y una ruta inabrible cae a terminal"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
