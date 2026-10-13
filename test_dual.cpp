// =========================================================================
// test_dual.cpp - Tests de comportamiento de los dual numbers (issue #2).
//
// Para un desarrollador Python: este archivo cumple el rol de un
// `test_dual.py` con pytest. `make test` lo compila y lo corre; como C++ no
// trae pytest, cada `test_*()` imprime PASS/FAIL y el `main()` termina con
// codigo de salida != 0 si algo fallo (igual que un pytest que "falla").
//
// Los tests llaman SOLO a funciones publicas declaradas en dualfunctions.h
// (parsv, tan2dual, narg, parse_tree): son el "contrato" del modulo, no
// detalles internos. Los valores esperados salen de identidades matematicas
// conocidas (regla de la cadena, libm), NO de reimplementar el codigo.
// =========================================================================

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include "dualfunctions.h"

static int g_checks = 0;   // asserts ejecutados (como el contador de pytest)
static int g_fallos = 0;   // asserts fallidos

// vec_str: renderiza un vector de long double como texto, para el mensaje de
// fallo. En Python seria `str(lista)` con formato; `std::ostringstream` es
// como `io.StringIO()` (acumula texto sin imprimirlo todavia).
static std::string vec_str(const std::vector<long double>& v) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (std::isnan(v[i])) os << "nan";   // long double sin equivalente en Python
        else os << (double)v[i];             // cast solo para imprimir mas corto
        if (i + 1 < v.size()) os << ", ";
    }
    os << "]";
    return os.str();
}

// check_vec: compara `got` contra `esperado` componente a componente.
// `tol` absorbe el redondeo de punto flotante (como
// `pytest.approx(esperado, abs=1e-9)`): los valores no son identicos bit a
// bit porque libm calcula sin/cos con otro orden de operaciones.
// Un `nan` esperado exige un `nan` en el resultado (isnan, no ==).
static void check_vec(const std::string& nombre,
                      const std::vector<long double>& got,
                      const std::vector<long double>& esperado,
                      long double tol = 1e-9L) {
    g_checks++;
    bool ok = (got.size() == esperado.size());
    if (ok) {
        for (size_t i = 0; i < esperado.size(); i++) {
            if (std::isnan(esperado[i])) {
                if (!std::isnan(got[i])) { ok = false; break; }
            } else if (std::isnan(got[i]) || std::fabs(got[i] - esperado[i]) > tol) {
                ok = false;
                break;
            }
        }
    }
    if (ok) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre
                  << "  (got=" << vec_str(got)
                  << " esperado=" << vec_str(esperado) << ")\n";
        g_fallos++;
    }
}

// check_int: igual que `assert narg(12) == 1` en pytest.
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

// check_str: igual que `assert parse_tree(...) == "..."` en pytest.
// En C++ `std::string` es el `str` inmutable de Python; la comparacion ==
// compara contenido, igual que en Python.
static void check_str(const std::string& nombre, const std::string& got,
                      const std::string& esperado) {
    g_checks++;
    if (got == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "  (got=\"" << got
                  << "\" esperado=\"" << esperado << "\")\n";
        g_fallos++;
    }
}

// -------------------------------------------------------------------------
// Slice 1: token 12 (negacion) como funcion de 1 argumento.
// Criterio del issue #2: narg(12) debe ser 1 y parsv() debe procesar el
// token 12 en el segundo bucle negando su *argumento* (no la entrada).
// -------------------------------------------------------------------------

static void test_narg_token12() {
    check_int("narg(12) es 1 (negacion unaria)", narg(12.0L), 1);
}

static void test_parsv_negacion_de_sin() {
    // -sin(x) en x=1, por regla de la cadena:
    //   valor = -sin(1), f' = -cos(1), f'' = +sin(1).
    // Tokens en notacion prefija: 12 (negacion) 13 (sin) 11 (x).
    std::vector<long double> tokens  = {12, 13, 11};
    std::vector<long double> xval    = {1.0L, 1.0L, 0.0L};  // [x, dx/dx=1, 0]
    std::vector<long double> esperado = {-std::sin(1.0L), -std::cos(1.0L), std::sin(1.0L)};
    check_vec("parsv(-sin(x)) en x=1", parsv(tokens, xval), esperado);
}

// -------------------------------------------------------------------------
// Slice 2: tokens 13-21 no se inicializan como constantes.
// Criterio del issue #2: `x[k] >= 1 || x[k] <= 10` es siempre verdadera, asi
// que cada token (incluidos operadores y funciones) arrancaba con valor
// numerico = su codigo. Una expresion incompleta -aqui sin(+) sin operandos-
// debe quedar en NaN (no evaluable), no en sin(18).
// -------------------------------------------------------------------------

static void test_parsv_expresion_incompleta_es_nan() {
    std::vector<long double> tokens  = {13, 18};  // sin( + ) sin operandos
    std::vector<long double> xval    = {0.5L, 1.0L, 0.0L};
    std::vector<long double> esperado = {NAN, NAN, NAN};
    check_vec("parsv(expresion incompleta) queda NaN", parsv(tokens, xval), esperado);
}

// -------------------------------------------------------------------------
// Slice 3: primera derivada de tan en tan2dual().
// Criterio del issue #2: f[1] debe usar la componente dual xval[1] (= u',
// contador de la regla de la cadena) y no la componente real xval[0].
// Formulas: tan(u)' = u'/cos^2(u);
//           tan(u)'' = u''/cos^2(u) + 2*u'^2*tan(u)/cos^2(u).
// tan2dual no se alcanza desde parsv (la gramatica no tiene token tan), por
// eso se prueba llamandola directo, como cualquier funcion publica del .h.
// -------------------------------------------------------------------------

static void test_tan2dual_primera_derivada() {
    std::vector<long double> xval = {0.5L, 2.0L, 1.0L};  // [u, u', u'']
    long double c2 = std::cos(0.5L) * std::cos(0.5L);
    std::vector<long double> esperado = {
        std::tan(0.5L),                                  // tan(u)
        2.0L / c2,                                       // u'/cos^2(u)
        1.0L / c2 + (2.0L * std::tan(0.5L) / c2) * 4.0L  // u''/cos^2 + 2*u'^2*tan/cos^2
    };
    check_vec("tan2dual([0.5,2,1]) usa xval[1] en f'", tan2dual(xval), esperado);
}

// -------------------------------------------------------------------------
// Slice 4: locks de regresion. Estos casos ya daban bien antes de los
// arreglos; quedan fijados aqui para que un cambio futuro no los rompa.
// Cubren el criterio 5 ("sin(x) en x=0 -> [0,1,0]") y el resto de funciones
// conocidas del spec padre.
// -------------------------------------------------------------------------

static void test_narg_tabla() {
    check_int("narg(1)  = 0 (constante)", narg(1.0L), 0);
    check_int("narg(11) = 0 (variable x)", narg(11.0L), 0);
    check_int("narg(13) = 1 (seno)", narg(13.0L), 1);
    check_int("narg(17) = 1 (raiz)", narg(17.0L), 1);
    check_int("narg(18) = 2 (suma)", narg(18.0L), 2);
    check_int("narg(21) = 2 (potencia)", narg(21.0L), 2);
}

static void test_parsv_funciones_conocidas() {
    // x en x=0: [x, dx/dx, 0] = [0, 1, 0].
    std::vector<long double> e0 = {0.0L, 1.0L, 0.0L};
    check_vec("parsv(x)    en x=0", parsv({11}, e0), {0.0L, 1.0L, 0.0L});
    // sin(x) en 0 -> [0, 1, 0] (criterio explicito del issue #2).
    check_vec("parsv(sin(x)) en x=0", parsv({13, 11}, e0), {0.0L, 1.0L, 0.0L});
    // cos(x) en 0 -> [1, -sin(0), -cos(0)] = [1, 0, -1].
    check_vec("parsv(cos(x)) en x=0", parsv({14, 11}, e0), {1.0L, 0.0L, -1.0L});
    // cos(x)+sin(x) en 0 -> [1, -sin+cos, -cos-sin] = [1, 1, -1].
    check_vec("parsv(cos(x)+sin(x)) en x=0",
              parsv({18, 14, 11, 13, 11}, e0), {1.0L, 1.0L, -1.0L});
    // sin(x)*cos(x) en 1: (sin(2x)/2)' = cos(2x), (cos(2x))' = -2*sin(2x).
    check_vec("parsv(sin(x)*cos(x)) en x=1",
              parsv({19, 13, 11, 14, 11}, {1.0L, 1.0L, 0.0L}),
              {std::sin(1.0L) * std::cos(1.0L), std::cos(2.0L), -2.0L * std::sin(2.0L)});
    // -x en 1 -> [-x, -1, 0].
    check_vec("parsv(-x)   en x=1", parsv({12, 11}, {1.0L, 1.0L, 0.0L}),
              {-1.0L, -1.0L, 0.0L});
    // exp(x) en 1: todas las derivadas valen e.
    check_vec("parsv(exp(x)) en x=1", parsv({15, 11}, {1.0L, 1.0L, 0.0L}),
              {std::exp(1.0L), std::exp(1.0L), std::exp(1.0L)});
    // log(x) en 1 -> [0, 1/x, -1/x^2] = [0, 1, -1].
    check_vec("parsv(log(x)) en x=1", parsv({16, 11}, {1.0L, 1.0L, 0.0L}),
              {0.0L, 1.0L, -1.0L});
    // sqrt(x) en 4 -> [2, 1/(2*sqrt(x)), -1/(4*x^(3/2))] = [2, 1/4, -1/32].
    check_vec("parsv(sqrt(x)) en x=4", parsv({17, 11}, {4.0L, 1.0L, 0.0L}),
              {2.0L, 0.25L, -0.03125L});
}

static void test_parse_tree_token12() {
    // Criterio del issue #2: el render del token 12 debe ser consistente con
    // narg(12) == 1, es decir, un operador de un hueco "()" que el argumento
    // llena: -1.0*(x) para -x.
    std::vector<long double> neg_x = {12, 11};
    check_str("parse_tree([12,11]) = -1.0*(x)", parse_tree(neg_x), "-1.0*(x)");
    // Bonus: el ganador esperado del DE, cos(x)+sin(x).
    std::vector<long double> suma = {18, 14, 11, 13, 11};  // +(cos(x), sin(x))
    check_str("parse_tree([18,14,11,13,11])", parse_tree(suma), "(cos(x))+(sin(x))");
}

int main() {
    std::cout << "== test_dual: issue #2 ==\n";
    test_narg_token12();
    test_parsv_negacion_de_sin();
    test_parsv_expresion_incompleta_es_nan();
    test_tan2dual_primera_derivada();
    test_narg_tabla();
    test_parsv_funciones_conocidas();
    test_parse_tree_token12();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // codigo de salida como el de pytest
}
