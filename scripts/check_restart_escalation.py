#!/usr/bin/env python3
"""Verifica el cableado del escalado adaptativo en el bucle DE (issue #128).

`test_restart_escalation.cpp` cubre la logica pura de `EscaladorReinicio`.
Este checker cubre el otro extremo: ejecuta `build/ode-solver` en modo
`adaptive` con reinicios forzados (`ODE_STALE_LIMIT=1`) y valida la linea
`restart |` que emite el bucle real:

1. La secuencia de fracciones es 0.4 -> 0.6 -> 0.8 -> 0.2 y se mantiene en
   0.2 mientras no haya un `new_best`.
2. `replaced=` sigue a `fraction=` (la fraccion escalada se aplica de verdad,
   no solo se registra).
3. `mutation_share=` vale 0.5 bajo el tope y 1.0 en refinamiento.
4. Un `new_best` reinicia la escalera en el siguiente reinicio.

En Python seria un test de integracion de pytest que ejecuta el entry point
como subproceso y valida su stdout, el mismo patron que check_run_log.py.
"""

import os
import re
import subprocess
import sys
from pathlib import Path


BINARIO = Path(__file__).resolve().parents[1] / "build" / "ode-solver"

# Poblacion pequena para que la corrida sea rapida y las fracciones caigan en
# enteros (0.4/0.6/0.8/0.2 de 20 = 8/12/16/4).
POBLACION = 20
GENERACIONES = 40

# Forma de la linea adaptativa (issues #127/#128/#129). El trigger puede ser
# `stale` o `diversity`; ambos comparten la escalera que valida este checker.
RE_REINICIO = re.compile(
    r"restart \| gen=(\d+) stagnant_gens=(\d+) replaced=(\d+) "
    r"fraction=([0-9.]+) mutation_share=([0-9.]+) "
    r"trigger=(\w+) kept=(\d+)"
)
RE_NUEVO_RECORD = re.compile(r"new_best \|")

MIN_FRACCION = 0.4
MAX_FRACCION = 0.8
CAP = 3
CUOTA_HIBRIDA = 0.5
FRACCION_REFINAMIENTO = 0.2
CUOTA_REFINAMIENTO = 1.0


def correr_solver():
    """Ejecuta el solver en modo adaptive con reinicios en cada generacion.

    En Python seria `subprocess.run([...], env=os.environ | {...}, ...)`.
    Se fuerzan todos los knobs del escalado para que el resultado no dependa
    del entorno del usuario; `ODE_LOG=terminal` evita crear ode-solver.log y
    permite leer la salida por stdout.
    """
    entorno = dict(os.environ)
    entorno.update(
        ODE_LOG="terminal",
        ODE_SEED="42",
        ODE_CASE="0",
        ODE_GEN=str(GENERACIONES),
        ODE_POP=str(POBLACION),
        ODE_DIM="20",
        ODE_WARM_START="0",
        ODE_STALE_LIMIT="1",
        ODE_RESTART_MODE="adaptive",
        ODE_RESTART_MIN_FRACTION=str(MIN_FRACCION),
        ODE_RESTART_FRACTION=str(MAX_FRACCION),
        ODE_RESTART_CAP=str(CAP),
        ODE_RESTART_MUTATION_SHARE=str(CUOTA_HIBRIDA),
    )
    # ODE_LOG=terminal gana, pero eliminar ODE_LOG_FILE evita cualquier duda.
    entorno.pop("ODE_LOG_FILE", None)
    return subprocess.run(
        [BINARIO],
        env=entorno,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def parametros_esperados(nivel):
    """Devuelve (fraccion, cuota) del reinicio `nivel` (0-based).

    Replica la formula publica de EscaladorReinicio: por debajo del cap sube
    0.2 desde el minimo hasta el tope; al llegar al cap pasa a refinamiento.
    """
    if nivel >= CAP:
        return FRACCION_REFINAMIENTO, CUOTA_REFINAMIENTO
    return min(MAX_FRACCION, MIN_FRACCION + 0.2 * nivel), CUOTA_HIBRIDA


def verificar(lineas):
    """Devuelve la lista de incumplimientos del cableado (vacia si todo bien)."""
    errores = []
    reinicios = []
    resets_verificados = 0
    reset_pendiente = False
    nivel = 0

    for linea in lineas:
        # El bucle llama a escalador.nuevo_record() al registrar un record:
        # el siguiente reinicio debe volver al primer escalon. Solo cuenta como
        # reset si la escalera ya habia avanzado.
        if RE_NUEVO_RECORD.search(linea):
            if nivel > 0:
                reset_pendiente = True
            nivel = 0

        coincidencia = RE_REINICIO.search(linea)
        if not coincidencia:
            continue

        gen, _stagnant, reemplazados, fraccion, cuota, _trigger, _kept = (
            coincidencia.groups()
        )
        reemplazados = int(reemplazados)
        fraccion = float(fraccion)
        cuota = float(cuota)
        fraccion_esperada, cuota_esperada = parametros_esperados(nivel)
        reinicios.append((int(gen), reemplazados, fraccion, cuota))
        if reset_pendiente:
            # Este es el primer reinicio tras el record: la comprobacion de
            # `fraccion_esperada` de 0.4 confirma que el reset esta cableado.
            resets_verificados += 1
            reset_pendiente = False

        if abs(fraccion - fraccion_esperada) > 0.005:
            errores.append(
                f"gen={gen}: fraction={fraccion:.2f}, esperado "
                f"{fraccion_esperada:.2f} en el reinicio {nivel}"
            )
        if abs(cuota - cuota_esperada) > 0.005:
            errores.append(
                f"gen={gen}: mutation_share={cuota:.2f}, esperado "
                f"{cuota_esperada:.2f} en el reinicio {nivel}"
            )

        # `replaced=` debe seguir a la fraccion aplicada. Se admite +-1 por el
        # truncamiento de `static_cast<int>(fraccion * poblacion)`.
        esperados = round(fraccion * POBLACION)
        if abs(reemplazados - esperados) > 1:
            errores.append(
                f"gen={gen}: replaced={reemplazados}, esperado ~{esperados} "
                f"para fraction={fraccion:.2f} y population={POBLACION}"
            )

        nivel += 1

    if len(reinicios) < CAP + 1:
        errores.append(
            f"solo hubo {len(reinicios)} reinicios; se necesitan al menos "
            f"{CAP + 1} para observar el refinamiento-only"
        )
    elif not any(fraccion == FRACCION_REFINAMIENTO for _, _, fraccion, _ in reinicios):
        errores.append("la corrida no llego a refinement-only (fraction=0.2)")
    if resets_verificados == 0:
        errores.append(
            "ningun new_best corto una escalera ya avanzada seguido de un "
            "reinicio; no se pudo verificar el reset del cableado"
        )

    return errores, reinicios


def main():
    try:
        proceso = correr_solver()
    except subprocess.TimeoutExpired:
        print("check_restart_escalation: build/ode-solver excedio los 60s",
              file=sys.stderr)
        return 1

    if proceso.returncode != 0:
        print(
            "check_restart_escalation: build/ode-solver salio con codigo "
            f"{proceso.returncode}",
            file=sys.stderr,
        )
        return 1

    lineas = [linea for linea in proceso.stdout.splitlines() if linea.strip()]
    errores, reinicios = verificar(lineas)

    if errores:
        for error in errores:
            print(f"check_restart_escalation: FAIL | {error}", file=sys.stderr)
        return 1

    print(
        "check_restart_escalation: PASS | "
        f"{len(reinicios)} reinicios con fraction/replaced/mutation_share "
        "escalados y reset en new_best"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
