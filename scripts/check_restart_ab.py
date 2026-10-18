#!/usr/bin/env python3
"""Verifica el experimento A/B legacy vs adaptive de reinicios (issue #130).

`scripts/restart_ab.sh` corre el MISMO binario (`build/ode-solver`) dos veces
por semilla, una con `ODE_RESTART_MODE=legacy` y otra con `adaptive`, con
presupuestos identicos (`ODE_GEN`, `ODE_STALE_LIMIT`, poblacion, dimension y
caso) y extrae la tabla estandar de metricas con el extractor compartido
`scripts/metricas.awk` (el mismo que usa `ab_experiment.sh`).

Este checker corre el script en modo corto y determinista (semilla 42,
gen=200, stale_limit=20, 30s de timeout por corrida) y valida:

1. La tabla comparativa: `metricas.tsv` tiene una fila por modo (legacy y
   adaptive) con los ocho campos estandar extraidos (sin "?") y valores
   plausibles; en stdout se imprime el metricas.tsv completo y el resumen
   pareado por semilla (fitness de cada modo, `mejor` y CPU).
2. El cableado por modo: cada log capturado reporta su `restart_mode=`
   efectivo en `de_run_start` y al menos una linea `restart |`, es decir el
   A/B ejercita de verdad el mecanismo de reinicio en ambos modos.
3. Presupuestos comparables: ambas corridas reportan el mismo
   `max_generations=`, `stale_limit=`, `population=` y `case=`, y los valores
   pedidos por flags (`--gen`, `--stale-limit`) llegan al binario.

Las piezas comunes con `check_ab_experiment.py` (entorno, corrida, lectura de
metricas.tsv y clave=valor del log) viven en scripts/ab_common.py. En Python
seria el equivalente a un test de integracion de pytest que corre el script
del experimento como subproceso y valida su tabla de resultados, el mismo
patron que `check_ab_experiment.py`.
"""

import re
import subprocess
import sys
import tempfile
from pathlib import Path

from ab_common import (
    REPO_ROOT,
    correr_script,
    es_finito,
    estado_valido,
    leer_metricas,
    pares_clave_valor,
)


EXPERIMENTO = REPO_ROOT / "scripts" / "restart_ab.sh"

SEMILLA = "42"
GEN_CORTO = "200"        # ODE_GEN de la corrida corta: termina sin converger
STALE_CORTO = "20"       # ODE_STALE_LIMIT: los reinicios disparan en ambos modos
TIMEOUT_CORRIDA = "30"   # segundos maximos por corrida dentro del experimento
TIMEOUT_SCRIPT = 180     # segundos para el experimento completo

MODOS = ("legacy", "adaptive")

# Campos que metricas.tsv debe extraer de cada log (mismo contrato que
# scripts/ab_experiment.sh): numericos (finitos) y de texto (no "?").
CAMPOS_NUMERICOS = ("generaciones", "mejor_fitness", "tiempo_cpu",
                    "tiempo_reloj")
CAMPOS_TEXTO = ("expresion", "vector")

# Presupuesto que ambos logs deben reportar (llega por flags al binario).
CAMPOS_PRESUPUESTO = ("max_generations", "stale_limit", "population", "case")

RE_DE_RUN_START = re.compile(r"de_run_start \|")
RE_REINICIO = re.compile(r"restart \|")


def correr_experimento(directorio):
    """Corre scripts/restart_ab.sh en modo corto con el outdir temporal.

    El presupuesto corto va por flags (--gen, --stale-limit); `correr_script`
    deja el entorno limpio de ODE_* (el script fija ODE_SEED, ODE_RESTART_MODE
    y ODE_STALE_LIMIT en cada corrida via flags).
    """
    return correr_script(
        EXPERIMENTO,
        ["--seed", SEMILLA,
         "--gen", GEN_CORTO,
         "--stale-limit", STALE_CORTO,
         "--timeout", TIMEOUT_CORRIDA,
         "--outdir", str(directorio)],
        TIMEOUT_SCRIPT,
    )


def errores_tabla(filas, salida):
    """Criterio 1: tabla comparativa con ambos modos y ocho campos extraidos."""
    errores = []
    if set(filas) != set(MODOS):
        errores.append(
            f"metricas.tsv debe tener una fila por modo {MODOS}, tiene "
            f"{sorted(filas)}"
        )
        return errores

    for campo in ("semilla", "estado", "convergio"):
        if not filas[MODOS[0]].get(campo):
            errores.append(f"metricas.tsv no extrajo la columna {campo!r}")

    for modo in MODOS:
        fila = filas[modo]
        if fila.get("semilla") != SEMILLA:
            errores.append(
                f"{modo}: semilla extraida {fila.get('semilla')!r}"
            )
        estado = fila.get("estado", "")
        if not estado_valido(estado):
            errores.append(
                f"{modo}: estado fuera del vocabulario ({estado!r})"
            )
        elif estado != "termino":
            errores.append(
                f"{modo}: la corrida no termino (estado={estado!r})"
            )
        if fila.get("convergio", "") not in ("si", "no"):
            errores.append(
                f"{modo}: convergio sin extraer ({fila.get('convergio')!r})"
            )
        for campo in CAMPOS_NUMERICOS:
            if not es_finito(fila.get(campo, "")):
                errores.append(
                    f"{modo}: {campo} sin extraer ({fila.get(campo)!r})"
                )
        for campo in CAMPOS_TEXTO:
            if fila.get(campo, "") in ("", "?"):
                errores.append(
                    f"{modo}: {campo} sin extraer ({fila.get(campo)!r})"
                )
        generaciones = fila.get("generaciones", "")
        if es_finito(generaciones) and int(float(generaciones)) > int(GEN_CORTO):
            errores.append(
                f"{modo}: generaciones={generaciones} supera el presupuesto "
                f"({GEN_CORTO})"
            )

    # La tabla impresa en stdout: encabezado estandar, ambos modos y el
    # resumen pareado por semilla (una fila de seis columnas por semilla).
    for esperado in ("mejor_fitness", "generaciones", "tiempo_cpu",
                     "legacy", "adaptive"):
        if esperado not in salida:
            errores.append(f"la tabla impresa no contiene {esperado!r}")
    for esperado in ("fitness_legacy", "fitness_adaptive"):
        if esperado not in salida:
            errores.append(f"el resumen impreso no contiene {esperado!r}")
    filas_resumen = [linea.split() for linea in salida.splitlines()]
    if not any(len(campos) == 6 and campos[0] == SEMILLA
               for campos in filas_resumen):
        errores.append(
            f"el resumen impreso no trae una fila de 6 columnas para la "
            f"semilla {SEMILLA}"
        )

    return errores


def leer_linea_arranque(texto):
    """Primera linea `de_run_start` del log, como dict clave -> valor."""
    for linea in texto.splitlines():
        if RE_DE_RUN_START.search(linea):
            return pares_clave_valor(linea)
    return {}


def errores_logs(directorio):
    """Criterios 2 y 3: modo efectivo, reinicios y presupuestos comparables."""
    errores = []
    arranques = {}
    reinicios = {}

    for modo in MODOS:
        log = Path(directorio) / f"{modo}-seed{SEMILLA}.log"
        texto = log.read_text(encoding="utf-8") if log.exists() else ""
        if not texto.strip():
            errores.append(f"el log capturado de '{modo}' esta vacio")
            continue

        arranque = leer_linea_arranque(texto)
        if not arranque:
            errores.append(f"al log de '{modo}' le falta la linea de_run_start")
        arranques[modo] = arranque

        reinicios[modo] = sum(1 for linea in texto.splitlines()
                              if RE_REINICIO.search(linea))
        if reinicios[modo] == 0:
            errores.append(
                f"'{modo}': ninguna linea 'restart |' con "
                f"ODE_STALE_LIMIT={STALE_CORTO}; el A/B no ejercito el reinicio"
            )

        if arranque.get("restart_mode") != modo:
            errores.append(
                f"'{modo}': de_run_start reporta "
                f"restart_mode={arranque.get('restart_mode')!r}"
            )

    if len(arranques) != len(MODOS):
        return errores

    # Presupuestos comparables: pedidos por flags y sin diferencias entre modos.
    referencia = arranques[MODOS[0]]
    for campo in CAMPOS_PRESUPUESTO:
        valor = referencia.get(campo)
        if valor is None:
            errores.append(f"de_run_start no reporta {campo}=")
        for modo in MODOS[1:]:
            if arranques[modo].get(campo) != valor:
                errores.append(
                    f"presupuesto distinto entre modos: {campo}="
                    f"{valor!r} (legacy) vs "
                    f"{arranques[modo].get(campo)!r} (adaptive)"
                )
    if referencia.get("max_generations") != GEN_CORTO:
        errores.append(
            f"--gen {GEN_CORTO} no llego al binario: "
            f"max_generations={referencia.get('max_generations')!r}"
        )
    if referencia.get("stale_limit") != STALE_CORTO:
        errores.append(
            f"--stale-limit {STALE_CORTO} no llego al binario: "
            f"stale_limit={referencia.get('stale_limit')!r}"
        )

    return errores


def main():
    if not EXPERIMENTO.exists():
        print(f"check_restart_ab: FAIL | falta {EXPERIMENTO}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="restart-ab-check-") as temporal:
        try:
            corrida = correr_experimento(temporal)
        except subprocess.TimeoutExpired:
            print(f"check_restart_ab: FAIL | el experimento excedio los "
                  f"{TIMEOUT_SCRIPT}s", file=sys.stderr)
            return 1

        if corrida.returncode != 0:
            print(f"check_restart_ab: FAIL | restart_ab.sh salio con "
                  f"codigo {corrida.returncode}", file=sys.stderr)
            for etiqueta, salida in (("stdout", corrida.stdout),
                                     ("stderr", corrida.stderr)):
                if salida.strip():
                    print(f"    ultimas lineas de {etiqueta}:", file=sys.stderr)
                    for linea in salida.splitlines()[-5:]:
                        print(f"    {linea}", file=sys.stderr)
            return 1

        try:
            filas = leer_metricas(temporal)
        except (OSError, KeyError) as error:
            print(f"check_restart_ab: FAIL | no se pudo leer metricas.tsv: "
                  f"{error}", file=sys.stderr)
            return 1

        casos = (
            ("tabla comparativa legacy/adaptive", lambda: errores_tabla(
                filas, corrida.stdout)),
            ("cableado por modo y presupuestos", lambda: errores_logs(temporal)),
        )

        fallos = 0
        for nombre, funcion in casos:
            errores = funcion()
            if errores:
                fallos += 1
                print(f"check_restart_ab: FAIL | {nombre}", file=sys.stderr)
                for error in errores:
                    print(f"    {error}", file=sys.stderr)
            else:
                print(f"check_restart_ab: PASS | {nombre}")

    if fallos:
        print(f"check_restart_ab: FAIL | {fallos}/{len(casos)} casos",
              file=sys.stderr)
        return 1

    print("check_restart_ab: PASS | metricas.tsv compara legacy y adaptive "
          "con presupuestos identicos, resumen por semilla y reinicios en "
          "ambos modos")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
