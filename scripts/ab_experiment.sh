#!/usr/bin/env bash
# =========================================================================
# scripts/ab_experiment.sh - Experimento A/B antes/despues (issue #4).
#
# Compila dos versiones del prototipo y las corre con la MISMA semilla para
# responder si los resultados historicos (archivos `out`/`out.txt`) son
# confiables:
#
#   - "antes"  : la revision bc70e95 (codigo con los bugs del issue #2),
#   - "despues": el HEAD del repo (con los fixes del issue #2).
#
# Ninguna de las dos revisiones aceptaba una semilla fija (usaban
# srand(time(NULL))), asi que a la version "antes" se le aplica el gancho de
# scripts/ab_seed_hook.patch: el mismo `resolver_semilla()` de ODE_SEED que
# usa "despues". El parche solo agrega el gancho; no toca la matematica.
#
# Salidas en $OUTDIR (por defecto un mktemp en /tmp/opencode):
#   - antes/main.exe, despues/build/ode-solver   binarios comparados
#   - antes-seedN.log, despues-seedN.log         salida completa de cada corrida
#   - metricas.tsv                               tabla de la comparacion
#
# Uso:
#   scripts/ab_experiment.sh [--seed "1 42 2026"] [--timeout 300]
#                            [--outdir DIR] [--before REV]
#
# Para un desarrollador Python: este script cumple el rol de un
# `ab_experiment.py`; esta en bash porque el repo no tiene entorno Python y
# todas las piezas (git, parche, compilador, timeout) son de shell.
# =========================================================================
set -euo pipefail

REPO="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

# Valores por defecto, sobreescribibles por variables de entorno o por flags.
BEFORE_REV=${BEFORE_REV:-bc70e95}   # revision sin los fixes del issue #2
SEEDS=${SEEDS:-"1 42 2026"}         # una corrida de A y de B por semilla
TIMEOUT_S=${TIMEOUT_S:-300}         # segundos maximos por corrida
OUTDIR=${OUTDIR:-}

uso() {
    cat <<'FIN'
uso: scripts/ab_experiment.sh [opciones]

  --seed "N1 N2 .."   semillas a correr en ambas versiones (def: "1 42 2026")
  --timeout S         segundos maximos por corrida (def: 300)
  --outdir DIR        directorio de resultados (def: mktemp en /tmp/opencode)
  --before REV        revision de la version A (def: bc70e95)
  -h, --help          muestra esta ayuda
FIN
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --seed)    SEEDS="$2";      shift 2 ;;
        --timeout) TIMEOUT_S="$2";  shift 2 ;;
        --outdir)  OUTDIR="$2";     shift 2 ;;
        --before)  BEFORE_REV="$2"; shift 2 ;;
        -h|--help) uso; exit 0 ;;
        *) echo "opcion desconocida: $1" >&2; uso >&2; exit 2 ;;
    esac
done

if [[ -z "$OUTDIR" ]]; then
    OUTDIR="$(mktemp -d /tmp/opencode/ab-experiment.XXXXXX)"
fi
mkdir -p "$OUTDIR"

CXX=${CXX:-g++}
FLAGS="-std=c++17 -Wall -O3"   # los mismos del Makefile

echo "== Experimento A/B (issue #4) =="
echo "  version B (despues): $(git -C "$REPO" rev-parse --short HEAD)  $(git -C "$REPO" log -1 --format=%s)"
echo "  version A (antes)  : $BEFORE_REV"
echo "  semillas           : $SEEDS"
echo "  timeout por corrida: ${TIMEOUT_S}s"
echo "  compilador         : $($CXX --version | head -1)"
echo "  resultados en      : $OUTDIR"

# Si hay cambios sin commitear, "despues" no es exactamente HEAD: avisar.
if [[ -n "$(git -C "$REPO" status --porcelain)" ]]; then
    echo "  AVISO: hay cambios sin commitear; version B usa el HEAD commiteado,"
    echo "         pero el gancho de A sale del arbol de trabajo."
fi

# -------------------------------------------------------------------------
# 1) Copias limpias de las dos revisiones (sin tocar el repo).
#    git archive empaqueta solo lo commiteado: A nunca ve los fixes.
# -------------------------------------------------------------------------
mkdir -p "$OUTDIR/antes" "$OUTDIR/despues"
git -C "$REPO" archive "$BEFORE_REV" | tar -x -C "$OUTDIR/antes"
git -C "$REPO" archive HEAD          | tar -x -C "$OUTDIR/despues"

# -------------------------------------------------------------------------
# 2) Gancho de semilla en A: los mismos semilla.{h,cpp} que usa B y el
#    parche que reemplaza srand(time(NULL)) por resolver_semilla(...).
# -------------------------------------------------------------------------
cp "$REPO/src/semilla.h" "$REPO/src/semilla.cpp" "$OUTDIR/antes/"
patch --batch --silent -d "$OUTDIR/antes" -p1 < "$REPO/scripts/ab_seed_hook.patch"
echo "  sha256 semilla.cpp  : $(sha256sum "$REPO/src/semilla.cpp" | cut -d' ' -f1)"

# -------------------------------------------------------------------------
# 3) Compilar ambas versiones con los mismos flags.
#    "antes" (bc70e95) tiene layout plano; "despues" (HEAD) tiene layout nuevo
#    con src/. Ambos producen binarios en sus respectivos directorios.
# -------------------------------------------------------------------------
( cd "$OUTDIR/antes"   && $CXX $FLAGS dualfunctions.cpp semilla.cpp parse_tree.cpp -o main.exe )
( cd "$OUTDIR/despues" && mkdir -p build && $CXX $FLAGS src/dualfunctions.cpp src/semilla.cpp src/parse_tree.cpp -o build/ode-solver )
echo "  md5 antes/despues  : $(md5sum "$OUTDIR/antes/main.exe" | cut -d' ' -f1) / $(md5sum "$OUTDIR/despues/build/ode-solver" | cut -d' ' -f1)"

# -------------------------------------------------------------------------
# 4) Correr A y B con cada semilla y extraer las metricas del log.
#    Conviven dos formatos:
#      - historico (version A y logs viejos): "Seed: N", "BestCurrent :X"
#        (issue #6; antes "BestCurrent1 :X"/"BestCurrent2 :X"), "****-> : N",
#        el vector como linea "[a, b, ...]", "f(x)= ..." y "Execution Time: T";
#      - nuevo (parse_tree.cpp con log estructurado): lineas
#        "[  0.12s] [INFO ] evento | clave=valor ..." con las claves seed,
#        gen, generations, best, expr, vector, status y elapsed_cpu_s.
#    El awk entiende las dos formas; la convergencia sale del campo status
#    (nuevo) o del tope de generaciones (historico).
# -------------------------------------------------------------------------
extraer_metricas() {
    # $1 = log, $2 = estado ("termino" o "timeout")
    # El separador de campos es el tabulador: en el printf de abajo, "\t".
    awk -v estado="$2" '
        # valor(clave): valor del par clave=valor que aparezca en la linea
        # (formato nuevo), o "" si no esta. En Python seria
        # dict(p.split("=", 1) for p in linea.split() if "=" in p).get(clave, "").
        function valor(clave,   n, pares, i) {
            n = split($0, pares, " ")
            for (i = 1; i <= n; i++) {
                if (pares[i] ~ ("^" clave "=")) return substr(pares[i], length(clave) + 2)
            }
            return ""
        }
        # --- Formato nuevo: "[<t>s] [NIVEL] evento | clave=valor ..." ---
        /\] \[(INFO |WARN |ERROR|DEBUG)\] / {
            # `generations=` de la linea de cierre pisa al `gen=` del avance.
            g = valor("generations")
            if (g != "") gen = g + 0
            else { g = valor("gen"); if (g != "") gen = g + 0 }

            v = valor("best")
            if (v ~ /^-?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?$/) {
                v += 0
                if (n_fit == 0 || v < fit) { fit = v; n_fit = 1 }
            }

            x = valor("expr");   if (x != "") expr = x
            w = valor("vector"); if (w != "") vec = w
            s = valor("status"); if (s != "") status_run = s
            e = valor("elapsed_cpu_s"); if (e != "") t_cpu = e
            sd = valor("seed");  if (sd != "") seed = sd
            next
        }
        # --- Formato historico ---
        /^BestCurrent[0-9]? :/ {
            v = $2; sub(/^:/, "", v);
            if (v ~ /^-?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?$/) {
                v += 0;
                if (n_fit == 0 || v < fit) { fit = v; n_fit = 1 }
            }
        }
        /\*\*\*\*-> : [0-9]+/  { gen = $3 + 0 }
        # Vector historico "[21, 11, ...]": los digitos seguidos de "," o "]"
        # evitan confundirlo con el timestamp "[  0.12s]" del formato nuevo.
        /^\[[0-9]+[, \]]/      { vec = $0 }
        /^f\(x\)= /            { expr = substr($0, 7) }
        /^Execution Time: /    { t_cpu = $3 }
        /^Seed: /              { seed = $2 }
        END {
            # Convergio si la corrida termino por fitness: en el formato nuevo
            # lo dice `status=converged`; en el historico se deduce del ultimo
            # gen impreso (el tope de 100000 generaciones imprime 100001
            # porque el contador incrementa antes del cout).
            if (status_run != "") conv = (estado == "termino" && status_run == "converged") ? "si" : "no";
            else conv = (estado == "termino" && gen != "" && gen <= 100000) ? "si" : "no";
            gsub(/\t/, " ", vec); gsub(/\t/, " ", expr);
            printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                   seed, estado, conv, (gen == "" ? "?" : gen),
                   (n_fit ? fit : "?"), (expr == "" ? "?" : expr),
                   (vec == "" ? "?" : vec), (t_cpu == "" ? "?" : t_cpu);
        }' "$1"
}

METRICS="$OUTDIR/metricas.tsv"
printf 'version\tsemilla\testado\tconvergio\tgeneraciones\tmejor_fitness\texpresion\tvector\ttiempo_cpu\ttiempo_reloj\n' > "$METRICS"

for SEED in $SEEDS; do
    for V in antes despues; do
        LOG="$OUTDIR/$V-seed$SEED.log"
        T0=$(date +%s.%N)
        STATUS=0
        # "antes" tiene layout plano (main.exe); "despues" tiene layout nuevo (build/ode-solver).
        if [[ "$V" == "antes" ]]; then
            BIN="./main.exe"
        else
            BIN="./build/ode-solver"
        fi
        ( cd "$OUTDIR/$V" && ODE_SEED="$SEED" timeout "$TIMEOUT_S" $BIN > "$LOG" 2>&1 ) || STATUS=$?
        ESTADO="termino"                  # el loop termino por si solo
        if [[ $STATUS -eq 124 ]]; then
            ESTADO="timeout"              # lo corto `timeout`
        elif [[ $STATUS -ne 0 ]]; then
            ESTADO="error:$STATUS"        # compilacion/ejecucion rota
        fi
        T1=$(date +%s.%N)
        WALL=$(awk -v a="$T0" -v b="$T1" 'BEGIN{printf "%.2f", b-a}')
        printf '%s\t%s\t%s\n' "$V" "$(extraer_metricas "$LOG" "$ESTADO")" "$WALL" >> "$METRICS"
        echo "  corrida: $V semilla=$SEED estado=$ESTADO reloj=${WALL}s"
    done
done

# -------------------------------------------------------------------------
# 5) Mostrar la tabla y dejar el resto en OUTDIR.
# -------------------------------------------------------------------------
echo
column -t -s $'\t' "$METRICS" || cat "$METRICS"
echo
echo "logs completos y metricas en: $OUTDIR"
