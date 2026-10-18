#!/usr/bin/env bash
# =========================================================================
# scripts/restart_ab.sh - Experimento A/B legacy vs adaptive (issue #130).
#
# Corre el MISMO binario (build/ode-solver) dos veces por semilla con
# presupuestos identicos; la unica variable es ODE_RESTART_MODE:
#
#   - "legacy"  : reinicio historico (issue #108), el default,
#   - "adaptive": reinicio adaptativo escalado (issues #125/#127/#128/#129).
#
# Los knobs del reinicio se fijan a sus defaults documentados (fraccion
# minima 0.4, tope 0.8, cap 3, cuota hibrida 0.5, umbral de diversidad 0.05)
# para que la comparacion no dependa del entorno del usuario; el presupuesto
# (caso, poblacion, dimension, generaciones, stale_limit) tambien es comun.
#
# Salidas en $OUTDIR (por defecto un mktemp en /tmp/opencode):
#   - legacy-seedN.log, adaptive-seedN.log   log completo de cada corrida
#   - metricas.tsv                           tabla comparativa estandar
#
# El parser del log es scripts/metricas.awk y el arnes de corrida vive en
# scripts/ab_common.sh, ambos compartidos con scripts/ab_experiment.sh.
#
# Uso (compilar antes: cmake -B build && cmake --build build):
#   scripts/restart_ab.sh [--seed "1 42 2026"] [--gen 1000]
#                         [--stale-limit 50] [--timeout 300] [--outdir DIR]
#
# Para un desarrollador Python: este script cumple el rol de un
# `restart_ab.py`; esta en bash por la misma razon que ab_experiment.sh
# (git, timeout y column son de shell y el repo no tiene entorno Python).
# =========================================================================
set -euo pipefail

REPO="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
BINARIO="$REPO/build/ode-solver"
EXTRACTOR="$REPO/scripts/metricas.awk"

# Arnes compartido con scripts/ab_experiment.sh: estado_de_status,
# crear_metricas, extraer_metricas, wall_de y mostrar_metricas.
source "$REPO/scripts/ab_common.sh"

# Valores por defecto, sobreescribibles por flags. El presupuesto por defecto
# (caso 1 del paper, gen 1000, stale_limit 50) mantiene ambos modos dentro de
# ~35s por corrida y hace disparar reinicios en los dos.
SEMILLAS=${SEMILLAS:-"1 42 2026"}    # una corrida legacy y una adaptive por semilla
GENERACIONES=${GENERACIONES:-1000}   # generaciones maximas por corrida
STALE_LIMIT=${STALE_LIMIT:-50}
POBLACION=${POBLACION:-100}
DIMENSION=${DIMENSION:-20}
CASO=${CASO:-1}               # caso 1 = el del paper
TIMEOUT_S=${TIMEOUT_S:-300}
OUTDIR=${OUTDIR:-}

uso() {
    cat <<'FIN'
uso: scripts/restart_ab.sh [opciones]

  --seed "N1 N2 .."   semillas a correr en ambos modos (def: "1 42 2026")
  --gen N             generaciones maximas por corrida (def: 1000)
  --stale-limit N     generaciones sin mejora antes del reinicio (def: 50)
  --pop N             poblacion (def: 100)
  --dim N             dimension del individuo (def: 20)
  --case N            caso ODE (def: 1, el caso del paper)
  --timeout S         segundos maximos por corrida (def: 300)
  --outdir DIR        directorio de resultados (def: mktemp en /tmp/opencode)
  -h, --help          muestra esta ayuda
FIN
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --seed)        SEMILLAS="$2";      shift 2 ;;
        --gen)         GENERACIONES="$2";  shift 2 ;;
        --stale-limit) STALE_LIMIT="$2";   shift 2 ;;
        --pop)         POBLACION="$2";     shift 2 ;;
        --dim)         DIMENSION="$2";     shift 2 ;;
        --case)        CASO="$2";          shift 2 ;;
        --timeout)     TIMEOUT_S="$2";     shift 2 ;;
        --outdir)      OUTDIR="$2";        shift 2 ;;
        -h|--help)     uso; exit 0 ;;
        *) echo "opcion desconocida: $1" >&2; uso >&2; exit 2 ;;
    esac
done

if [[ ! -x "$BINARIO" ]]; then
    echo "restart_ab: falta $BINARIO; corre 'cmake -B build && cmake --build build'" >&2
    exit 1
fi
if [[ ! -f "$EXTRACTOR" ]]; then
    echo "restart_ab: falta $EXTRACTOR (extractor compartido de metricas)" >&2
    exit 1
fi

if [[ -z "$OUTDIR" ]]; then
    OUTDIR="$(mktemp -d /tmp/opencode/restart-ab.XXXXXX)"
fi
mkdir -p "$OUTDIR"

SHA_BINARIO=$(sha256sum "$BINARIO" | cut -d' ' -f1)

echo "== Experimento A/B de reinicios (issue #130): legacy vs adaptive =="
echo "  revision             : $(git -C "$REPO" rev-parse --short HEAD)  $(git -C "$REPO" log -1 --format=%s)"
echo "  binario (mismo en A/B): build/ode-solver  sha256=${SHA_BINARIO:0:16}"
echo "  semillas             : $SEMILLAS"
echo "  presupuesto comun    : case=$CASO pop=$POBLACION dim=$DIMENSION gen=$GENERACIONES stale_limit=$STALE_LIMIT"
echo "  timeout por corrida  : ${TIMEOUT_S}s"
echo "  resultados en        : $OUTDIR"

# El binario sale del arbol de trabajo: si hay cambios sin commitear puede no
# corresponder a HEAD (mismo aviso que ab_experiment.sh).
if [[ -n "$(git -C "$REPO" status --porcelain)" ]]; then
    echo "  AVISO: hay cambios sin commitear; el binario puede no ser el de HEAD"
fi

METRICAS="$OUTDIR/metricas.tsv"
crear_metricas "$METRICAS"

for SEMILLA in $SEMILLAS; do
    for MODO in legacy adaptive; do
        LOG="$OUTDIR/$MODO-seed$SEMILLA.log"
        T0=$(date +%s.%N)
        STATUS=0
        # Los knobs van explicitos para que la corrida no dependa del shell:
        # ODE_RESTART_MODE es la unica diferencia entre los dos modos, y el
        # resto queda en los defaults documentados de cada issue. Se elimina
        # ODE_LOG_FILE para que ODE_LOG=terminal mande sin ambiguedad (en
        # terminal no se crea ode-solver.log). En Python seria
        # subprocess.run([...], env={**os.environ, "ODE_LOG": "terminal", ...}).
        ( cd "$OUTDIR" && env -u ODE_LOG_FILE \
            ODE_LOG=terminal \
            ODE_SEED="$SEMILLA" \
            ODE_CASE="$CASO" \
            ODE_GEN="$GENERACIONES" \
            ODE_POP="$POBLACION" \
            ODE_DIM="$DIMENSION" \
            ODE_WARM_START=0 \
            ODE_STALE_LIMIT="$STALE_LIMIT" \
            ODE_RESTART_MODE="$MODO" \
            ODE_RESTART_MIN_FRACTION=0.4 \
            ODE_RESTART_FRACTION=0.8 \
            ODE_RESTART_CAP=3 \
            ODE_RESTART_MUTATION_SHARE=0.5 \
            ODE_DIV_THRESHOLD=0.05 \
            timeout "$TIMEOUT_S" "$BINARIO" > "$LOG" 2>&1 ) || STATUS=$?
        ESTADO="$(estado_de_status "$STATUS")"
        T1=$(date +%s.%N)
        WALL="$(wall_de "$T0" "$T1")"
        printf '%s\t%s\t%s\n' "$MODO" "$(extraer_metricas "$LOG" "$ESTADO")" "$WALL" >> "$METRICAS"
        echo "  corrida: modo=$MODO semilla=$SEMILLA estado=$ESTADO reloj=${WALL}s"
    done
done

# -------------------------------------------------------------------------
# resumen_semillas: una fila por semilla pareando los dos modos; `mejor` es el
# modo de menor mejor_fitness ("empate" si coinciden, "?" si a alguno no se le
# extrajo). En Python seria un dict por semilla y un bucle que arma la tabla.
# -------------------------------------------------------------------------
resumen_semillas() {
    # $1 = ruta del metricas.tsv.
    awk -F'\t' '
        BEGIN {
            printf "semilla\tfitness_legacy\tfitness_adaptive\tmejor\tcpu_legacy\tcpu_adaptive\n"
        }
        $1 == "legacy" { fit_l = $6; cpu_l = $9 }
        $1 == "adaptive" {
            fit_a = $6; cpu_a = $9
            mejor = "?"
            if (fit_l ~ /^-?[0-9]/ && fit_a ~ /^-?[0-9]/) {
                if (fit_l + 0 < fit_a + 0) mejor = "legacy"
                else if (fit_a + 0 < fit_l + 0) mejor = "adaptive"
                else mejor = "empate"
            }
            printf "%s\t%s\t%s\t%s\t%s\t%s\n",
                   $2, fit_l, fit_a, mejor, cpu_l, cpu_a
        }' "$1"
}

# -------------------------------------------------------------------------
# Tabla comparativa: primero el resumen pareado por semilla y luego el
# metricas.tsv completo; la unica diferencia entre los dos modos de una
# semilla es ODE_RESTART_MODE, el resto del presupuesto es comun.
# -------------------------------------------------------------------------
echo
echo "== Resumen por semilla (menor fitness = mejor) =="
RESUMEN="$(resumen_semillas "$METRICAS")"
printf '%s\n' "$RESUMEN" | column -t -s $'\t' || printf '%s\n' "$RESUMEN"
echo
mostrar_metricas "$METRICAS"
echo
echo "logs completos y metricas en: $OUTDIR"
