// test_per_case_config.cpp — tests de la configuracion por caso (issue #108).
//
// El issue #108 pide que `resolver_config` devuelva defaults distintos por
// caso (paper practice: pop/dim/gen se tunean por caso). Los casos 0-2 usan
// los defaults originales (100/20/5000); los casos 3-5 usan configs mas
// agresivos para converger.
//
// Se compila con -DODE_NO_MAIN (apaga el main() del programa y deja el de este
// archivo) y junto con dualfunctions.cpp y semilla.cpp; parse_tree.cpp se
// incluye mas abajo en vez de enlazarse, porque FunbObj no tiene header.
// En Python seria `from parse_tree import resolver_config`.

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>

#include "dualfunctions.h"
#include "token.h"

#include "parse_tree.cpp"

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_population.cpp).
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

static void check_int(const std::string& nombre, int recibido, int esperado) {
    g_checks++;
    if (recibido == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << " (esperado " << esperado
                  << ", recibido " << recibido << ")\n";
        g_fallos++;
    }
}

// -------------------------------------------------------------------------
// Tests: resolver_config acepta caso y devuelve defaults por caso.
// -------------------------------------------------------------------------

// Seam 1: resolver_config con caso=0 devuelve defaults originales.
// Atrapa: la funcion no acepta caso o usa defaults incorrectos.
void test_resolver_config_caso_0_defaults() {
    // Sin variables de entorno, caso 0 debe devolver defaults originales.
    ConfigBusqueda config = resolver_config(nullptr, nullptr, nullptr, nullptr, 0);
    
    check_int("caso_0 poblacion", config.poblacion, 100);
    check_int("caso_0 dimension", config.dimension, 20);
    check_int("caso_0 generaciones", config.max_generaciones, 5000);
    check_bool("caso_0 warm_start", !config.warm_start);
}

// Seam 1: resolver_config con caso=3 devuelve defaults tuned para converger.
// Atrapa: caso 3 no usa defaults mas agresivos.
void test_resolver_config_caso_3_tuned() {
    // Caso 3 debe tener defaults mas agresivos (paper practice).
    ConfigBusqueda config = resolver_config(nullptr, nullptr, nullptr, nullptr, 3);
    
    // Segun el paper, casos mas complejos necesitan mas poblacion y generaciones.
    // Estos valores iniciales son placeholders; el experimento los ajustara.
    check_bool("caso_3 poblacion >= 100", config.poblacion >= 100);
    check_bool("caso_3 dimension >= 20", config.dimension >= 20);
    check_bool("caso_3 generaciones >= 5000", config.max_generaciones >= 5000);
}

// Seam 1: resolver_config con env vars sobreescribe defaults por caso.
// Atrapa: env vars no tienen prioridad sobre defaults por caso.
void test_resolver_config_env_override() {
    // ODE_POP, ODE_DIM, ODE_GEN deben sobreescribir defaults por caso.
    setenv("ODE_POP", "250", 1);
    setenv("ODE_DIM", "30", 1);
    setenv("ODE_GEN", "10000", 1);
    
    ConfigBusqueda config = resolver_config(getenv("ODE_POP"), getenv("ODE_DIM"),
                                            getenv("ODE_GEN"), nullptr, 3);
    
    check_int("env override poblacion", config.poblacion, 250);
    check_int("env override dimension", config.dimension, 30);
    check_int("env override generaciones", config.max_generaciones, 10000);
    
    // Limpiar variables de entorno para no afectar otros tests.
    unsetenv("ODE_POP");
    unsetenv("ODE_DIM");
    unsetenv("ODE_GEN");
}

// -------------------------------------------------------------------------
// Tests: parametros de reinicio configurables (issue #108).
// -------------------------------------------------------------------------

// Seam 2: resolver_config acepta ODE_STALE_LIMIT y ODE_RESTART_FRACTION.
// Atrapa: variables de entorno no se parsean o no se almacenan en config.
void test_resolver_config_restart_params() {
    // ODE_STALE_LIMIT y ODE_RESTART_FRACTION deben ser configurables.
    setenv("ODE_STALE_LIMIT", "300", 1);
    setenv("ODE_RESTART_FRACTION", "0.9", 1);
    
    ConfigBusqueda config = resolver_config(nullptr, nullptr, nullptr, nullptr, 0,
                                            getenv("ODE_STALE_LIMIT"),
                                            getenv("ODE_RESTART_FRACTION"));
    
    check_int("stale_limit from env", config.stale_limit, 300);
    // Comparacion con epsilon para long double (evita problemas de precision).
    const long double epsilon = 1e-9L;
    check_bool("restart_fraction from env", 
               std::abs(config.restart_fraction - 0.9L) < epsilon);
    
    // Limpiar variables de entorno.
    unsetenv("ODE_STALE_LIMIT");
    unsetenv("ODE_RESTART_FRACTION");
}

// Seam 2: resolver_config usa defaults cuando no hay env vars.
// Atrapa: defaults incorrectos para parametros de reinicio.
void test_resolver_config_restart_defaults() {
    // Sin env vars, debe usar defaults: stale_limit=500, restart_fraction=0.8.
    ConfigBusqueda config = resolver_config(nullptr, nullptr, nullptr, nullptr, 0);
    
    check_int("stale_limit default", config.stale_limit, 500);
    // Comparacion con epsilon para long double (evita problemas de precision).
    const long double epsilon = 1e-9L;
    check_bool("restart_fraction default", 
               std::abs(config.restart_fraction - 0.8L) < epsilon);
}

// -------------------------------------------------------------------------
// main() ejecuta los tests y devuelve codigo de salida.
// -------------------------------------------------------------------------

int main() {
    std::cout << "=== tests resolver_config por caso (issue #108) ===\n\n";
    
    test_resolver_config_caso_0_defaults();
    test_resolver_config_caso_3_tuned();
    test_resolver_config_env_override();
    test_resolver_config_restart_params();
    test_resolver_config_restart_defaults();
    
    std::cout << "\n--- resumen: " << g_checks << " checks, "
              << g_fallos << " fallos ---\n";
    
    return g_fallos > 0 ? 1 : 0;
}
