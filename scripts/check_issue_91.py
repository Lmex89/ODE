#!/usr/bin/env python3
"""Verifica los criterios de aceptacion del issue #91: check_run_log.py debe
validar build/ode-solver, no un main.exe en la raiz.

Parte de #86 (Update remaining stale paths after the reorg). Criterios:

1. `python3 scripts/check_run_log.py` imprime PASS contra build/ode-solver
2. La ruta del binario se resuelve como la composicion build/ + ode-solver
3. No queda main.exe en el docstring, la ruta ni los mensajes del script
4. El contrato del log (lineas estructuradas, firma del autor, claves del A/B)
   se sigue verificando igual

Seams de verificacion:

1. Invocacion CLI real (criterios 1 y 4): ejecuta check_run_log.py como
   subproceso contra el build/ode-solver compilado y exige exit 0 + PASS.
   El PASS solo se imprime si las tres validaciones del contrato dan bien, asi
   que cubre tambien el criterio 4. Detecta rutas equivocadas o invocaciones
   rotas; no detecta texto obsoleto.

2. main.exe obsoleto en la raiz (criterio 2): con un main.exe falso en la raiz
   cuyo log viola el contrato, el checker debe pasar igual porque valida
   build/ode-solver y no el binario raiz. Es el peligro que motiva el issue:
   un clone longevo conserva un main.exe obsoleto y el checker validaria el
   binario equivocado en silencio. Detecta la ruta equivocada aunque el
   binario raiz exista; no detecta texto obsoleto.

3. Contenido estatico (criterio 3): el docstring, la ruta y los mensajes de
   check_run_log.py no mencionan main.exe. Detecta texto obsoleto que el
   comportamiento no revela (el checker puede funcionar y aun asi hablar del
   binario viejo); no detecta nada de runtime.

4. Camino negativo (criterio 4): en un arbol temporal con una copia del
   checker y un build/ode-solver falso que emite un log fuera de contrato, el
   checker debe fallar con 'check_run_log: FAIL'. Cubre las tres validaciones
   por separado (lineas sueltas, firma ausente, claves del A/B ausentes) y
   evita que el PASS del seam 1 sea vacio por validaciones relajadas. Detecta
   contrato recortado; no detecta la ruta del binario.

En Python seria un test de integracion que ejecuta el checker como subproceso
y valida su exit code y stdout/stderr.
"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CHECK_RUN_LOG = REPO_ROOT / "scripts" / "check_run_log.py"
ODE_SOLVER = REPO_ROOT / "build" / "ode-solver"
MAIN_EXE_RAIZ = REPO_ROOT / "main.exe"

TIMEOUT = 120

# Casos de log fuera de contrato, uno por validacion del checker (descripcion,
# cuerpo del build/ode-solver falso que los emite).
CASOS_FUERA_DE_CONTRATO = (
    (
        "una linea suelta fuera del formato estructurado",
        "#!/bin/sh\n"
        "echo 'hola mundo'\n",
    ),
    (
        "lineas estructuradas sin la firma del autor al final",
        "#!/bin/sh\n"
        "echo '[  0.01s] [INFO ] corrida | seed=42 gen=1 generations=1 "
        "best=1 expr=x vector=1 status=ok elapsed_cpu_s=0.01'\n",
    ),
    (
        "la firma del autor pero sin las claves del A/B",
        "#!/bin/sh\n"
        "echo '[  0.01s] [INFO ] signature | author=Lmex'\n",
    ),
)


def correr_checker():
    """Ejecuta check_run_log.py como lo haria una persona desde la raiz.
    En Python seria subprocess.run([...], capture_output=True, text=True)."""
    return subprocess.run(
        [sys.executable, str(CHECK_RUN_LOG)],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=TIMEOUT,
        check=False,
    )


def verificar_pass_real():
    """Seam 1: el checker pasa contra el build/ode-solver real.
    Devuelve la lista de errores (vacia si todo bien)."""
    if not ODE_SOLVER.exists():
        return [
            f"no existe {ODE_SOLVER.relative_to(REPO_ROOT)}; "
            "corre 'cmake -B build && cmake --build build'"
        ]

    proc = correr_checker()
    errores = []
    if proc.returncode != 0:
        errores.append(
            f"check_run_log.py salio con codigo {proc.returncode} "
            "(se esperaba PASS)"
        )
        if proc.stderr.strip():
            errores.append(f"stderr: {proc.stderr.strip()[:500]}")
    if "check_run_log: PASS" not in proc.stdout:
        errores.append(
            "stdout no contiene 'check_run_log: PASS': "
            f"{proc.stdout.strip()[:500]!r}"
        )
    return errores


def verificar_main_exe_obsoleto_ignorado():
    """Seam 2: un main.exe obsoleto en la raiz no debe ser validado. El
    main.exe falso emite un log fuera de contrato (sin firma): si el checker
    lo ejecutara, fallaria. Devuelve la lista de errores (vacia si todo bien)."""
    if not ODE_SOLVER.exists():
        return [
            f"no existe {ODE_SOLVER.relative_to(REPO_ROOT)}; "
            "corre 'cmake -B build && cmake --build build'"
        ]

    errores = []
    creado = False
    try:
        if not MAIN_EXE_RAIZ.exists():
            MAIN_EXE_RAIZ.write_text(
                "#!/bin/sh\n"
                "echo '[  0.01s] [INFO ] corrida obsoleta sin firma'\n"
            )
            MAIN_EXE_RAIZ.chmod(0o755)
            creado = True

        proc = correr_checker()
        if proc.returncode != 0:
            errores.append(
                "con un main.exe obsoleto en la raiz, check_run_log.py salio "
                f"con codigo {proc.returncode} (debe ignorarlo y validar "
                "build/ode-solver)"
            )
            if proc.stderr.strip():
                errores.append(f"stderr: {proc.stderr.strip()[:500]}")
        if "check_run_log: PASS" not in proc.stdout:
            errores.append(
                "con un main.exe obsoleto en la raiz, stdout no contiene "
                f"'check_run_log: PASS': {proc.stdout.strip()[:500]!r}"
            )
    finally:
        # Solo retira el main.exe si lo creo este seam; uno preexistente es
        # del clone y no se toca (en este seam el checker debe ignorarlo igual).
        if creado:
            MAIN_EXE_RAIZ.unlink()

    return errores


def verificar_sin_menciones_main_exe():
    """Seam 3: el script no menciona main.exe (docstring, ruta ni mensajes).
    Devuelve la lista de errores (vacia si todo bien)."""
    errores = []
    texto = CHECK_RUN_LOG.read_text(encoding="utf-8")
    if "main.exe" in texto:
        lineas = [
            f"    linea {i}: {linea.strip()}"
            for i, linea in enumerate(texto.splitlines(), 1)
            if "main.exe" in linea
        ]
        errores.append("check_run_log.py aun menciona main.exe:")
        errores.extend(lineas)
    return errores


def correr_checker_con_binario_falso(script_falso):
    """Seam 4: replica en un temporal el layout minimo (scripts/check_run_log.py
    + build/ode-solver) y ejecuta el checker con ese binario falso. Al ser una
    copia, el checker resuelve su build/ode-solver dentro del temporal. Devuelve
    el CompletedProcess de esa invocacion."""
    with tempfile.TemporaryDirectory(prefix="check_issue_91_") as tmp:
        tmp = Path(tmp)
        (tmp / "scripts").mkdir()
        (tmp / "build").mkdir()
        shutil.copy2(CHECK_RUN_LOG, tmp / "scripts" / "check_run_log.py")

        falso = tmp / "build" / "ode-solver"
        falso.write_text(script_falso)
        falso.chmod(0o755)

        return subprocess.run(
            [sys.executable, str(tmp / "scripts" / "check_run_log.py")],
            cwd=tmp,
            capture_output=True,
            text=True,
            timeout=TIMEOUT,
            check=False,
        )


def verificar_camino_negativo():
    """Seam 4: un log fuera de contrato sigue rechazandose con FAIL. Evita que
    el PASS del checker sea vacio si alguien relaja sus validaciones.
    Devuelve la lista de errores (vacia si todo bien)."""
    errores = []
    for descripcion, script_falso in CASOS_FUERA_DE_CONTRATO:
        proc = correr_checker_con_binario_falso(script_falso)
        if proc.returncode == 0:
            errores.append(
                f"el checker acepto un log con {descripcion} "
                "(contrato relajado)"
            )
        if "check_run_log: FAIL" not in proc.stderr:
            errores.append(
                f"el rechazo de un log con {descripcion} no reporto "
                "'check_run_log: FAIL'"
            )
    return errores


def main():
    errores = []
    errores.extend(verificar_pass_real())
    errores.extend(verificar_main_exe_obsoleto_ignorado())
    errores.extend(verificar_sin_menciones_main_exe())
    errores.extend(verificar_camino_negativo())

    if errores:
        for error in errores:
            print(f"check_issue_91: FAIL | {error}", file=sys.stderr)
        return 1

    print(
        "check_issue_91: PASS | check_run_log.py valida build/ode-solver "
        "y su contrato de log"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
