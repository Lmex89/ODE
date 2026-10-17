// =========================================================================
// test_salida.cpp - Tests de la resolucion de la salida (issue #112).
//
// Igual que test_semilla.cpp: un `test_salida.py` de pytest en Python. Los
// tests llaman SOLO a `resolver_salida()`, la funcion publica de salida.h;
// no miran como se abre el archivo ni que stream usa main(). Los valores
// esperados son literales del issue #112, no una reimplementacion del codigo.
// =========================================================================

#include <iostream>
#include <string>

#include "salida.h"

static int g_checks = 0;   // asserts ejecutados (como el contador de pytest)
static int g_fallos = 0;   // asserts fallidos

// check_bool: compara una condicion contra true e imprime PASS/FAIL.
// En Python seria `assert cond`, con el mensaje armado a mano.
static void check_bool(const std::string& nombre, bool condicion) {
    g_checks++;
    if (condicion) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        g_fallos++;
        std::cout << "FAIL  " << nombre << "\n";
    }
}

// check_texto: compara una cadena contra el literal esperado.
static void check_texto(const std::string& nombre, const std::string& recibido,
                        const std::string& esperado) {
    g_checks++;
    if (recibido == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        g_fallos++;
        std::cout << "FAIL  " << nombre << " (esperado \"" << esperado
                  << "\", obtenido \"" << recibido << "\")\n";
    }
}

// -------------------------------------------------------------------------
// Slice 1: sin ODE_LOG la corrida escribe a ode-solver.log del directorio
// actual. Atrapa: el default vuelve a ser la terminal (el problema original).
// -------------------------------------------------------------------------

static void test_sin_variables_usa_archivo_por_defecto() {
    const ConfigSalida salida = resolver_salida(nullptr, nullptr);
    check_bool("sin ODE_LOG -> modo archivo",
               salida.modo == ModoSalida::Archivo);
    check_texto("sin ODE_LOG -> ode-solver.log", salida.ruta, "ode-solver.log");
}

// -------------------------------------------------------------------------
// Slice 2: solo el valor exacto "terminal" devuelve la salida a stdout;
// ausente, vacio o cualquier otro texto sigue escribiendo el archivo default.
// Atrapa: comparaciones laxas (mayusculas, prefijos) que rompen el contrato.
// -------------------------------------------------------------------------

static void test_ode_log_terminal() {
    const ConfigSalida salida = resolver_salida("terminal", nullptr);
    check_bool("ODE_LOG=terminal -> modo terminal",
               salida.modo == ModoSalida::Terminal);
}

static void test_ode_log_otros_valores_usan_archivo() {
    check_bool("ODE_LOG=\"\" -> modo archivo",
               resolver_salida("", nullptr).modo == ModoSalida::Archivo);
    check_bool("ODE_LOG=stdout -> modo archivo",
               resolver_salida("stdout", nullptr).modo == ModoSalida::Archivo);
    check_bool("ODE_LOG=Terminal -> modo archivo (sensible a mayusculas)",
               resolver_salida("Terminal", nullptr).modo == ModoSalida::Archivo);
}

// -------------------------------------------------------------------------
// Slice 3: ODE_LOG_FILE elige el archivo; vacio no es una ruta util y vuelve
// al default. Atrapa: ignorar la variable o intentar abrir "".
// -------------------------------------------------------------------------

static void test_ode_log_file_custom() {
    const ConfigSalida salida = resolver_salida(nullptr, "/tmp/ode/experimento.log");
    check_bool("ODE_LOG_FILE custom -> modo archivo",
               salida.modo == ModoSalida::Archivo);
    check_texto("ODE_LOG_FILE custom -> ruta exacta", salida.ruta,
                "/tmp/ode/experimento.log");
}

static void test_ode_log_file_vacio_usa_default() {
    const ConfigSalida salida = resolver_salida(nullptr, "");
    check_bool("ODE_LOG_FILE=\"\" -> modo archivo",
               salida.modo == ModoSalida::Archivo);
    check_texto("ODE_LOG_FILE=\"\" -> ode-solver.log", salida.ruta,
                "ode-solver.log");
}

int main() {
    std::cout << "== test_salida: issue #112 ==\n";
    test_sin_variables_usa_archivo_por_defecto();
    test_ode_log_terminal();
    test_ode_log_otros_valores_usan_archivo();
    test_ode_log_file_custom();
    test_ode_log_file_vacio_usa_default();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // codigo de salida como el de pytest
}
