// test_population.cpp — tests del semillero de la poblacion (issue #11) y de
// la configuracion de la busqueda (issue #28).
//
// Para un desarrollador Python: este archivo es como un modulo `test_*.py` de
// pytest, pero sin framework: cada `test_*()` imprime PASS/FAIL y `main()` al
// final hace de resumen (seria el `pytest.main()` + exit code). El binario
// termina con codigo != 0 si algo fallo, igual que pytest.
//
// Se compila con -DODE_NO_MAIN (apaga el main() del programa y deja el de este
// archivo) y junto con dualfunctions.cpp y semilla.cpp; parse_tree.cpp se
// incluye mas abajo en vez de enlazarse, porque FunbObj no tiene header (mismo
// patron que test_grid.cpp: ver el comentario del include).
// En Python no existe "dos main() en un mismo binario": aqui el preprocesador
// `#ifndef ODE_NO_MAIN` del programa deja al compilador elegir que main() incluir.

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "dualfunctions.h"  // ahora autocontenido: incluye <vector>/<string> por si mismo
#include "token.h"          // expone el contrato publico del rango del alfabeto

// En vez de declarar prototipos sueltos, este test incluye parse_tree.cpp
// entero: createPopulation y FunbObj viven ahi sin header. En Python seria
// `from parse_tree import createPopulation, FunbObj`: importar el modulo real.
// El main() del programa queda fuera por -DODE_NO_MAIN (ver el Makefile).
#include "parse_tree.cpp"

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_dual/test_grid): contadores globales y
// helpers check_* que imprimen PASS/FAIL.
// -------------------------------------------------------------------------

static int g_checks = 0;   // total de asserts ejecutados
static int g_fallos = 0;   // asserts que fallaron

// check_bool: para invariantes que no son igualdades (`assert cond, mensaje`).
static void check_bool(const std::string& nombre, bool cond) {
    g_checks++;
    if (cond) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "\n";
        g_fallos++;
    }
}

// check_int: para igualdades entre enteros; imprime el valor recibido cuando
// falla, como el `assert got == esperado, f"...{got}..."` de pytest.
static void check_int(const std::string& nombre, int got, int esperado) {
    g_checks++;
    if (got == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "  (got=" << got
                  << " esperado=" << esperado << ")\n";
        g_fallos++;
    }
}

// -------------------------------------------------------------------------
// Slice 1 (issue #11): todo individuo del semillero mide lo mismo — 20 tokens.
// B5 de REVIEW.md: el seed `dos` tenia 21 y `Mutation()` leia fuera de rango
// cuando ese individuo entraba como operando x. La invariante de diseno es que
// la poblacion inicial es uniforme: los 5 individuos escritos a mano y los
// aleatorios de vector_population(20) miden 20 tokens.
// -------------------------------------------------------------------------

static void test_createPopulation_tamano_uniforme() {
    // srand fijo: createPopulation sortea los individuos aleatorios con rand();
    // fijar la semilla hace el test reproducible (en Python seria
    // `random.seed(20261013)`), igual que en los tests de mutacion/cruce.
    srand(20261013);

    // Configuracion por defecto del Caso 1 del paper (issue #28): dimension 20.
    ConfigBusqueda config;
    config.poblacion = 10;

    // `poblacion` entra por referencia y createPopulation la llena con
    // push_back (en Python seria `poblacion = []` pasada por referencia).
    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    check_bool("createPopulation produce individuos", !poblacion.empty());

    // vecvalue() es la via publica de lectura; _vecval es privado (Python no
    // tiene esta distincion: todo seria publico).
    bool todos_20 = true;
    for (size_t i = 0; i < poblacion.size(); i++) {
        if (poblacion[i].vecvalue().size() != 20) todos_20 = false;
    }
    check_bool("todos los individuos tienen 20 tokens (dimension del paper)", todos_20);
}

// -------------------------------------------------------------------------
// Slice 2 (issue #28): createPopulation crea EXACTAMENTE los individuos que
// pide config.poblacion. Antes el bucle aleatorio era `i < tam - 4` con 5
// semillas fijas, asi que una poblacion de 10 salia de 11 individuos (el
// off-by-one que reporta REVIEW C5: 5 + tam - 4 = tam + 1).
// -------------------------------------------------------------------------

static void test_createPopulation_cantidad_exacta() {
    srand(20261013);

    ConfigBusqueda config;
    config.poblacion = 10;

    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    // 10 es un literal independiente de la implementacion: el contrato es
    // "tantos individuos como pidas", no "los que salgan de la formula".
    check_bool("createPopulation(10) crea exactamente 10 individuos",
               poblacion.size() == 10);
}

// -------------------------------------------------------------------------
// Slice 3 (issue #28): la dimension sale de la configuracion, no de un 20
// fijo. Con ODE_DIM=12 la poblacion entera debe medir 12 tokens y seguir
// usando solo el alfabeto TOKEN_MIN..TOKENS_ALFABETO (el criterio de
// "individuos validos" de #28).
// -------------------------------------------------------------------------
static void test_createPopulation_respeta_la_dimension() {
    srand(20261013);

    ConfigBusqueda config;
    config.poblacion = 8;
    config.dimension = 12;

    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    bool todos_12 = true;
    bool tokens_validos = true;
    for (size_t i = 0; i < poblacion.size(); i++) {
        const vector<ldouble>& v = poblacion[i].vecvalue();
        if (v.size() != 12) todos_12 = false;
        for (size_t j = 0; j < v.size(); j++) {
            if (v[j] < TOKEN_MIN || v[j] > TOKENS_ALFABETO) tokens_validos = false;
        }
    }
    check_bool("cada individuo mide la dimension pedida (12)", todos_12);
    check_bool("todos los tokens estan en el alfabeto TOKEN_MIN..TOKENS_ALFABETO",
               tokens_validos);
}

// -------------------------------------------------------------------------
// Slice 4 (issue #28): el warm start es opt-in. Por defecto la poblacion es
// aleatoria uniforme (ecuacion (6) del paper, REVIEW D6); con
// ODE_WARM_START=1 entran las cinco expresiones escritas a mano del prototipo.
// -------------------------------------------------------------------------

static void test_warm_start_opt_in() {
    // semillas_warm_start() devuelve la lista por referencia constante (en
    // Python seria la misma lista del modulo, sin copiarla).
    const vector<vector<ldouble>>& semillas = semillas_warm_start();

    check_bool("el prototipo aporta cinco semillas warm start", semillas.size() == 5);
    bool miden_20 = true;
    for (size_t i = 0; i < semillas.size(); i++) {
        if (semillas[i].size() != 20) miden_20 = false;
    }
    check_bool("cada semilla mide la dimension del paper (20)", miden_20);

    // Con warm start, las semillas van al frente de la poblacion.
    srand(20261013);
    ConfigBusqueda caliente;
    caliente.poblacion = 10;
    caliente.warm_start = true;
    vector<FunbObj> con_semillas;
    createPopulation(con_semillas, caliente, 0);

    check_bool("warm start produce exactamente la poblacion pedida",
               con_semillas.size() == 10);

    bool al_frente = true;
    for (size_t i = 0; i < semillas.size(); i++) {
        if (con_semillas[i].vecvalue() != semillas[i]) al_frente = false;
    }
    check_bool("las cinco semillas estan al frente de la poblacion", al_frente);

    // Ancla independiente: la primera semilla es la expresion `uno` del
    // prototipo, transcrita aqui como literal para que un cambio accidental de
    // los datos no pase inadvertido (solo la comprobaria el mismo codigo).
    vector<ldouble> uno = {18,13,17,11,14,14,11,14,14,14,13,14,14,14,14,14,14,14,14,11};
    check_bool("la primera semilla es la `uno` del prototipo",
               con_semillas[0].vecvalue() == uno);

    // Sin warm start ninguna semilla entra. srand fijo hace el sorteo
    // deterministico, asi que la comprobacion no es probabilistica.
    srand(20261013);
    ConfigBusqueda frio;
    frio.poblacion = 10;
    vector<FunbObj> sin_semillas;
    createPopulation(sin_semillas, frio, 0);

    bool ninguna = true;
    for (size_t i = 0; i < sin_semillas.size(); i++) {
        for (size_t s = 0; s < semillas.size(); s++) {
            if (sin_semillas[i].vecvalue() == semillas[s]) ninguna = false;
        }
    }
    check_bool("sin warm start no aparece ninguna semilla fija", ninguna);
}

// -------------------------------------------------------------------------
// Slice 5 (issue #28): la reproducibilidad empieza en la poblacion inicial: la
// misma semilla de rand() debe producir exactamente los mismos individuos
// (criterion 3 de #28).
// -------------------------------------------------------------------------
static void test_createPopulation_reproducible() {
    ConfigBusqueda config;
    config.poblacion = 10;

    srand(20261013);
    vector<FunbObj> primera;
    createPopulation(primera, config, 0);

    srand(20261013);
    vector<FunbObj> segunda;
    createPopulation(segunda, config, 0);

    bool iguales = primera.size() == segunda.size();
    for (size_t i = 0; i < primera.size() && iguales; i++) {
        if (primera[i].vecvalue() != segunda[i].vecvalue()) iguales = false;
    }
    check_bool("la misma semilla produce la misma poblacion inicial", iguales);
}

// -------------------------------------------------------------------------
// Slice 6 (issue #28): configuracion de la busqueda. Sin variables de
// entorno, los valores por defecto son los del Caso 1 del paper (REVIEW
// C5-C7): 100 individuos, dimension 20 y 5000 generaciones. En Python esto
// seria un `@dataclass` con defaults, construido por una funcion pura que
// recibe los `os.environ.get(...)` ya leidos.
// -------------------------------------------------------------------------

static void test_config_defaults() {
    // Mismo patron que resolver_semilla: la funcion recibe los getenv() ya
    // resueltos (nullptr = variable ausente) para poder probarse sin tocar el
    // entorno del proceso.
    ConfigBusqueda config = resolver_config(nullptr, nullptr, nullptr, nullptr);

    // Los literales 100/20/5000 vienen del paper (Caso 1), no de la
    // implementacion: son la fuente de verdad independiente del test.
    check_int("poblacion por defecto = 100 (Caso 1 del paper)", config.poblacion, 100);
    check_int("dimension por defecto = 20 (Caso 1 del paper)", config.dimension, 20);
    check_int("generaciones por defecto = 5000 (Caso 1 del paper)",
              config.max_generaciones, 5000);
    check_bool("warm start desactivado por defecto", !config.warm_start);
}

static void test_config_overrides() {
    // Cada variable de entorno pisa su default (equivalente a leer
    // os.environ["ODE_POP"], etc.). "0" en la bandera desactiva el warm start.
    ConfigBusqueda config = resolver_config("50", "30", "77", "0");

    check_int("ODE_POP=50 sobreescribe la poblacion", config.poblacion, 50);
    check_int("ODE_DIM=30 sobreescribe la dimension", config.dimension, 30);
    check_int("ODE_GEN=77 sobreescribe las generaciones", config.max_generaciones, 77);
    check_bool("ODE_WARM_START=0 desactiva el warm start", !config.warm_start);

    // Con la dimension por defecto (20) el warm start si se acepta.
    ConfigBusqueda caliente = resolver_config(nullptr, nullptr, nullptr, "1");
    check_bool("ODE_WARM_START=1 activa el warm start con dimension 20",
               caliente.warm_start);
}

static void test_config_invalida_usa_respaldo() {
    // "abc" y "" no son numeros, "0" esta por debajo del minimo de
    // generaciones y "tal vez" no es una bandera: cada valor invalido cae a su
    // respaldo, la misma regla que usa resolver_semilla con ODE_SEED.
    ConfigBusqueda config = resolver_config("abc", "", "0", "tal vez");

    check_int("ODE_POP invalida usa el default 100", config.poblacion, 100);
    check_int("ODE_DIM invalida usa el default 20", config.dimension, 20);
    check_int("ODE_GEN invalida usa el default 5000", config.max_generaciones, 5000);
    check_bool("ODE_WARM_START invalida usa el default (apagado)",
               !config.warm_start);

    // Una poblacion menor que 4 no puede elegir 3 individuos base distintos
    // del objetivo (indices_distintos): el minimo aceptado es 4.
    check_int("ODE_POP=3 queda por debajo del minimo y usa el default",
              resolver_config("3", nullptr, nullptr, nullptr).poblacion, 100);
    check_int("ODE_POP=4 es aceptable",
              resolver_config("4", nullptr, nullptr, nullptr).poblacion, 4);

    // "17abc" no es un entero puro: se rechaza entero en vez de quedarse con 17.
    check_int("un texto a medias no es un entero valido",
              resolver_config("17abc", nullptr, nullptr, nullptr).poblacion, 100);
}

static void test_config_warm_start_requiere_dimension_20() {
    // Las cinco semillas warm start miden 20 tokens: con otra dimension no son
    // individuos validos para el emparejamiento gen a gen de
    // Mutation()/Recombination(), asi que la peticion se ignora (el valor
    // efectivo queda registrado en la linea de arranque).
    ConfigBusqueda config = resolver_config(nullptr, "30", nullptr, "1");

    check_int("la dimension pedida se conserva (30)", config.dimension, 30);
    check_bool("ODE_WARM_START=1 se ignora con ODE_DIM=30", !config.warm_start);
}

int main() {
    std::cout << "== test_population: issues #11 y #28 (semillero y configuracion) ==\n";
    test_createPopulation_tamano_uniforme();
    test_createPopulation_cantidad_exacta();
    test_createPopulation_respeta_la_dimension();
    test_warm_start_opt_in();
    test_createPopulation_reproducible();
    test_config_defaults();
    test_config_overrides();
    test_config_invalida_usa_respaldo();
    test_config_warm_start_requiere_dimension_20();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // como el exit code de pytest
}
