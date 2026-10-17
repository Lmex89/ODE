#!/usr/bin/env python3
"""Verifica el contrato del log de corridas reales de build/ode-solver (issue #44).

Comprueba cinco cosas sobre la salida completa de corridas cortas y
deterministicas (ODE_SEED=42, ODE_CASE=1):

1. Toda linea non-empty sale por los helpers log_info()/log_warn(): formato
   "[<t>s] [NIVEL] ..." documentado en parse_tree.cpp.
2. La ultima linea es la firma del autor emitida como evento estructurado
   ("signature | author=Lmex"), no un cout suelto.
3. Las claves que parsea scripts/ab_experiment.sh (seed, gen, generations,
   best, expr, vector, status, elapsed_cpu_s) siguen presentes.
4. La cabecera de_run_start informa el modo de reinicio efectivo (issue #126):
   restart_mode=legacy sin ODE_RESTART_MODE (el default) y restart_mode=adaptive
   con ODE_RESTART_MODE=adaptive.
5. La linea restart | informa trigger=, fraction=, kept= y replaced= (issue
   #127): el disparador de estancamiento con la fraccion efectiva y cuantos
   individuos sobrevivieron. En legacy la fraccion es ODE_RESTART_FRACTION
   (0.80) y en adaptive la del primer reinicio del episodio (0.40, antes de
   que la escalera de #128 suba). ODE_GEN=2 + ODE_STALE_LIMIT=1 producen
   exactamente un reinicio. Las cifras esperadas asumen los defaults
   (poblacion 100, fracciones 0.80/0.40): el checker aisla ODE_POP y las
   fracciones del entorno.

En Python seria el equivalente a un test de integracion de pytest que ejecuta
el entry point como subproceso y valida su stdout.
"""

import os
import re
import subprocess
import sys
from pathlib import Path


BINARIO = Path(__file__).resolve().parents[1] / "build" / "ode-solver"

# Forma de cada linea estructurada, segun el bloque de logging de
# parse_tree.cpp: "[  0.12s] [INFO ] evento | clave=valor ...". Los dos espacios
# finales del nivel ("INFO ") se conservan con el grupo (INFO|WARN).
LINEA_ESTRUCTURADA = re.compile(r"^\[\s*\d+\.\d+s\] \[(INFO |WARN )\] \S")

# Contrato con scripts/ab_experiment.sh: renombrar o perder una de estas claves
# rompe el awk del experimento A/B.
CLAVES_PARSEADAS = (
    "seed=",
    "gen=",
    "generations=",
    "best=",
    "expr=",
    "vector=",
    "status=",
    "elapsed_cpu_s=",
)

FIRMA_AUTOR = "signature | author=Lmex"


def correr_main(con_modo=None, extra_env=None):
    """Ejecuta build/ode-solver en su modo corto y determinista. En Python seria
    subprocess.run([...], env=os.environ | {...}, capture_output=True)."""
    # ODE_LOG=terminal (issue #114): desde #112 el solver escribe en
    # ode-solver.log por defecto, pero este checker valida el contrato sobre
    # stdout. Se fuerza el modo terminal aunque el entorno del usuario diga otra
    # cosa, porque la ultima clave de dict() gana.
    entorno = dict(os.environ, ODE_SEED="42", ODE_CASE="1", ODE_GEN="1",
                   ODE_LOG="terminal")
    # Para validar el default hay que garantizar que ODE_RESTART_MODE no venga
    # del entorno del usuario: None lo elimina y un string lo fija (issue #126).
    if con_modo is None:
        entorno.pop("ODE_RESTART_MODE", None)
    else:
        entorno["ODE_RESTART_MODE"] = con_modo
    # Las cifras esperadas de la linea restart | (80/20 legacy, 40/60 adaptive)
    # derivan del tamano de poblacion y de las fracciones por defecto: se
    # eliminan del entorno para que un ODE_POP/ODE_RESTART_FRACTION del shell
    # no cambie el conteo (issue #134, revision).
    for clave in ("ODE_POP", "ODE_RESTART_FRACTION", "ODE_RESTART_MIN_FRACTION"):
        entorno.pop(clave, None)
    # ODE_STALE_LIMIT=1 + ODE_GEN=2 (issue #127) hacen disparar exactamente un
    # reinicio deterministico: la gen 1 fija un record y la gen 2 se estanca y
    # reinicia (el primero del episodio; la escalera de #128 subiria la
    # fraccion en los siguientes, que no llegan a ocurrir).
    if extra_env:
        entorno.update(extra_env)
    return subprocess.run(
        [BINARIO],
        env=entorno,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def errores_del_log(lineas, modo_esperado, reinicio_esperado):
    """Devuelve la lista de incumplimientos del contrato (vacia si todo bien).

    `reinicio_esperado` es None si la corrida no debe reiniciar, o la tupla
    (fraction, kept, replaced) que debe informar la linea restart | (issue
    #127)."""
    errores = []

    # 1. Ninguna linea suelta: todas pasan por los helpers.
    sueltas = [linea for linea in lineas if not LINEA_ESTRUCTURADA.match(linea)]
    if sueltas:
        errores.append("lineas fuera del formato estructurado [t] [NIVEL] ...:")
        errores.extend(f"    {linea!r}" for linea in sueltas)

    # 2. La firma del autor cierra el log como evento estructurado.
    if not lineas:
        errores.append("la corrida no produjo ninguna linea de log")
    elif not lineas[-1].endswith(FIRMA_AUTOR):
        errores.append(
            f"la ultima linea no es la firma estructurada {FIRMA_AUTOR!r}: "
            f"{lineas[-1]!r}"
        )

    # 3. El contrato de claves del script A/B no cambia.
    texto = "\n".join(lineas)
    faltantes = [clave for clave in CLAVES_PARSEADAS if clave not in texto]
    if faltantes:
        errores.append(f"faltan claves parseadas por el script A/B: {faltantes}")

    # 4. La cabecera informa el modo de reinicio efectivo (issue #126).
    cabecera = next((linea for linea in lineas if "de_run_start |" in linea), None)
    if cabecera is None:
        errores.append("la corrida no tiene cabecera de_run_start")
    elif f"restart_mode={modo_esperado}" not in cabecera:
        errores.append(
            f"la cabecera no informa restart_mode={modo_esperado}: {cabecera!r}"
        )

    # 5. La linea restart | informa el disparador y las cifras efectivas
    # (issue #127): trigger=stale con la fraccion aplicada, cuantos
    # sobrevivieron (kept) y cuantos se reemplazaron (replaced).
    if reinicio_esperado is not None:
        fraccion, kept, replaced = reinicio_esperado
        reinicios = [linea for linea in lineas if "restart |" in linea]
        if not reinicios:
            errores.append("la corrida debia reiniciar y no hay linea restart |")
        else:
            linea = reinicios[-1]
            for campo in ("trigger=stale", f"fraction={fraccion}",
                          f"kept={kept}", f"replaced={replaced}"):
                if campo not in linea:
                    errores.append(
                        f"restart | no informa {campo}: {linea!r}"
                    )

    return errores


def main():
    # Dos corridas (issue #126/#127): sin ODE_RESTART_MODE (default legacy) y
    # con ODE_RESTART_MODE=adaptive. Ambas fuerzan un unico reinicio
    # deterministico (ODE_STALE_LIMIT=1, ODE_GEN=2) y deben informar su modo
    # en la cabecera y las cifras del reinicio en la linea restart |: legacy
    # conserva su fraccion 0.80 y adaptive usa el primer escalon de la escalera
    # (0.40).
    errores = []
    reinicio_env = {"ODE_STALE_LIMIT": "1", "ODE_GEN": "2"}
    for nombre, con_modo, modo_esperado, reinicio_esperado in (
        ("default (legacy)", None, "legacy", ("0.80", "20", "80")),
        ("adaptive", "adaptive", "adaptive", ("0.40", "60", "40")),
    ):
        try:
            proc = correr_main(con_modo, reinicio_env)
        except subprocess.TimeoutExpired:
            print("check_run_log: build/ode-solver excedio los 60s", file=sys.stderr)
            return 1

        if proc.returncode != 0:
            print(f"check_run_log: build/ode-solver salio con codigo {proc.returncode}",
                  file=sys.stderr)
            return 1

        lineas = [linea for linea in proc.stdout.splitlines() if linea.strip()]
        errores.extend(errores_del_log(lineas, modo_esperado, reinicio_esperado))
        print(f"check_run_log: corrida {nombre}: {len(lineas)} lineas")

    if errores:
        for error in errores:
            print(f"check_run_log: FAIL | {error}", file=sys.stderr)
        return 1

    print("check_run_log: PASS | formato, firma del autor, claves del A/B, "
          "restart_mode y restart | intactos")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
