// test_grid.cpp — tests de la rejilla de evaluacion (issue #7).
//
// Para un desarrollador Python: este archivo es como un modulo `test_*.py` de
// pytest, pero sin framework: cada `test_*()` imprime PASS/FAIL y `main()` al
// final hace de resumen (seria el `pytest.main()` + exit code). El binario
// termina con codigo != 0 si algo fallo, igual que pytest.
//
// Se compila con -DODE_NO_MAIN (apaga el main() del programa y deja el de este
// archivo) y junto con dualfunctions.cpp; parse_tree.cpp se incluye mas abajo
// en vez de enlazarse, porque FunbObj no tiene header (ver el comentario del
// include).
// En Python no existe "dos main() en un mismo binario": aqui el preprocesador
// `#ifndef ODE_NO_MAIN` del programa deja al compilador elegir que main() incluir.

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>

#include "dualfunctions.h"  // necesita <vector>/<string> antes (header no autocontenido)

// En vez de declarar prototipos de funciones sueltas (como hacen test_mutation
// y test_crossover), este test incluye parse_tree.cpp entero: x_rango y
// FunbObj viven ahi sin header, y copiar la clase (con sus miembros privados)
// podria desincronizarse en silencio. Incluyendo el .cpp, el test compila el
// codigo real; el main() del programa queda fuera por -DODE_NO_MAIN (ver el
// target en el Makefile). En Python seria como `from parse_tree import
// x_rango, FunbObj`: importar el modulo real, no reescribirlo.
#include "parse_tree.cpp"

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_dual/test_mutation/test_crossover):
// contadores globales y helpers check_* que imprimen PASS/FAIL.
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
static void check_ld(const std::string& nombre, long double got, long double esperado,
                     long double tol = 1e-9L) {
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
// Slice 1: x_rango() produce la rejilla de evaluacion del paper — 100 puntos
// igualmente espaciados en [0,1], con ambos extremos incluidos. La formula
// esperada es x_i = start + (stop-start)*i/(npoints-1), i = 0..npoints-1
// (que en Python seria `[start + (stop-start)*i/(npoints-1) for i in
// range(npoints)]`); el test calcula los valores por su cuenta, sin llamar a
// la implementacion para obtener el valor esperado.
// -------------------------------------------------------------------------

static void test_x_rango_cantidad_de_puntos() {
    // El tercer argumento es un conteo: con 100 se obtienen exactamente 100
    // puntos (no 101, como daria un paso fijo de 0.01 en un intervalo cerrado).
    check_int("x_rango(0,1,100) produce 100 puntos",
              (int) x_rango(0.0L, 1.0L, 100).size(), 100);
    // El mismo contrato con otro conteo: la rejilla historica de 11 puntos.
    check_int("x_rango(0,1,11) produce 11 puntos",
              (int) x_rango(0.0L, 1.0L, 11).size(), 11);
}

static void test_x_rango_extremos_incluidos() {
    vector<vector<long double>> pts = x_rango(0.0L, 1.0L, 100);
    check_ld("primer punto es x=0", pts.front()[0], 0.0L);
    check_ld("ultimo punto es x=1", pts.back()[0], 1.0L);
}

static void test_x_rango_espaciado_uniforme() {
    vector<vector<long double>> pts = x_rango(0.0L, 1.0L, 100);
    // x_i = i/99 (paso 1/99 ≈ 0.0101, el "0.01 o similar" del issue). El
    // valor esperado se calcula aqui mismo a partir de la especificacion.
    bool uniforme = true;
    for (size_t i = 0; i < pts.size(); i++) {
        long double esperado = (long double) i / 99.0L;
        if (std::fabs(pts[i][0] - esperado) > 1e-12L) { uniforme = false; break; }
    }
    check_bool("x_i = i/99 para i=0..99 (paso uniforme 1/99)", uniforme);
}

static void test_x_rango_triple_dual() {
    // Cada punto es el 2dual [x, 1, 0]: x con dx/dx=1 y d2x/dx2=0 (eq. (14)
    // del paper). Toda la evaluacion de fitness depende de esta convencion.
    vector<vector<long double>> pts = x_rango(0.0L, 1.0L, 100);
    bool convencion = true;
    for (size_t i = 0; i < pts.size(); i++) {
        if (pts[i].size() != 3 || pts[i][1] != 1.0L || pts[i][2] != 0.0L) {
            convencion = false;
            break;
        }
    }
    check_bool("cada punto es [x, 1, 0] (dual de dx/dx)", convencion);
}

// -------------------------------------------------------------------------
// Slice 2: la configuracion de uso — FunbObj evalua la fitness sobre la
// rejilla de 100 puntos del paper, no sobre la historica de 11. Ejemplo
// trabajado con la expresion mas simple, `x` (token 11):
//     parsv([11], [x,1,0]) = [x,1,0]  =>  residuo (y''+y)^4 = (0+x)^4 = x^4
//     fitness(x) = 300*(0-1)^4 + 700*(1-1)^4 + sum_i (x_i)^4
//                = 300 + sum_{i=0}^{99} (i/99)^4
// El valor esperado se calcula aqui por su cuenta (300 y la suma), sin usar la
// implementacion; los pesos 300/700 y el exponente 4 son los de fitness().
// -------------------------------------------------------------------------

static void test_fitness_usa_la_rejilla_de_100_puntos() {
    FunbObj f({11}, 0.0L, 1.0L);
    long double esperado = 300.0L;  // penalizacion de y(0)=0: 300*(0-1)^4
    for (int i = 0; i < 100; i++) {
        long double x = (long double) i / 99.0L;
        esperado += std::pow(x, 4);  // (y''+y)^4 con y=x: x^4
    }
    check_ld("fitness(x) suma la rejilla de 100 puntos", f.fitness(), esperado, 1e-6L);
}

// -------------------------------------------------------------------------
// Slice 3: el Caso 1 del paper (ODE_CASE=1). La fitness usa el residuo de la
// ecuacion (18), x*y'' + y' - cos(x) = 0 (la forma multiplicada de (16) que
// el paper minimiza) y las condiciones iniciales y(0)=0, y'(0)=1.
// -------------------------------------------------------------------------

static void test_fitness_caso1_residuo_del_paper() {
    // Ejemplo trabajado con `x` (token 11): parsv([11], [x,1,0]) = [x,1,0] =>
    //     residuo = x*0 + 1 - cos(x) = 1 - cos(x)
    //     penalizaciones = 300*(0-0)^4 + 700*(1-1)^4 = 0
    // El valor esperado se calcula aqui por su cuenta (la suma), sin usar la
    // implementacion.
    FunbObj f({11}, 0.0L, 1.0L, 1);
    long double esperado = 0.0L;
    for (int i = 0; i < 100; i++) {
        long double x = (long double) i / 99.0L;
        esperado += std::pow(1.0L - std::cos(x), 4);
    }
    check_ld("fitness_caso1(x) usa x*y''+y'-cos(x)", f.fitness(), esperado, 1e-6L);
}

static void test_fitness_caso1_funcion_unaria() {
    // Segunda comprobacion con una funcion unaria, `sin(x)` (tokens 13,11):
    //     parsv(sin(x)) = [sin(x), cos(x), -sin(x)]
    //     residuo = x*(-sin(x)) + cos(x) - cos(x) = -x*sin(x)
    //     penalizaciones = 300*(sin 0 - 0)^4 + 700*(cos 0 - 1)^4 = 0
    // Pedir y(0)=0 hace que `sin(x)` sea una semilla natural del caso 1.
    FunbObj f({13, 11}, 0.0L, 1.0L, 1);
    long double esperado = 0.0L;
    for (int i = 0; i < 100; i++) {
        long double x = (long double) i / 99.0L;
        esperado += std::pow(-x * std::sin(x), 4);
    }
    check_ld("fitness_caso1(sin(x)) usa los pesos del caso 1", f.fitness(), esperado, 1e-6L);
}

// -------------------------------------------------------------------------
// Slice 4 (issue #20, B6 de REVIEW.md): fitness() no muta al individuo y se
// cachea. Antes, un arbol sin x recibia hasta 4 escrituras aleatorias de 11
// dentro de fitness() y la primera llamada devolvia INFINITY mientras una
// segunda podia devolver otro valor: la evaluacion cambiaba al individuo.
// -------------------------------------------------------------------------

static void test_fitness_sin_x_no_muta_el_individuo() {
    // 20 constantes (misma longitud que un individuo real), sin token x (11).
    srand(20261013);   // fija el sorteo de la reparacion heredada
    std::vector<long double> sin_x(20, 3.0L);
    FunbObj f(sin_x, 0.0L, 1.0L);
    std::vector<long double> antes = f.vecvalue();

    long double r1 = f.fitness();
    long double r2 = f.fitness();

    check_bool("fitness() de un arbol sin x no toca el vector",
               f.vecvalue() == antes);
    check_bool("fitness() de un arbol sin x es INFINITY", std::isinf(r1));
    check_bool("fitness() repetido devuelve lo mismo", r2 == r1);
}

static void test_fitness_valido_es_estable() {
    // sin(x) (tokens 13,11): fitness finita, y dos llamadas deben devolver
    // exactamente lo mismo (la cache no cambia el resultado).
    FunbObj f({13, 11}, 0.0L, 1.0L);
    long double r1 = f.fitness();
    check_bool("fitness() de un arbol valido es estable",
               !std::isnan(r1) && f.fitness() == r1);
}

// La radicanda (x - 1/2)^2 - 1/10 es positiva en x=0 y x=1, pero negativa
// cerca de x=1/2. Asi la expresion es evaluable en los extremos y produce
// NaN en puntos interiores, que debe invalidar al individuo completo.
static void test_fitness_nan_interior_es_infinita() {
    // En Python seria una lista de enteros: sqrt((x - 1/2)**2 - 1/10).
    std::vector<long double> nan_interior = {
        17, 18, 21, 18, 11, 12, 20, 1, 2, 2, 12, 20, 1, 10
    };
    FunbObj f(nan_interior, 0.0L, 1.0L);
    check_bool("fitness() con NaN interior es INFINITY",
               std::isinf(f.fitness()));
}

// NOTA (#10, REVIEW B1): la solucion aproximada del paper para el Caso 1,
//     y_s(x) = x*cos(x/(5*cos(cos 9)))   (ec. (20))
// en prefijo es {19,11,14,20,11,19,5,14,14,9}, y su fitness con este residuo
// es ~1.5e-8 (< 1e-5), asi que el caso es alcanzable CON ESTE ALFABETO. Hoy
// no se puede comprobar en un test: parsv() todavia no consume operandos
// (issue #10), y esa expresion se evalua mal (p. ej. `x/2` da NaN y
// `x*cos(x/5)` da el valor de `x*x`). Cuando #10 se cierre, el test natural
// es `check_bool("ec. (20) converge (<1e-5)", f.fitness() < 1e-5L)` sobre
// esos 10 tokens.

int main() {
    std::cout << "== test_grid: issue #7 (rejilla de evaluacion) ==\n";
    test_x_rango_cantidad_de_puntos();
    test_x_rango_extremos_incluidos();
    test_x_rango_espaciado_uniforme();
    test_x_rango_triple_dual();
    test_fitness_usa_la_rejilla_de_100_puntos();
    test_fitness_caso1_residuo_del_paper();
    test_fitness_caso1_funcion_unaria();
    test_fitness_sin_x_no_muta_el_individuo();
    test_fitness_valido_es_estable();
    test_fitness_nan_interior_es_infinita();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // como el exit code de pytest
}
