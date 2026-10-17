#!/usr/bin/env python3
"""Verifica la extraccion de metricas del experimento A/B (issue #115).

Corre scripts/ab_experiment.sh en modo corto y determinista (ODE_GEN=200,
--seed 42, --timeout 20, --outdir temporal) y valida, para cada version:

1. "despues" (HEAD): el redirect del experimento captura el log estructurado
   (con las claves seed=, gen=, generations=, best=, expr=, vector=, status=
   y elapsed_cpu_s=), no deja ode-solver.log en su directorio de corrida
   (senal de modo terminal) y metricas.tsv extrae las ocho claves sin "?".
2. "antes" (bc70e95): el formato historico (Seed:, BestCurrent, ****->,
   f(x)=, vector) se sigue capturando y extrayendo aunque el timeout corte
   la corrida a mitad.

Punto de regresion del issue: desde #112 el solver escribe en ode-solver.log
por defecto y deja stdout vacio; sin ODE_LOG=terminal la corrida de "despues"
llega vacia al redirect del experimento y metricas.tsv sale toda en "?".

En Python seria el equivalente a un test de integracion de pytest que corre el
script del experimento como subproceso y valida su tabla de resultados.
"""

import math
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
EXPERIMENTO = REPO_ROOT / "scripts" / "ab_experiment.sh"

SEMILLA = "42"
GEN_CORTO = "200"        # ODE_GEN de la corrida corta: "despues" termina solo
TIMEOUT_CORRIDA = "20"   # segundos maximos por corrida dentro del experimento
TIMEOUT_SCRIPT = 300     # segundos para el experimento completo (compila A y B)

# Nombre del log por defecto (contrato de resolver_salida, issue #112): si
# "despues" corre en modo archivo, aparece aqui y el redirect captura vacio.
LOG_POR_DEFECTO = "ode-solver.log"

# Claves que el awk del experimento parsea del formato nuevo (mismo contrato
# que scripts/check_run_log.py): si alguna falta, "despues" no es comparable.
CLAVES_DESPUES = ("seed=", "gen=", "generations=", "best=", "expr=",
                  "vector=", "status=", "elapsed_cpu_s=")

# Evidencia minima del formato historico dentro de la corrida de "antes".
EVIDENCIA_ANTES = ("Seed: ", "BestCurrent", "****-> : ", "f(x)= ")

# Vector historico "[21, 11, ...]": mismos digitos seguidos de "," o "]" que
# usa el awk del experimento para no confundirlo con el timestamp "[  0.12s]".
RE_VECTOR_HISTORICO = re.compile(r"^\[[0-9]+[, \]]")


def entorno_controlado():
    """Copia os.environ sin las variables ODE_* (un ODE_CASE u ODE_LOG del
    shell cambiaria la corrida) y fija ODE_GEN para que "despues" termine
    dentro del timeout; el script fija ODE_SEED en cada corrida."""
    entorno = {clave: valor for clave, valor in os.environ.items()
               if not clave.startswith("ODE_")}
    entorno["ODE_GEN"] = GEN_CORTO
    return entorno


def correr_experimento(directorio):
    """Corre scripts/ab_experiment.sh con el outdir temporal."""
    return subprocess.run(
        ["bash", str(EXPERIMENTO),
         "--seed", SEMILLA,
         "--timeout", TIMEOUT_CORRIDA,
         "--outdir", str(directorio)],
        cwd=REPO_ROOT,
        env=entorno_controlado(),
        capture_output=True,
        text=True,
        timeout=TIMEOUT_SCRIPT,
        check=False,
    )


def es_finito(texto):
    """True si el campo extraido es un numero finito; "?" no lo es."""
    try:
        return math.isfinite(float(texto))
    except ValueError:
        return False


def leer_metricas(directorio):
    """metricas.tsv como dict version -> dict(campo -> valor)."""
    ruta = Path(directorio) / "metricas.tsv"
    lineas = [linea for linea in ruta.read_text(encoding="utf-8").splitlines()
              if linea.strip()]
    cabecera = lineas[0].split("\t")
    filas = {}
    for linea in lineas[1:]:
        fila = dict(zip(cabecera, linea.split("\t")))
        filas[fila["version"]] = fila
    return filas


def errores_despues(directorio, filas):
    """Caso 1: "despues" corre en modo terminal y sus ocho claves se extraen."""
    errores = []
    log = Path(directorio) / f"despues-seed{SEMILLA}.log"
    texto = log.read_text(encoding="utf-8") if log.exists() else ""
    if not texto.strip():
        errores.append(f"el log capturado de 'despues' esta vacio ({log.name})")
    for clave in CLAVES_DESPUES:
        if clave not in texto:
            errores.append(f"al log de 'despues' le falta la clave {clave!r}")

    archivo = Path(directorio) / "despues" / LOG_POR_DEFECTO
    if archivo.exists():
        errores.append(
            f"'despues' dejo {LOG_POR_DEFECTO}: no corrio en modo terminal")

    fila = filas.get("despues")
    if fila is None:
        return errores + ["metricas.tsv no tiene fila para 'despues'"]

    if fila.get("semilla") != SEMILLA:
        errores.append(f"'despues': semilla extraida {fila.get('semilla')!r}")
    if fila.get("estado") != "termino":
        errores.append(
            f"'despues': la corrida no termino (estado={fila.get('estado')!r})")
    if not es_finito(fila.get("mejor_fitness", "")):
        errores.append(
            f"'despues': mejor_fitness sin extraer ({fila.get('mejor_fitness')!r})")
    for campo in ("generaciones", "tiempo_cpu"):
        if not es_finito(fila.get(campo, "")):
            errores.append(
                f"'despues': {campo} sin extraer ({fila.get(campo)!r})")
    for campo in ("expresion", "vector"):
        if fila.get(campo, "") in ("", "?"):
            errores.append(
                f"'despues': {campo} sin extraer ({fila.get(campo)!r})")
    if fila.get("convergio", "") not in ("si", "no"):
        errores.append(
            f"'despues': convergio sin extraer ({fila.get('convergio')!r})")
    return errores


def errores_antes(directorio, filas):
    """Caso 2: "antes" conserva su formato y su extraccion bajo timeout."""
    errores = []
    log = Path(directorio) / f"antes-seed{SEMILLA}.log"
    texto = log.read_text(encoding="utf-8") if log.exists() else ""
    if not texto.strip():
        errores.append(f"el log capturado de 'antes' esta vacio ({log.name})")
    for evidencia in EVIDENCIA_ANTES:
        if evidencia not in texto:
            errores.append(f"al log de 'antes' le falta {evidencia!r}")
    if not any(RE_VECTOR_HISTORICO.match(linea) for linea in texto.splitlines()):
        errores.append("al log de 'antes' le falta el vector historico")

    fila = filas.get("antes")
    if fila is None:
        return errores + ["metricas.tsv no tiene fila para 'antes'"]

    if fila.get("semilla") != SEMILLA:
        errores.append(f"'antes': semilla extraida {fila.get('semilla')!r}")
    if fila.get("estado") != "timeout":
        errores.append(
            f"'antes': se esperaba el corte por timeout (estado={fila.get('estado')!r})")
    if not es_finito(fila.get("mejor_fitness", "")):
        errores.append(
            f"'antes': mejor_fitness sin extraer ({fila.get('mejor_fitness')!r})")
    if not es_finito(fila.get("generaciones", "")):
        errores.append(
            f"'antes': generaciones sin extraer ({fila.get('generaciones')!r})")
    for campo in ("expresion", "vector"):
        if fila.get(campo, "") in ("", "?"):
            errores.append(
                f"'antes': {campo} sin extraer ({fila.get(campo)!r})")
    return errores


def main():
    if not EXPERIMENTO.exists():
        print(f"check_ab_experiment: FAIL | falta {EXPERIMENTO}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="ab-experiment-check-") as temporal:
        try:
            corrida = correr_experimento(temporal)
        except subprocess.TimeoutExpired:
            print(f"check_ab_experiment: FAIL | el experimento excedio los "
                  f"{TIMEOUT_SCRIPT}s", file=sys.stderr)
            return 1

        if corrida.returncode != 0:
            print(f"check_ab_experiment: FAIL | ab_experiment.sh salio con "
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
            print(f"check_ab_experiment: FAIL | no se pudo leer metricas.tsv: "
                  f"{error}", file=sys.stderr)
            return 1

        casos = (
            ("despues (HEAD, modo terminal)", errores_despues),
            ("antes (bc70e95, intacto)", errores_antes),
        )

        fallos = 0
        for nombre, funcion in casos:
            errores = funcion(temporal, filas)
            if errores:
                fallos += 1
                print(f"check_ab_experiment: FAIL | {nombre}", file=sys.stderr)
                for error in errores:
                    print(f"    {error}", file=sys.stderr)
            else:
                print(f"check_ab_experiment: PASS | {nombre}")

    if fallos:
        print(f"check_ab_experiment: FAIL | {fallos}/{len(casos)} casos",
              file=sys.stderr)
        return 1

    print("check_ab_experiment: PASS | metricas.tsv extrae los dos formatos "
          "(despues en modo terminal, antes intacto)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
