// =========================================================================
// test_semilla.cpp - Tests del control de semilla (issue #4).
//
// Para un desarrollador Python: igual que un `test_semilla.py` de pytest.
// `ctest --test-dir build` lo compila y lo corre; termina con codigo de salida != 0 si
// algun assert falla. Los tests llaman SOLO a `resolver_semilla()`, la
// funcion publica de semilla.h: es el contrato del modulo, no un detalle
// interno. Los valores esperados son literales del experimento A/B (misma
// semilla -> misma corrida), no una reimplementacion del codigo.
// =========================================================================

#include <iostream>
#include <string>

#include "semilla.h"

static int g_checks = 0;   // asserts ejecutados (como el contador de pytest)
static int g_fallos = 0;   // asserts fallidos

// check_long: compara un entero contra el esperado e imprime PASS/FAIL.
// En Python seria `assert got == esperado`, con el mensaje armado a mano
// porque C++ no trae pytest.
static void check_long(const std::string& nombre, long got, long esperado) {
    g_checks++;
    if (got == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        g_fallos++;
        std::cout << "FAIL  " << nombre << " (esperado " << esperado
                  << ", obtenido " << got << ")\n";
    }
}

// -------------------------------------------------------------------------
// Slice 1: ODE_SEED valida -> la semilla es ese numero.
// -------------------------------------------------------------------------

static void test_semilla_desde_entorno() {
    check_long("resolver_semilla(\"42\", 7) = 42",
               resolver_semilla("42", 7), 42);
}

// -------------------------------------------------------------------------
// Slice 2: ODE_SEED ausente o invalida -> se usa el respaldo (el reloj que
// main() inyecta), no un 0 silencioso que haria creer que la corrida es
// reproducible cuando no lo es.
// -------------------------------------------------------------------------

static void test_semilla_invalida_usa_respaldo() {
    check_long("resolver_semilla(\"\", 7) = 7",
               resolver_semilla("", 7), 7);
    check_long("resolver_semilla(\"abc\", 7) = 7",
               resolver_semilla("abc", 7), 7);
    check_long("resolver_semilla(nullptr, 7) = 7",
               resolver_semilla(nullptr, 7), 7);
}

int main() {
    std::cout << "== test_semilla: issue #4 ==\n";
    test_semilla_desde_entorno();
    test_semilla_invalida_usa_respaldo();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // codigo de salida como el de pytest
}
