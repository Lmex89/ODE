#!/usr/bin/env python3
"""Smoke test for ODE cases 3-7 (issue #105).

Validates that the search behaves sanely end-to-end:
- Run completes
- Final best is finite (not INFINITY)
- Final best is strictly lower than first generation best (search improves)

This is the `smoke` mode; convergence checking lives in #108.
"""

import os
import re
import subprocess
import sys
from pathlib import Path


BINARIO = Path(__file__).resolve().parents[1] / "build" / "ode-solver"
CASOS = [3, 4, 5, 6, 7]
GENERACIONES = 200
SEED = 42
TIMEOUT = 120


def correr_caso(caso):
    """Run solver for one ODE case. Returns (returncode, stdout, stderr)."""
    entorno = dict(
        os.environ,
        ODE_SEED=str(SEED),
        ODE_CASE=str(caso),
        ODE_GEN=str(GENERACIONES),
    )
    proc = subprocess.run(
        [BINARIO],
        env=entorno,
        capture_output=True,
        text=True,
        timeout=TIMEOUT,
        check=False,
    )
    return proc.returncode, proc.stdout, proc.stderr


def parsear_log(lineas):
    """Extract first gen best and final best from log lines.
    
    Returns (first_best, final_best) where each is a float or None if not found.
    
    The log format is:
    - Each generation: gen=<n> best=<value> best_ever=<value> ...
    - Solution: solution | best=<value> ...
    - Summary: de_run_end | status=<status> generations=<n> best=<value> ...
    """
    first_best = None
    final_best = None
    
    for linea in lineas:
        # Extract first generation best (gen=1)
        match = re.search(r'gen=1\s+best=(\S+)', linea)
        if match and first_best is None:
            first_best = float(match.group(1))
        
        # Extract final best from solution line
        match = re.search(r'solution\s*\|\s*best=(\S+)', linea)
        if match:
            final_best = float(match.group(1))
        
        # Or from de_run_end summary
        match = re.search(r'de_run_end\s*\|.*best=(\S+)', linea)
        if match:
            final_best = float(match.group(1))
    
    return first_best, final_best


def validar_caso(caso):
    """Validate one ODE case. Returns (pass: bool, errors: list[str])."""
    errores = []
    
    try:
        returncode, stdout, stderr = correr_caso(caso)
    except subprocess.TimeoutExpired:
        return False, [f"timeout after {TIMEOUT}s"]
    
    if returncode != 0:
        return False, [f"exit code {returncode}"]
    
    lineas = stdout.splitlines()
    first_best, final_best = parsear_log(lineas)
    
    # Validate that we found both values
    if first_best is None:
        errores.append("could not find gen=1 best in log")
    if final_best is None:
        errores.append("could not find final best in log")
    
    if not errores:
        # Validate that final best is finite
        if final_best == float('inf'):
            errores.append(f"final best is INFINITY (first_best={first_best:.3e})")
        
        # Validate that search improved (final < first)
        if final_best >= first_best:
            errores.append(f"search did not improve: first={first_best:.3e}, final={final_best:.3e}")
    
    return len(errores) == 0, errores


def main():
    print(f"Smoke test: cases {CASOS}, gen={GENERACIONES}, seed={SEED}")
    print()
    
    resultados = {}
    for caso in CASOS:
        paso, errores = validar_caso(caso)
        resultados[caso] = (paso, errores)
    
    print()
    print("=" * 60)
    print("Summary:")
    for caso, (paso, errores) in resultados.items():
        status = "PASS" if paso else "FAIL"
        print(f"  Case {caso}: {status}")
        if errores:
            for err in errores:
                print(f"    - {err}")
    
    print("=" * 60)
    
    todos_pasaron = all(paso for paso, _ in resultados.values())
    if todos_pasaron:
        print("check_all_cases: PASS")
        return 0
    else:
        print("check_all_cases: FAIL")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
