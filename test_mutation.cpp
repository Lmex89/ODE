// test_mutation.cpp — tests del operador de mutacion DE/rand/1/bin (issue #5).
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

#include "dualfunctions.h"  // necesita <vector>/<string> antes (header no autocontenido)
#include "token.h"          // expone el contrato publico del rango del alfabeto

// Prototipos de las costuras (seams) que se prueban: son funciones publicas de
// parse_tree.cpp que hoy no viven en un header, asi que el test las declara.
// Si alguien cambia una firma, este archivo deja de compilar (el fallo es
// ruidoso, no silencioso).
ldouble mutacion_rand1(ldouble r1, ldouble r2, ldouble r3, ldouble F);
int token_valido(ldouble v);
void indices_distintos(int tam, int excluir, int& r1, int& r2, int& r3);
vector<ldouble> Mutation(vector<ldouble>x, vector<ldouble> y, vector<ldouble>z);

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_dual.cpp): contadores globales y
// helpers check_* que imprimen PASS/FAIL.
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
                  << (double)got << " esperado=" << (double)esperado << ")\n";
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
// Slice 1: nucleo de la mutacion, ecuacion (7) del paper:
//     v = x_r1 + F * (x_r2 - x_r3)
// Los valores "esperados" son cuentas a mano con esa formula (fuente
// independiente del codigo): con F=0 el resultado es x_r1; con F=1 es
// x_r1 + (x_r2 - x_r3); con F=0.5, la mitad del camino.
// -------------------------------------------------------------------------

static void test_mutacion_rand1_formula() {
    check_ld("mutacion_rand1(5,9,4,F=0.5) = 7.5", mutacion_rand1(5.0L, 9.0L, 4.0L, 0.5L), 7.5L);
    check_ld("mutacion_rand1(5,9,4,F=0)   = 5",   mutacion_rand1(5.0L, 9.0L, 4.0L, 0.0L), 5.0L);
    check_ld("mutacion_rand1(5,9,4,F=1)   = 10",  mutacion_rand1(5.0L, 9.0L, 4.0L, 1.0L), 10.0L);
    // Diferencia negativa: 2 + 0.25*(1-10) = -0.25 (F puede alejar el token).
    check_ld("mutacion_rand1(2,1,10,F=0.25) = -0.25", mutacion_rand1(2.0L, 1.0L, 10.0L, 0.25L), -0.25L);
}

// -------------------------------------------------------------------------
// Slice 2: token_valido() — el paper pide convertir el real de la ecuacion (7)
// a un token entero del alfabeto (1..TOKENS_ALFABETO). Redondeo al entero mas cercano y
// recorte ("clamp") al rango; esperados calculados a mano.
// -------------------------------------------------------------------------

static void test_token_valido_rango() {
    check_int("token_valido(7.5)  = 8  (redondea hacia arriba)", token_valido(7.5L), 8);
    check_int("token_valido(7.4)  = 7  (redondea hacia abajo)",  token_valido(7.4L), 7);
    check_int("token_valido(11.0) = 11 (token intacto)",         token_valido(11.0L), 11);
    check_int("token_valido(TOKENS_ALFABETO) conserva el borde superior",
              token_valido(TOKENS_ALFABETO), TOKENS_ALFABETO);
    check_int("token_valido(30.2) recorta a TOKENS_ALFABETO",
              token_valido(30.2L), TOKENS_ALFABETO);
    check_int("token_valido(0.6)  = 1  (recorta por abajo)",     token_valido(0.6L), 1);
    check_int("token_valido(-0.25)= 1  (negativo recorta)",      token_valido(-0.25L), 1);
}

// -------------------------------------------------------------------------
// Slice 3: indices_distintos() — los tres vectores base (r1, r2, r3) deben ser
// individuos *distintos* de la poblacion y ninguno puede ser el individuo que
// se esta mutando. Se repite el sorteo muchas veces (semilla fija) y se
// verifican los invariantes, no el valor concreto (que es aleatorio).
// -------------------------------------------------------------------------

static void test_indices_distintos() {
    srand(20261012);  // semilla fija: mismo flujo de rand() en cada corrida

    bool rango_ok = true, distintos_ok = true;
    for (int k = 0; k < 1000; k++) {
        int r1, r2, r3;
        indices_distintos(4, 1, r1, r2, r3);  // solo 4 individuos y se excluye el 1
        if (r1 < 0 || r1 >= 4 || r2 < 0 || r2 >= 4 || r3 < 0 || r3 >= 4) rango_ok = false;
        if (r1 == r2 || r1 == r3 || r2 == r3) distintos_ok = false;
        if (r1 == 1 || r2 == 1 || r3 == 1) distintos_ok = false;  // es el individuo objetivo
    }
    check_bool("indices_distintos(4, excluir=1): los 3 indices caen en [0,4)", rango_ok);
    check_bool("indices_distintos(4, excluir=1): distintos entre si y != excluido", distintos_ok);

    // Con tam=8 la misma llamada debe producir combinaciones variadas (siempre
    // devolviera, por ejemplo, {0,1,2}, no seria aleatoria). Barajando 300
    // veces, la probabilidad de ver una sola combinacion es ~0.
    bool vario = false;
    int p1, p2, p3;
    indices_distintos(8, 0, p1, p2, p3);
    for (int k = 0; k < 300 && !vario; k++) {
        int r1, r2, r3;
        indices_distintos(8, 0, r1, r2, r3);
        if (r1 != p1 || r2 != p2 || r3 != p3) vario = true;
    }
    check_bool("indices_distintos(8, excluir=0): el sorteo varia entre llamadas", vario);
}

// -------------------------------------------------------------------------
// Slice 4: Mutation() de punta a punta. Aqui se prueba el comportamiento
// observable del operador completo: tamano, rango de tokens y cota de la
// perturbacion que impone F en [0,1].
// -------------------------------------------------------------------------

// vec_str: renderiza un vector de tokens para los mensajes de fallo (como
// `str(lista)` en Python).
static std::string vec_str(const std::vector<long double>& v) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < v.size(); i++) {
        os << (double)v[i];
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

// Si x_r2 y x_r3 son iguales, la ecuacion (7) queda v = x_r1 + F*0 = x_r1:
// la mutacion debe devolver al individuo base tal cual.
static void test_mutation_identidad_si_y_igual_z() {
    std::vector<long double> x = {11, 5, 3};
    std::vector<long double> y = {11, 7, 9};
    std::vector<long double> z = {11, 7, 9};
    check_vec("Mutation(x, y, y) devuelve x (F*0 = 0)", Mutation(x, y, z), x);
}

// F esta en [0,1], asi que cada token se mueve, como mucho, la distancia
// |y[i] - z[i]| (mas 0.5 por el redondeo). Con la mutacion ad-hoc anterior
// (|x-y| + z) esa cota no se cumplia.
static void test_mutation_vecindad_acotada() {
    srand(20261012);
    // x[0]=11 e y[0]=z[0]=11: el token x sobrevive siempre y no se dispara la
    // reparacion, asi la cota se puede medir sobre todo el vector.
    std::vector<long double> x = {11, 10, 2, 20, 5};
    std::vector<long double> y = {11, 4, 9, 3, 18};
    std::vector<long double> z = {11, 8, 1, 19, 6};

    std::vector<long double> res = Mutation(x, y, z);

    bool tamano_ok = (res.size() == x.size());
    check_bool("Mutation conserva el tamano del vector", tamano_ok);

    bool tokens_ok = true;
    for (size_t i = 0; i < res.size(); i++) {
        bool entero = (res[i] == floor(res[i]));
        if (!entero || res[i] < TOKEN_MIN || res[i] > TOKENS_ALFABETO) tokens_ok = false;
    }
    check_bool("Mutation devuelve tokens enteros en [TOKEN_MIN,TOKENS_ALFABETO]", tokens_ok);

    bool cota_ok = true;
    for (size_t i = 0; i < res.size(); i++) {
        ldouble cota = fabs(y[i] - z[i]) + 0.5L;  // 0.5 = redondeo al entero
        if (fabs(res[i] - x[i]) > cota) cota_ok = false;
    }
    check_bool("Mutation mueve cada token a lo sumo |y-z| (F en [0,1])", cota_ok);
}

// B5 de REVIEW.md: un individuo semilla tiene TOKENS_ALFABETO tokens y el
// resto 20; la mutacion no debe leer y[20]/z[20] (fuera de rango) y debe
// devolver un vector de TOKENS_ALFABETO tokens validos.
static void test_mutation_tamanos_distintos() {
    srand(20261012);
    std::vector<long double> x(TOKENS_ALFABETO, 11.0L);
    std::vector<long double> y(20, 5.0L);
    std::vector<long double> z(20, 3.0L);
    std::vector<long double> res = Mutation(x, y, z);

    check_bool("Mutation(x[TOKENS_ALFABETO], y[20], z[20]) conserva el tamano",
               res.size() == TOKENS_ALFABETO);
    bool tokens_ok = true;
    for (size_t i = 0; i < res.size(); i++) {
        if (res[i] < TOKEN_MIN || res[i] > TOKENS_ALFABETO || res[i] != floor(res[i])) {
            tokens_ok = false;
        }
    }
    check_bool("Mutation no inventa tokens invalidos con tamanos distintos", tokens_ok);
}

// Reparacion heredada: la fitness descarta los arboles sin token x (11), asi
// que la mutacion garantiza que siempre quede al menos uno.
static void test_mutation_conserva_token_x() {
    srand(20261012);
    std::vector<long double> x = {1, 2, 3, 4};
    std::vector<long double> y = {4, 3, 2, 1};
    std::vector<long double> z = {2, 2, 2, 2};
    std::vector<long double> res = Mutation(x, y, z);

    bool hay_x = false;
    for (size_t i = 0; i < res.size(); i++) {
        if (res[i] == 11.0L) hay_x = true;
    }
    check_bool("Mutation deja al menos un token x (11)", hay_x);
}

int main() {
    std::cout << "== test_mutation: issue #5 (DE/rand/1/bin) ==\n";
    test_mutacion_rand1_formula();
    test_token_valido_rango();
    test_indices_distintos();
    test_mutation_identidad_si_y_igual_z();
    test_mutation_vecindad_acotada();
    test_mutation_tamanos_distintos();
    test_mutation_conserva_token_x();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // como el exit code de pytest
}
