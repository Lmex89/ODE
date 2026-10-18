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

int main() {
    std::cout << "== test_restart_lifecycle: issue #138 (contrato de ControlReinicio) ==\n";
    test_primera_generacion_sin_reinicio();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;
}
