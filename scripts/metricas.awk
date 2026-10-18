# =========================================================================
# scripts/metricas.awk - Extractor de metricas de un log de ode-solver.
#
# Se usa con `awk -v estado=E -f scripts/metricas.awk <log>` y lo comparten
# scripts/ab_experiment.sh (antes/despues, issue #4) y scripts/restart_ab.sh
# (legacy/adaptive, issue #130). Cada corrida se resume en una fila TSV con
# ocho campos, siempre en este orden:
#
#   semilla  estado  convergio  generaciones  mejor_fitness  expresion
#   vector  tiempo_cpu
#
# Entradas:
#   -v estado=E   "termino" si el proceso acabo por si solo, "timeout" si lo
#                 corto `timeout`, "error:N" si el binario fallo. Se usa para
#                 deducir la convergencia en los logs historicos.
#   archivo       log completo de la corrida (stdout del binario).
#
# Conviven dos formatos de log:
#   - historico (revisiones viejas): "Seed: N", "BestCurrent :X", "****-> : N",
#     el vector como linea "[a, b, ...]", "f(x)= ..." y "Execution Time: T";
#   - nuevo (parse_tree.cpp con log estructurado): lineas
#     "[  0.12s] [INFO ] evento | clave=valor ..." con las claves seed, gen,
#     generations, best, expr, vector, status y elapsed_cpu_s.
# La convergencia sale del campo status (nuevo) o del tope de generaciones
# (historico). En Python seria un dict(...) por linea con split("="), como
# anota la funcion valor().
# =========================================================================

# valor(clave): valor del par clave=valor que aparezca en la linea (formato
# nuevo), o "" si no esta. En Python seria
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
}
