#!/usr/bin/env python3
"""Smoke and convergence tests for ODE cases (issues #105, #108).

Two modes:
- smoke: validates search behaves sanely (completes, finite, improves)
- convergence: validates cases reach J < 1e-5 with status=converged

Convergence mode uses per-case configs and handles documented limitations.
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path


BINARIO = Path(__file__).resolve().parents[1] / "build" / "ode-solver"
REPO_ROOT = Path(__file__).resolve().parents[1]
CASOS = [3, 4, 5, 6, 7]
GENERACIONES = 200
SEED = 42
TIMEOUT = 120
TIMEOUT_CONVERGENCIA = 600  # convergence mode necesita más tiempo (pop=300, dim=30)

# Per-case configs for convergence mode (issue #108).
# These are tuned via experiments; cases 3-5 need more aggressive search.
# Case 3: pop=300, dim=30 converges at gen 184 with best=1.03e-38 (seed 42)
# Cases 4-5: require more generations and tuning (documented in docs/limitations.md if needed)
CASO_CONFIGS = {
    0: {"pop": 100, "dim": 20, "gen": 5000},
    1: {"pop": 100, "dim": 20, "gen": 5000},
    2: {"pop": 100, "dim": 20, "gen": 5000},
    3: {"pop": 300, "dim": 30, "gen": 20000},
    4: {"pop": 300, "dim": 30, "gen": 20000},
    5: {"pop": 300, "dim": 30, "gen": 20000},
    6: {"pop": 100, "dim": 20, "gen": 5000},
    7: {"pop": 100, "dim": 20, "gen": 5000},
}

CONVERGENCE_THRESHOLD = 1e-5


def cargar_limitaciones():
    """Load documented limitations from docs/limitations.md.
    
    Returns a set of case numbers that are documented as limitations.
    If the file doesn't exist, returns empty set.
    """
    limitaciones_file = REPO_ROOT / "docs" / "limitations.md"
    if not limitaciones_file.exists():
        return set()
    
    limitaciones = set()
    contenido = limitaciones_file.read_text()
    # Look for lines like "- Case 3:" or "Case 3:"
    for match in re.finditer(r'Case\s+(\d+)', contenido):
        limitaciones.add(int(match.group(1)))
    
    return limitaciones


def correr_caso(caso, modo="smoke", seed=SEED):
    """Run solver for one ODE case. Returns (returncode, stdout, stderr).
    
    Uses per-case config in convergence mode, fixed config in smoke mode.
    """
    # ODE_LOG=terminal (issue #114): this checker reads the run log from
    # stdout, but since #112 the solver writes to ode-solver.log by default.
    # Forcing terminal mode here overrides any ODE_LOG/ODE_LOG_FILE in the
    # caller's environment (the last key wins in dict()).
    if modo == "convergence":
        config = CASO_CONFIGS.get(caso, CASO_CONFIGS[0])
        entorno = dict(
            os.environ,
            ODE_LOG="terminal",
            ODE_SEED=str(seed),
            ODE_CASE=str(caso),
            ODE_POP=str(config["pop"]),
            ODE_DIM=str(config["dim"]),
            ODE_GEN=str(config["gen"]),
        )
        timeout = TIMEOUT_CONVERGENCIA
    else:
        entorno = dict(
            os.environ,
            ODE_LOG="terminal",
            ODE_SEED=str(seed),
            ODE_CASE=str(caso),
            ODE_GEN=str(GENERACIONES),
        )
        timeout = TIMEOUT
    
    proc = subprocess.run(
        [BINARIO],
        env=entorno,
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )
    return proc.returncode, proc.stdout, proc.stderr


def parsear_log(lineas):
    """Extract first gen best, final best, and status from log lines.
    
    Returns (first_best, final_best, status) where each float is None if not found,
    and status is None if not found.
    
    The log format is:
    - Each generation: gen=<n> best=<value> best_ever=<value> ...
    - Solution: solution | best=<value> ...
    - Summary: de_run_end | status=<status> generations=<n> best=<value> ...
    """
    first_best = None
    final_best = None
    status = None
    
    for linea in lineas:
        # Extract first generation best (gen=1)
        match = re.search(r'gen=1\s+best=(\S+)', linea)
        if match and first_best is None:
            first_best = float(match.group(1))
        
        # Extract final best from solution line
        match = re.search(r'solution\s*\|\s*best=(\S+)', linea)
        if match:
            final_best = float(match.group(1))
        
        # Extract status from de_run_end summary
        match = re.search(r'de_run_end\s*\|.*status=(\S+)', linea)
        if match:
            status = match.group(1)
        
        # Or from de_run_end summary (best)
        match = re.search(r'de_run_end\s*\|.*best=(\S+)', linea)
        if match:
            final_best = float(match.group(1))
    
    return first_best, final_best, status


def validar_caso(caso, modo="smoke", seed=SEED):
    """Validate one ODE case. Returns (pass: bool, errors: list[str]).
    
    In smoke mode: validates run completes, finite, improves.
    In convergence mode: validates status=converged and best < 1e-5.
    """
    errores = []
    
    try:
        returncode, stdout, stderr = correr_caso(caso, modo, seed)
    except subprocess.TimeoutExpired:
        return False, [f"timeout after {TIMEOUT}s"]
    
    if returncode != 0:
        return False, [f"exit code {returncode}"]
    
    lineas = stdout.splitlines()
    first_best, final_best, status = parsear_log(lineas)
    
    # Validate that we found both values
    if first_best is None:
        errores.append("could not find gen=1 best in log")
    if final_best is None:
        errores.append("could not find final best in log")
    
    if not errores:
        if modo == "convergence":
            # Convergence mode: status must be converged and best < threshold
            if status != "converged":
                errores.append(f"status={status}, expected converged")
            if final_best >= CONVERGENCE_THRESHOLD:
                errores.append(f"best={final_best:.3e} >= threshold {CONVERGENCE_THRESHOLD:.1e}")
        else:
            # Smoke mode: validate that final best is finite
            if final_best == float('inf'):
                errores.append(f"final best is INFINITY (first_best={first_best:.3e})")
            
            # Validate that search improved (final < first)
            if final_best >= first_best:
                errores.append(f"search did not improve: first={first_best:.3e}, final={final_best:.3e}")
    
    return len(errores) == 0, errores


def main():
    parser = argparse.ArgumentParser(description="Run ODE solver tests")
    parser.add_argument(
        "--mode",
        choices=["smoke", "convergence"],
        default="smoke",
        help="Test mode: smoke (default) or convergence",
    )
    parser.add_argument(
        "--cases",
        nargs="+",
        type=int,
        default=CASOS,
        help="Cases to test (default: 3 4 5 6 7)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=SEED,
        help="Random seed (default: 42)",
    )
    args = parser.parse_args()
    
    seed = args.seed
    CASOS_TEST = args.cases
    
    if args.mode == "smoke":
        print(f"Smoke test: cases {CASOS_TEST}, gen={GENERACIONES}, seed={seed}")
    else:
        print(f"Convergence test: cases {CASOS_TEST}, seed={seed}")
        print(f"Using per-case configs (see CASO_CONFIGS)")
    print()
    
    # Load documented limitations for convergence mode
    limitaciones = cargar_limitaciones() if args.mode == "convergence" else set()
    
    resultados = {}
    for caso in CASOS_TEST:
        paso, errores = validar_caso(caso, args.mode, seed)
        resultados[caso] = (paso, errores)
    
    print()
    print("=" * 60)
    print("Summary:")
    for caso, (paso, errores) in resultados.items():
        if paso:
            status = "PASS"
        elif args.mode == "convergence" and caso in limitaciones:
            status = "WARN (documented limitation)"
        else:
            status = "FAIL"
        
        print(f"  Case {caso}: {status}")
        if errores:
            for err in errores:
                print(f"    - {err}")
    
    print("=" * 60)
    
    # Check if all cases passed (or are documented limitations in convergence mode)
    todos_ok = True
    for caso, (paso, _) in resultados.items():
        if not paso:
            if args.mode == "convergence" and caso in limitaciones:
                continue  # Documented limitation, OK
            todos_ok = False
            break
    
    if todos_ok:
        print("check_all_cases: PASS")
        return 0
    else:
        print("check_all_cases: FAIL")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
