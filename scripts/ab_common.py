#!/usr/bin/env python3
"""Piezas compartidas por los checkers de los experimentos A/B (issue #130).

scripts/check_ab_experiment.py (antes/despues) y scripts/check_restart_ab.py
(legacy/adaptive) corren un script A/B como subproceso, leen el mismo
metricas.tsv y comparten contratos: entorno sin las ODE_* del shell, corrida
via subprocess, campos extraidos finitos, el vocabulario de la columna
`estado` y la lectura clave=valor del log estructurado. Todo eso vive aqui
una sola vez.

En Python seria el analogo a un conftest.py importado por dos suites pytest.
"""

import math
import os
import re
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]

# Vocabulario de la columna `estado` de metricas.tsv: lo produce
# scripts/ab_common.sh (estado_de_status) y lo transporta scripts/metricas.awk.
RE_ESTADO = re.compile(r"^(termino|timeout|error:[0-9]+)$")

# Par clave=valor de una linea estructurada "[<t>s] [NIVEL] evento | ...".
# Misma forma que valor() de scripts/metricas.awk (issue #130): si cambia el
# formato del log, cambiar los dos.
RE_CLAVE_VALOR = re.compile(r"(\w+)=([^\s]+)")


def entorno_controlado(extra=None):
    """Copia os.environ sin las variables ODE_* y agrega `extra`.

    Un ODE_CASE, ODE_GEN u ODE_LOG del shell cambiaria la corrida; cada
    checker fija lo suyo por flags o por `extra`. En Python seria
    {**{k: v for k, v in os.environ.items() if not k.startswith("ODE_")},
     **extra}.
    """
    entorno = {clave: valor for clave, valor in os.environ.items()
               if not clave.startswith("ODE_")}
    if extra:
        entorno.update(extra)
    return entorno


def correr_script(script, argumentos, timeout, entorno=None):
    """Corre `script` con `argumentos` (bash) desde la raiz del repo.

    Sin `entorno` usa entorno_controlado() sin extras. Ademas del timeout del
    subproceso, el propio script acota cada corrida. En Python seria
    subprocess.run(["bash", str(script), *args], capture_output=True, ...).
    """
    return subprocess.run(
        ["bash", str(script), *[str(argumento) for argumento in argumentos]],
        cwd=REPO_ROOT,
        env=entorno if entorno is not None else entorno_controlado(),
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )


def es_finito(texto):
    """True si el campo extraido es un numero finito; "?" no lo es."""
    try:
        return math.isfinite(float(texto))
    except (TypeError, ValueError):
        return False


def estado_valido(texto):
    """True si `texto` pertenece al vocabulario de la columna `estado`."""
    return isinstance(texto, str) and RE_ESTADO.match(texto) is not None


def pares_clave_valor(linea):
    """dict clave -> valor de los pares `clave=valor` de una linea."""
    return dict(RE_CLAVE_VALOR.findall(linea))


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
