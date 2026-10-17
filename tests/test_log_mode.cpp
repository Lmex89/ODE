// =========================================================================
// test_log_mode.cpp - Tests del destino de la salida (issue #111).
//
// Para un desarrollador Python: igual que un `test_log_mode.py` de pytest.
// `ctest --test-dir build` lo compila y lo corre; termina con codigo de
// salida != 0 si algun assert falla. Los tests llaman SOLO a `resolver_log()`,
// la funcion pura de log_mode.h: la redireccion real de stdout (un efecto
// sobre el proceso) se prueba con scripts/check_log_redirect.py sobre el
// binario build/ode-solver, no aqui.
// Los valores esperados son el contrato del issue #111 (archivo por defecto,
// "terminal" exacto, ruta configurable), no una reimplementacion del codigo.
// =========================================================================

#include <iostream>
#include <string>

#include "log_mode.h"

static int g_checks = 0;   // asserts ejecutados (como el contador de pytest)
static int g_fallos = 0;   // asserts fallidos

// check_str: compara dos strings e imprime PASS/FAIL, como
// `assert got == esperado` en pytest.
static void check_str(const std::string& nombre, const std::string& got,
                      const std::string& esperado) {
    g_checks++;
    if (got == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        g_fallos++;
        std::cout << "FAIL  " << nombre << " (esperado \"" << esperado
                  << "\", obtenido \"" << got << "\")\n";
    }
}

// check_bool: exige true e imprime PASS/FAIL (en Python seria
// `assert condicion, nombre`).
static void check_bool(const std::string& nombre, bool got) {
    g_checks++;
    if (got) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        g_fallos++;
        std::cout << "FAIL  " << nombre << "\n";
    }
}

// -------------------------------------------------------------------------
// Slice 1: sin ODE_LOG ni ODE_LOG_FILE la salida va al archivo por defecto
// `ode-solver.log` del directorio de trabajo.
// -------------------------------------------------------------------------

static void test_ausente_va_a_archivo() {
    ConfigLog config = resolver_log(nullptr, nullptr);

    check_bool("ODE_LOG ausente -> modo archivo",
               config.salida == SalidaLog::Archivo);
    check_str("ODE_LOG_FILE ausente -> ruta por defecto", config.ruta,
              "ode-solver.log");
}

// -------------------------------------------------------------------------
// Slice 2: solo el texto exacto "terminal" devuelve la salida a stdout; la
// variable ausente, vacia o con cualquier otro texto deja el modo archivo
// (la ausencia de ODE_LOG es el caso normal del contrato).
// -------------------------------------------------------------------------

static void test_solo_terminal_exacto() {
    check_bool("ODE_LOG=terminal -> modo terminal",
               resolver_log("terminal", nullptr).salida == SalidaLog::Terminal);
    check_bool("ODE_LOG=Terminal (otro case) -> modo archivo",
               resolver_log("Terminal", nullptr).salida == SalidaLog::Archivo);
    check_bool("ODE_LOG vacia -> modo archivo",
               resolver_log("", nullptr).salida == SalidaLog::Archivo);
    check_bool("ODE_LOG=terminalx -> modo archivo",
               resolver_log("terminalx", nullptr).salida == SalidaLog::Archivo);
}

// -------------------------------------------------------------------------
// Slice 3: ODE_LOG_FILE cambia la ruta; ausente o vacia volveria al default,
// porque una ruta vacia no es un archivo valido (open("") de Python falla).
// -------------------------------------------------------------------------

static void test_ruta_configurable() {
    check_str("ODE_LOG_FILE=custom.log usa esa ruta",
              resolver_log(nullptr, "custom.log").ruta, "custom.log");
    check_str("ODE_LOG_FILE=logs/run-42.log usa esa ruta",
              resolver_log(nullptr, "logs/run-42.log").ruta, "logs/run-42.log");
    check_str("ODE_LOG_FILE vacia vuelve al default",
              resolver_log(nullptr, "").ruta, "ode-solver.log");
}

int main() {
    std::cout << "== test_log_mode: issue #111 ==\n";
    test_ausente_va_a_archivo();
    test_solo_terminal_exacto();
    test_ruta_configurable();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // codigo de salida como el de pytest
}
