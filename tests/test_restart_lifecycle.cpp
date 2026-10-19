// test_restart_lifecycle.cpp — tests del ciclo de vida de reinicios (issue #138).
//
// El issue #138 define la costura entre el bucle DE y la decision de reinicio:
// ControlReinicio recibe la configuracion inmutable una sola vez y, por
// generacion, recibe los hechos ya calculados (si hubo `new_best` y el resumen
// de diversidad) y devuelve un valor de decision. Esta primera rebanada solo
// crea el esqueleto: la primera generacion debe devolver "no reinicio".
//
// Se compila con -DODE_NO_MAIN y junto con dualfunctions.cpp y semilla.cpp;
// parse_tree.cpp se incluye mas abajo (FunbObj no tiene header), el mismo
// patron que test_restart_escalation.cpp. En Python seria
// `from parse_tree import ControlReinicio, DecisionReinicio`.

#include <iostream>
#include <string>
#include <cmath>

#include "dualfunctions.h"
#include "token.h"

#include "parse_tree.cpp"

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_restart_escalation.cpp).
// -------------------------------------------------------------------------

static int g_checks = 0;
static int g_fallos = 0;

static void check_bool(const std::string& nombre, bool cond) {
    g_checks++;
    if (cond) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "\n";
        g_fallos++;
    }
}

static void check_decimal(const std::string& nombre, long double recibido,
                          long double esperado) {
    const long double epsilon = 1e-9L;
    g_checks++;
    if (std::abs(recibido - esperado) < epsilon) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "  (got=" << recibido
                  << " esperado=" << esperado << ")\n";
        g_fallos++;
    }
}

// -------------------------------------------------------------------------
// Seam: ControlReinicio::decidir() con la configuracion por defecto y una
// generacion sin `new_best` ni senal de diversidad.
// Atrapa: que el esqueleto reinicie en la primera generacion o que el valor
// de decision no distinga "no reinicio" de un reinicio real.
// -------------------------------------------------------------------------
static void test_primera_generacion_sin_reinicio() {
    const ConfigBusqueda config;
    ControlReinicio control(config);

    // Primera generacion sin record nuevo y sin resumen de diversidad: en
    // Python seria `decision = control.decidir(False, ResumenDiversidad())`.
    const DecisionReinicio decision =
        control.decidir(false, ResumenDiversidad{});

    check_bool("primera generacion no reinicia", !decision.hay_reinicio());
    check_bool("primera generacion no reporta disparador",
               decision.disparador == DisparadorReinicio::ninguno);
}

// -------------------------------------------------------------------------
// Seam: ControlReinicio::decidir() rastrea generaciones sin mejora y NO
// reinicia mientras el contador esté por debajo del stale_limit configurado.
// Atrapa: reiniciar antes de tiempo o no llevar el contador correctamente.
// -------------------------------------------------------------------------
static void test_sin_reinicio_debajo_del_limite() {
    ConfigBusqueda config;
    config.stale_limit = 5;  // Límite corto para probar el tracking
    ControlReinicio control(config);

    // 4 generaciones sin new_best y sin diversidad: aún no debe reiniciar
    for (int i = 0; i < 4; ++i) {
        const DecisionReinicio decision =
            control.decidir(false, ResumenDiversidad{});
        check_bool("generacion " + std::to_string(i + 1) +
                       " no reinicia (debajo del limite)",
                   !decision.hay_reinicio());
    }
}

// -------------------------------------------------------------------------
// Seam: ControlReinicio::decidir() dispara reinicio con trigger=stale cuando
// las generaciones sin mejora alcanzan el stale_limit configurado.
// Atrapa: no disparar en el límite, usar otro trigger, o no resetear el
// contador tras el reinicio.
// -------------------------------------------------------------------------
static void test_reinicio_en_el_limite_stale() {
    ConfigBusqueda config;
    config.stale_limit = 5;
    ControlReinicio control(config);

    // 4 generaciones sin new_best: aún no reinicia
    for (int i = 0; i < 4; ++i) {
        control.decidir(false, ResumenDiversidad{});
    }

    // 5ta generación: dispara stale
    const DecisionReinicio decision =
        control.decidir(false, ResumenDiversidad{});
    check_bool("quinta generacion reinicia por stale", decision.hay_reinicio());
    check_bool("disparador es stale",
               decision.disparador == DisparadorReinicio::stale);

    // Tras el reinicio, el contador se resetea: las siguientes 4 no reinician
    for (int i = 0; i < 4; ++i) {
        const DecisionReinicio siguiente =
            control.decidir(false, ResumenDiversidad{});
        check_bool("generacion post-reinicio " + std::to_string(i + 1) +
                       " no reinicia (contador reseteado)",
                   !siguiente.hay_reinicio());
    }
}

// -------------------------------------------------------------------------
// Seam: ControlReinicio::decidir() con nuevo_record=true debe resetear tanto
// la ventana de estancamiento (generaciones_sin_mejora_) como la escalera del
// escalador (reinicios_consecutivos_), para que el próximo reinicio vuelva a
// fraccion mínima (0.4) en lugar de continuar escalando.
// Atrapa: que nuevo_record solo resetee una de las dos cosas, o ninguna.
// -------------------------------------------------------------------------
static void test_nuevo_record_reinicia_ventana_y_escalera() {
    ConfigBusqueda config;
    config.stale_limit = 3;
    config.restart_min_fraction = 0.4L;
    config.restart_fraction = 0.8L;
    config.restart_mutation_share = 0.5L;
    config.restart_cap = 3;
    ControlReinicio control(config);

    // Primer reinicio: fraccion 0.4 (primer escalon)
    for (int i = 0; i < 2; ++i) control.decidir(false, ResumenDiversidad{});
    DecisionReinicio primero = control.decidir(false, ResumenDiversidad{});
    check_bool("primer reinicio stale dispara", primero.hay_reinicio());
    check_decimal("primer reinicio: fraccion 0.4", primero.parametros.fraccion, 0.4L);

    // Segundo reinicio consecutivo sin new_best: fraccion 0.6 (segundo escalon)
    for (int i = 0; i < 2; ++i) control.decidir(false, ResumenDiversidad{});
    DecisionReinicio segundo = control.decidir(false, ResumenDiversidad{});
    check_bool("segundo reinicio stale dispara", segundo.hay_reinicio());
    check_decimal("segundo reinicio: fraccion 0.6 (escalada)", segundo.parametros.fraccion, 0.6L);

    // Ahora llega un new_best: resetea escalera y ventana
    control.decidir(true, ResumenDiversidad{});

    // Tercer reinicio tras new_best: debe volver a 0.4 (no 0.8)
    for (int i = 0; i < 2; ++i) control.decidir(false, ResumenDiversidad{});
    DecisionReinicio tras_record = control.decidir(false, ResumenDiversidad{});
    check_bool("tercer reinicio tras new_best dispara", tras_record.hay_reinicio());
    check_decimal("tras new_best la fraccion vuelve a 0.4",
                  tras_record.parametros.fraccion, 0.4L);
    check_decimal("tras new_best la cuota vuelve a 0.5",
                  tras_record.parametros.cuota_mutacion, 0.5L);
}

// -------------------------------------------------------------------------
// Seam: En modo adaptive, si hay record finito y el resumen de diversidad
// muestra colapso (CV bajo, suficientes individuos finitos), dispara reinicio
// por diversidad ANTES de llegar al stale limit.
// Atrapa: que no se evalúe diversidad, que se dispare stale en vez de diversity,
// o que se dispare sin haber tenido un new_best previo.
// -------------------------------------------------------------------------
static void test_diversidad_dispara_en_modo_adaptive() {
    ConfigBusqueda config;
    config.restart_mode = ModoReinicio::adaptive;
    config.stale_limit = 10;  // Lo suficientemente alto para que no dispar stale
    config.div_threshold = 0.1L;
    ControlReinicio control(config);

    // Primero necesitamos un record finito (new_best)
    control.decidir(true, ResumenDiversidad{});

    // Ahora simulamos una generación con diversidad colapsada:
    // CV bajo (0.05 < 0.1), suficientes individuos finitos (8 de 10 = 80%)
    ResumenDiversidad diversidad_colapsada;
    diversidad_colapsada.individuos_finitos = 8;
    diversidad_colapsada.individuos_totales = 10;
    diversidad_colapsada.coeficiente_variacion = 0.05L;

    DecisionReinicio decision = control.decidir(false, diversidad_colapsada);

    check_bool("diversidad dispara reinicio en modo adaptive", decision.hay_reinicio());
    check_bool("disparador es diversity", decision.disparador == DisparadorReinicio::diversity);
}

// -------------------------------------------------------------------------
// Seam: En modo legacy, nunca se evalúa diversidad aunque las condiciones
// estén dadas. Solo stale puede disparar reinicio.
// Atrapa: que se dispare diversidad en modo legacy.
// -------------------------------------------------------------------------
static void test_legacy_suprime_diversidad() {
    ConfigBusqueda config;
    config.restart_mode = ModoReinicio::legacy;
    config.stale_limit = 10;
    config.div_threshold = 0.1L;
    ControlReinicio control(config);

    // Tenemos record y diversidad colapsada
    control.decidir(true, ResumenDiversidad{});

    ResumenDiversidad diversidad_colapsada;
    diversidad_colapsada.individuos_finitos = 8;
    diversidad_colapsada.individuos_totales = 10;
    diversidad_colapsada.coeficiente_variacion = 0.05L;

    DecisionReinicio decision = control.decidir(false, diversidad_colapsada);

    check_bool("legacy no dispara con diversidad colapsada", !decision.hay_reinicio());
}

// -------------------------------------------------------------------------
// Seam: Cuando ambas condiciones (stale y diversity) se cumplen en la misma
// generación, stale tiene prioridad sobre diversity.
// Atrapa: que se dispare diversity cuando stale también debería disparar.
// -------------------------------------------------------------------------
static void test_stale_tiene_prioridad_sobre_diversity() {
    ConfigBusqueda config;
    config.restart_mode = ModoReinicio::adaptive;
    config.stale_limit = 3;
    config.div_threshold = 0.1L;
    ControlReinicio control(config);

    // Tenemos record
    control.decidir(true, ResumenDiversidad{});

    // Llamamos decidir 2 veces sin new_best (stale counter = 2)
    control.decidir(false, ResumenDiversidad{});
    control.decidir(false, ResumenDiversidad{});

    // En la tercera llamada, stale_limit se alcanza (counter = 3)
    // Y además la diversidad está colapsada
    ResumenDiversidad diversidad_colapsada;
    diversidad_colapsada.individuos_finitos = 8;
    diversidad_colapsada.individuos_totales = 10;
    diversidad_colapsada.coeficiente_variacion = 0.05L;

    DecisionReinicio decision = control.decidir(false, diversidad_colapsada);

    check_bool("stale dispara cuando ambos condiciones se cumplen", decision.hay_reinicio());
    check_bool("stale tiene prioridad sobre diversity",
               decision.disparador == DisparadorReinicio::stale);
}

// -------------------------------------------------------------------------
// Seam: Después de `cap` reinicios consecutivos sin `new_best`, los parámetros
// deben ser de refinamiento (fraction=0.2, mutation_share=1.0).
// Atrapa: que no se escalone a refinamiento, o que los valores sean incorrectos.
// -------------------------------------------------------------------------
static void test_refinamiento_despues_del_cap() {
    ConfigBusqueda config;
    config.restart_mode = ModoReinicio::adaptive;
    config.stale_limit = 2;
    config.restart_min_fraction = 0.4L;
    config.restart_fraction = 0.8L;
    config.restart_mutation_share = 0.5L;
    config.restart_cap = 3;
    ControlReinicio control(config);

    // Primer reinicio: fracción mínima 0.4
    DecisionReinicio r1 = control.decidir(false, ResumenDiversidad{});
    r1 = control.decidir(false, ResumenDiversidad{});
    check_bool("primer reinicio dispara", r1.hay_reinicio());
    check_decimal("primer reinicio: fraccion 0.4", r1.parametros.fraccion, 0.4L);

    // Segundo reinicio: 0.6
    DecisionReinicio r2 = control.decidir(false, ResumenDiversidad{});
    r2 = control.decidir(false, ResumenDiversidad{});
    check_bool("segundo reinicio dispara", r2.hay_reinicio());
    check_decimal("segundo reinicio: fraccion 0.6", r2.parametros.fraccion, 0.6L);

    // Tercer reinicio: 0.8
    DecisionReinicio r3 = control.decidir(false, ResumenDiversidad{});
    r3 = control.decidir(false, ResumenDiversidad{});
    check_bool("tercer reinicio dispara", r3.hay_reinicio());
    check_decimal("tercer reinicio: fraccion 0.8", r3.parametros.fraccion, 0.8L);

    // Cuarto reinicio (después del cap=3): refinamiento
    DecisionReinicio r4 = control.decidir(false, ResumenDiversidad{});
    r4 = control.decidir(false, ResumenDiversidad{});
    check_bool("cuarto reinicio dispara", r4.hay_reinicio());
    check_decimal("cuarto reinicio: fraccion refinamiento 0.2", r4.parametros.fraccion, 0.2L);
    check_decimal("cuarto reinicio: mutation_share 1.0", r4.parametros.cuota_mutacion, 1.0L);
}

int main() {
    std::cout << "== test_restart_lifecycle: issue #138 (contrato de ControlReinicio) ==\n";
    test_primera_generacion_sin_reinicio();
    test_sin_reinicio_debajo_del_limite();
    test_reinicio_en_el_limite_stale();
    test_nuevo_record_reinicia_ventana_y_escalera();
    test_diversidad_dispara_en_modo_adaptive();
    test_legacy_suprime_diversidad();
    test_stale_tiene_prioridad_sobre_diversity();
    test_refinamiento_despues_del_cap();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;
}
