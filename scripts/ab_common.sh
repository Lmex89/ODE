# =========================================================================
# scripts/ab_common.sh - Piezas compartidas de los experimentos A/B.
#
# Lo cargan con `source` scripts/ab_experiment.sh (antes/despues, issue #4) y
# scripts/restart_ab.sh (legacy/adaptive, issue #130) despues de definir REPO;
# reune el arnes que antes estaba duplicado en cada script:
#
#   - estado_de_status: mapea el codigo de salida de la corrida al vocabulario
#     de `estado` ("termino", "timeout", "error:N"), columna 3 de metricas.tsv;
#   - crear_metricas / mostrar_metricas: cabecera estandar y tabla final;
#   - extraer_metricas: invoca el parser compartido scripts/metricas.awk;
#   - wall_de: segundos de reloj entre dos marcas `date +%s.%N`.
#
# En Python seria un modulo importable (el equivalente de scripts/ab_common.py
# en el lado de los checkers); aqui se carga con `source` para que las
# funciones queden en el shell de cada experimento.
# =========================================================================

# Vocabulario de `estado` (contrato compartido con scripts/metricas.awk y con
# scripts/ab_common.py, que valida el mismo patron).
ESTADO_TERMINO="termino"
ESTADO_TIMEOUT="timeout"

# Cabecera estandar de metricas.tsv: ocho campos por corrida, siempre iguales.
CABECERA_METRICAS=$'version\tsemilla\testado\tconvergio\tgeneraciones\tmejor_fitness\texpresion\tvector\ttiempo_cpu\ttiempo_reloj'

estado_de_status() {
    # $1 = codigo de salida de la corrida (124 = la corto `timeout`).
    if [[ "$1" -eq 124 ]]; then
        printf '%s\n' "$ESTADO_TIMEOUT"
    elif [[ "$1" -ne 0 ]]; then
        printf 'error:%s\n' "$1"
    else
        printf '%s\n' "$ESTADO_TERMINO"
    fi
}

crear_metricas() {
    # $1 = ruta del metricas.tsv; escribe la cabecera estandar.
    printf '%s\n' "$CABECERA_METRICAS" > "$1"
}

mostrar_metricas() {
    # $1 = ruta del metricas.tsv; `column` si esta disponible, `cat` si no.
    column -t -s $'\t' "$1" || cat "$1"
}

extraer_metricas() {
    # $1 = log, $2 = estado ("termino", "timeout" o "error:N").
    awk -v estado="$2" -f "$REPO/scripts/metricas.awk" "$1"
}

wall_de() {
    # $1 = marca inicial, $2 = marca final (date +%s.%N); segundos, 2 decimales.
    awk -v a="$1" -v b="$2" 'BEGIN{printf "%.2f", b-a}'
}
