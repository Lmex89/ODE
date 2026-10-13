// test_population.cpp — tests del semillero de la poblacion (issue #11).
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

#include "dualfunctions.h"  // necesita <vector>/<string> antes (header no autocontenido)

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

    // `poblacion` entra por referencia y createPopulation la llena con
    // push_back (en Python seria `poblacion = []` pasada por referencia).
    vector<FunbObj> poblacion;
    createPopulation(poblacion, 10, 0);

    check_bool("createPopulation produce individuos", !poblacion.empty());

    // vecvalue() es la via publica de lectura; _vecval es privado (Python no
    // tiene esta distincion: todo seria publico).
    bool todos_20 = true;
    for (size_t i = 0; i < poblacion.size(); i++) {
        if (poblacion[i].vecvalue().size() != 20) todos_20 = false;
    }
    check_bool("todos los individuos tienen 20 tokens (semillero uniforme)", todos_20);
}

// -------------------------------------------------------------------------
// Slice 2 (issue #28): createPopulation crea EXACTAMENTE los individuos que
// pide su parametro `tam`. Antes el bucle aleatorio era `i < tam - 4` con 5
// semillas fijas, asi que una poblacion de 10 salia de 11 individuos (el
// off-by-one que reporta REVIEW C5: 5 + tam - 4 = tam + 1).
// -------------------------------------------------------------------------

static void test_createPopulation_cantidad_exacta() {
    srand(20261013);

    vector<FunbObj> poblacion;
    createPopulation(poblacion, 10, 0);

    // 10 es un literal independiente de la implementacion: el contrato es
    // "tantos individuos como pidas", no "los que salgan de la formula".
    check_bool("createPopulation(10) crea exactamente 10 individuos",
               poblacion.size() == 10);
}

int main() {
    std::cout << "== test_population: issue #11 (uniformidad del semillero) ==\n";
    test_createPopulation_tamano_uniforme();
    test_createPopulation_cantidad_exacta();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // como el exit code de pytest
}
