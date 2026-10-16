// test_crossover.cpp — tests del cruce binomial DE/rand/1/bin (issue #6).
//
// Para un desarrollador Python: este archivo es como un modulo `test_*.py` de
// pytest, pero sin framework: cada `test_*()` imprime PASS/FAIL y `main()` al
// final hace de resumen (seria el `pytest.main()` + exit code). El binario
// termina con codigo != 0 si algo fallo, igual que pytest.
//
// Se compila junto con parse_tree.cpp (con -DODE_NO_MAIN para apagar su main()
// y quedarnos con el main() de este archivo) y dualfunctions.cpp.
// En Python no existe "enlazar dos modulos con dos main()": aqui el preprocesador
// `#ifndef ODE_NO_MAIN` del programa deja al compilador elegir que main() incluir.

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ctime>

#include "dualfunctions.h"  // ahora autocontenido: incluye <vector>/<string> por si mismo
#include "token.h"          // expone el contrato publico del rango del alfabeto

using namespace std;  // el header ya no inyecta nombres; el test los necesita para los prototipos

// Prototipos de las costuras (seams) que se prueban: son funciones publicas de
// parse_tree.cpp que hoy no viven en un header, asi que el test las declara.
// Si alguien cambia una firma, este archivo deja de compilar (el fallo es
// ruidoso, no silencioso).
ldouble gen_binomial(ldouble objetivo, ldouble mutante, ldouble r, ldouble Cr);

// Recombination devuelve el vector trial (no muta a los padres): los parametros
// entran por referencia constante (`const vector<ldouble>&`), que en Python
// seria pasar la lista sin copiarla y sin poder modificarla.
vector<ldouble> Recombination(const vector<ldouble>& objetivo, const vector<ldouble>& mutante, ldouble Cr);

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_dual.cpp/test_mutation.cpp): contadores
// globales y helpers check_* que imprimen PASS/FAIL.
// -------------------------------------------------------------------------

static int g_checks = 0;   // total de asserts ejecutados
static int g_fallos = 0;   // asserts que fallaron

// check_int: como `assert got == esperado` en pytest, pero sin abortar.
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

// check_ld: compara long doubles con tolerancia, como
// `pytest.approx(esperado, abs=1e-9)`. long double no tiene equivalente en
// Python (es mas ancho que float); la tolerancia absorbe el redondeo binario.
static void check_ld(const std::string& nombre, ldouble got, ldouble esperado,
                     ldouble tol = 1e-9L) {
    g_checks++;
    bool ok = !std::isnan(got) && std::fabs(got - esperado) <= tol;
    if (ok) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "  (got=" << std::setprecision(17)
                  << static_cast<double>(got) << " esperado=" << static_cast<double>(esperado) << ")\n";
        g_fallos++;
    }
}

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
// Slice 1: nucleo del cruce, ecuacion (8) del paper: cada gen del trial sale
// del mutante con probabilidad Cr y del objetivo en caso contrario.
// gen_binomial() es la decision pura de un gen dada su tirada r; los valores
// "esperados" son cuentas a mano de esa regla (fuente independiente del
// codigo), incluyendo los bordes Cr=0 y Cr=1 y la comparacion estricta r<Cr.
// -------------------------------------------------------------------------

static void test_gen_binomial_decide_por_cr() {
    // r < Cr -> gen del mutante (v); r >= Cr -> gen del objetivo (x).
    check_ld("gen_binomial(x=3, v=7, r=0.1, Cr=0.2) = 7", gen_binomial(3.0L, 7.0L, 0.1L, 0.2L), 7.0L);
    check_ld("gen_binomial(x=3, v=7, r=0.9, Cr=0.2) = 3", gen_binomial(3.0L, 7.0L, 0.9L, 0.2L), 3.0L);
    // La comparacion es estricta (r < Cr): con r == Cr gana el objetivo.
    check_ld("gen_binomial(x=3, v=7, r=0.2, Cr=0.2) = 3 (r<Cr estricto)", gen_binomial(3.0L, 7.0L, 0.2L, 0.2L), 3.0L);
    // Bordes: Cr=0 nunca toma el mutante; Cr=1 lo toma siempre (r<1).
    check_ld("gen_binomial(x=3, v=7, r=0.0, Cr=0.0) = 3 (Cr=0)", gen_binomial(3.0L, 7.0L, 0.0L, 0.0L), 3.0L);
    check_ld("gen_binomial(x=3, v=7, r=0.999, Cr=1.0) = 7 (Cr=1)", gen_binomial(3.0L, 7.0L, 0.999L, 1.0L), 7.0L);
}

// -------------------------------------------------------------------------
// Slice 2: Recombination() de punta a punta. Se prueba el comportamiento
// observable del operador con semilla fija (srand): tamano del trial, que
// cada gen salga de uno de los dos padres, el indice forzado j_rand del paper
// (REVIEW D2), la fraccion de genes del mutante (~Cr) y la reparacion heredada
// del token x (11).
// -------------------------------------------------------------------------

// vec_str: renderiza un vector de tokens para los mensajes de fallo (como
// `str(lista)` en Python).
static std::string vec_str(const std::vector<long double>& v) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < v.size(); i++) {
        os << static_cast<double>(v[i]);
        if (i + 1 < v.size()) os << ", ";
    }
    os << "]";
    return os.str();
}

// check_vec: igualdad exacta componente a componente (los tokens son enteros,
// no hace falta tolerancia); el tamano tambien debe coincidir.
static void check_vec(const std::string& nombre,
                      const std::vector<long double>& got,
                      const std::vector<long double>& esperado) {
    g_checks++;
    bool ok = (got.size() == esperado.size());
    if (ok) {
        for (size_t i = 0; i < esperado.size(); i++) {
            if (got[i] != esperado[i]) { ok = false; break; }
        }
    }
    if (ok) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "  (got=" << vec_str(got)
                  << " esperado=" << vec_str(esperado) << ")\n";
        g_fallos++;
    }
}

// Con Cr=1 el cruce toma el gen del mutante en cada posicion (r < 1 siempre
// que el sorteo no toque el maximo), asi que el trial debe ser el mutante tal
// cual. Los dos padres comparten un token x (11) para que la reparacion no
// altere el resultado.
static void test_recombination_cr1_toma_todo_el_mutante() {
    srand(20261012);
    std::vector<long double> objetivo = {18, 3, 5, 11, 9};  // TOKEN_ADD al inicio, TOKEN_X presente
    std::vector<long double> mutante  = {18, 4, 6, 11, 10};
    check_vec("Recombination(objetivo, mutante, Cr=1) == mutante",
              Recombination(objetivo, mutante, 1.0L), mutante);
}

// Con Cr=0 ningun gen se sortea a favor del mutante, pero el paper exige que
// j_rand (un indice forzado) tome el gen del mutante: el trial debe diferir
// del objetivo exactamente en esa posicion. El objetivo lleva dos tokens x
// para que, pase lo que pase con j_rand, la reparacion no dispare.
static void test_recombination_cr0_forzado_jrand() {
    srand(20261012);
    std::vector<long double> objetivo = {18, 3, 11, 7, 9};  // TOKEN_ADD al inicio (operador)
    std::vector<long double> mutante  = {18, 4, 6, 8, 10};  // TOKEN_ADD al inicio (operador)

    std::vector<long double> trial = Recombination(objetivo, mutante, 0.0L);

    check_bool("Recombination con Cr=0 conserva el tamano del objetivo",
               trial.size() == objetivo.size());

    int difs = 0;
    bool forzado_del_mutante = true;
    for (size_t i = 0; i < trial.size(); i++) {
        if (trial[i] != objetivo[i]) {
            difs++;
            if (trial[i] != mutante[i]) forzado_del_mutante = false;
        }
    }
    check_int("Cr=0: difiere del objetivo solo en la posicion forzada", difs, 1);
    check_bool("Cr=0: la posicion forzada toma el gen del mutante",
               forzado_del_mutante);
}

// Invariante principal del cruce: cada gen del trial sale del objetivo o del
// mutante, nunca de otra parte. Se aprovecha para medir la proporcion de genes
// del mutante: con Cr=0.2 y 20 genes debe rondar 0.2 (mas el aporte de j_rand,
// que fuerza un gen: 0.2 + 0.8/20 = 0.24). Los padres comparten el token x en
// la posicion 0 (fuera del conteo) para que la reparacion nunca dispare.
static void test_recombination_genes_de_los_padres_y_fraccion_cr() {
    srand(20261012);
    std::vector<long double> objetivo(20, 1.0L);
    std::vector<long double> mutante(20, 2.0L);
    objetivo[0] = 18.0L;  // TOKEN_ADD (operador) en posicion 0
    mutante[0]  = 18.0L;  // TOKEN_ADD (operador) en posicion 0
    objetivo[1] = 11.0L;  // TOKEN_X para evitar la reparacion de x
    mutante[1]  = 11.0L;  // TOKEN_X para evitar la reparacion de x

    const int repeticiones = 200;
    long total = 0, del_mutante = 0;
    bool solo_de_los_padres = true;

    for (int k = 0; k < repeticiones; k++) {
        std::vector<long double> trial = Recombination(objetivo, mutante, 0.2L);
        for (size_t i = 1; i < trial.size(); i++) {
            total++;
            if (trial[i] == 2.0L) {
                del_mutante++;
            } else if (trial[i] != objetivo[i] && trial[i] != mutante[i]) {
                solo_de_los_padres = false;
            }
        }
    }

    check_bool("cada gen del trial sale del objetivo o del mutante",
               solo_de_los_padres);
    double fraccion = static_cast<double>(del_mutante) / static_cast<double>(total);
    check_bool("fraccion de genes del mutante cerca de Cr=0.2 (esperado ~0.24)",
               fraccion > 0.18 && fraccion < 0.30);
}

// B5 de REVIEW.md: un individuo semilla tiene TOKENS_ALFABETO tokens y el resto 20. El
// trial debe conservar el tamano del objetivo y sus tokens sobrantes; a los
// padres les falta esa cola, asi que no pueden aportarla (j_rand solo apunta a
// las posiciones 0..19, las comunes a ambos).
static void test_recombination_tamano_y_cola_del_objetivo() {
    srand(20261012);
    std::vector<long double> objetivo(TOKENS_ALFABETO, 5.0L);
    std::vector<long double> mutante(20, 7.0L);
    objetivo[0] = 11.0L;
    mutante[0]  = 11.0L;
    objetivo[TOKENS_ALFABETO - 1] = TOKEN_POW;  // cola propia fuera del alcance del cruce

    std::vector<long double> trial = Recombination(objetivo, mutante, 0.2L);

    check_bool("Recombination(objetivo[TOKENS_ALFABETO], mutante[20]) conserva el tamano",
               trial.size() == TOKENS_ALFABETO);
    check_bool("el token extra del objetivo sobrevive al cruce",
               trial.size() == TOKENS_ALFABETO
                   && trial[TOKENS_ALFABETO - 1] == TOKEN_POW);
}

// Reparacion heredada: la fitness descarta los arboles sin token x (11), asi
// que el trial garantiza al menos uno aunque ningun padre lo tenga.
static void test_recombination_repara_token_x() {
    srand(20261012);
    std::vector<long double> objetivo = {1, 2, 3, 4};
    std::vector<long double> mutante  = {TOKEN_POW, TOKEN_DIV, TOKEN_MUL, TOKEN_ADD};
    std::vector<long double> trial = Recombination(objetivo, mutante, 0.2L);

    bool hay_x = false;
    for (size_t i = 0; i < trial.size(); i++) {
        if (trial[i] == 11.0L) hay_x = true;
    }
    check_bool("Recombination deja al menos un token x (11)", hay_x);
}

int main() {
    std::cout << "== test_crossover: issue #6 (cruce binomial) ==\n";
    test_gen_binomial_decide_por_cr();
    test_recombination_cr1_toma_todo_el_mutante();
    test_recombination_cr0_forzado_jrand();
    test_recombination_genes_de_los_padres_y_fraccion_cr();
    test_recombination_tamano_y_cola_del_objetivo();
    test_recombination_repara_token_x();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // como el exit code de pytest
}
