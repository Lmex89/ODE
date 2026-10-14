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
#include "token.h"

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

// valor_parsv: observa solo f(x) a traves de la seam publica parsv(). En
// Python seria `return parsv(tokens, [x, 1.0, 0.0])[0]`; crear xval aqui evita
// que la prueba use una funcion interna de dualfunctions.cpp.
static long double valor_parsv(const std::vector<long double>& tokens,
                               long double x) {
    std::vector<long double> xval = {x, 1.0L, 0.0L};
    return parsv(tokens, xval)[0];
}

// check_parsv_finite_difference: compara las derivadas que devuelve parsv()
// con una aproximacion independiente centrada. Es equivalente a calcular en
// Python `(f(x+h)-f(x-h))/(2*h)` y `(f(x+h)-2*f(x)+f(x-h))/h**2`.
static void check_parsv_finite_difference(const std::string& nombre,
                                          const std::vector<long double>& tokens,
                                          long double x,
                                          long double tol) {
    const long double h = 1e-5L;
    std::vector<long double> xval = {x, 1.0L, 0.0L};
    std::vector<long double> centro = parsv(tokens, xval);
    long double menos = valor_parsv(tokens, x - h);
    long double mas = valor_parsv(tokens, x + h);
    long double primera = (mas - menos) / (2.0L * h);
    long double segunda = (mas - 2.0L * centro[0] + menos) / (h * h);
    check_vec(nombre, {centro[1], centro[2]}, {primera, segunda}, tol);
}

// -------------------------------------------------------------------------
// Slice issue #18: la regla de sin() debe coincidir con una diferencia finita
// calculada sin reutilizar el resultado de parsv().
// -------------------------------------------------------------------------

static void test_parsv_sin_finite_difference() {
    check_parsv_finite_difference("parsv(sin(x)) coincide con diferencias finitas",
                                  {13, 11}, 0.5L, 1e-6L);
}

// check_parsv_case: aplica una expresion a todos los puntos del dominio que
// comparte la bateria. Es como `for x in puntos: assert_case(...)` en Python;
// el nombre incluye el punto para localizar un fallo sin inspeccionar indices.
static void check_parsv_case(const std::string& nombre,
                             const std::vector<long double>& tokens,
                             const std::vector<long double>& puntos) {
    for (long double x : puntos) {
        std::ostringstream caso;
        caso << nombre << " en x=" << static_cast<double>(x);
        check_parsv_finite_difference(caso.str(), tokens, x, 1e-6L);
    }
}

// test_parsv_finite_difference_bateria: cubre cada operacion de la gramatica
// por la seam publica parsv(). Las expresiones usan notacion prefija, igual
// que la lista de tokens que evolucionan los individuos del DE.
static void test_parsv_finite_difference_bateria() {
    // Estos puntos mantienen positivas las bases de log/sqrt y evitan x=0 en
    // 2/x; asi la aproximacion central mide derivadas reales y finitas.
    std::vector<long double> puntos = {0.25L, 0.5L, 0.75L};
    check_parsv_case("parsv(x)", {11}, puntos);
    check_parsv_case("parsv(-x)", {12, 11}, puntos);
    check_parsv_case("parsv(sin(x))", {13, 11}, puntos);
    check_parsv_case("parsv(cos(x))", {14, 11}, puntos);
    check_parsv_case("parsv(exp(x))", {15, 11}, puntos);
    check_parsv_case("parsv(log(x+2))", {16, 18, 11, 2}, puntos);
    check_parsv_case("parsv(sqrt(x+2))", {17, 18, 11, 2}, puntos);
    check_parsv_case("parsv(x+2)", {18, 11, 2}, puntos);
    check_parsv_case("parsv(x-2)", {18, 11, 12, 2}, puntos);
    check_parsv_case("parsv(x*x)", {19, 11, 11}, puntos);
    check_parsv_case("parsv(x/2)", {20, 11, 2}, puntos);
    check_parsv_case("parsv(2/x)", {20, 2, 11}, puntos);
    check_parsv_case("parsv(x^2)", {21, 11, 2}, puntos);
    check_parsv_case("parsv(2^x)", {21, 2, 11}, puntos);
    check_parsv_case("parsv(x^x)", {21, 11, 11}, puntos);
    check_parsv_case("parsv(sin(x^2+cos(x)))",
                     {13, 18, 21, 11, 2, 14, 11}, puntos);
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

// Slice TDD del issue #32: un codigo fuera del alfabeto no tiene aridad.
// El resultado -1 es el contrato de `arity()` en token.h; en Python seria
// como obtener un sentinel explicito de una tabla en vez de leer una variable
// sin inicializar (que no tiene equivalente seguro en Python).
static void test_narg_codigo_invalido() {
    check_int("narg(0) devuelve -1 fuera del alfabeto", narg(0.0L), -1);
    check_int("narg(22) devuelve -1 fuera del alfabeto", narg(22.0L), -1);
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

// Slice TDD del renderizador: un token invalido usa la plantilla vacia del
// alfabeto. En Python seria como insertar `render(token)` y obtener `""` para
// un codigo desconocido, no reutilizar por accidente el token anterior.
static void test_parse_tree_codigo_invalido() {
    std::vector<long double> tokens = {TOKEN_ADD, TOKEN_CONST1, 0};
    check_str("parse_tree conserva el hueco de un token invalido",
              parse_tree(tokens), "(1.0)+()");
}

// -------------------------------------------------------------------------
// Slice 5 (issue #10): OperF2 consume sus operandos en el sitio.
// El parser de referencia (paper, Apendice A, Algorithm 1) evalua de derecha
// a izquierda y borra (componente real = NaN) cada operando que consume, para
// que la operacion siguiente solo vea los resultados de sus dos subarboles.
// OperF2 recibe la matriz completa y la posicion desde la que buscar
// (`inicio`), como el Fortran que pasa `auxv(:,k+1:length)` por referencia:
// en Python no vale una copia de la lista, hay que mutar la original.
// -------------------------------------------------------------------------

static void test_operf2_consumo_en_sitio() {
    // x*x en x=2 -> [4, 4, 2]; los dos operandos [2,1,0] quedan borrados
    // (NaN en su componente real) despues de la operacion.
    std::vector<std::vector<long double>> mat = {{2, 2}, {1, 1}, {0, 0}};
    check_vec("OperF2(19) calcula x*x", OperF2(19, mat), {4.0L, 4.0L, 2.0L});
    check_vec("OperF2(19) borra los operandos consumidos", mat[0], {NAN, NAN});
}

static void test_operf2_salta_huecos_nan() {
    // [5, NaN, 3, 7]: los dos primeros operandos no-NaN son 5 y 3; el 7
    // queda fuera (una operacion binaria solo come dos) y el hueco NaN no
    // se toca. Antes se borraba el vecino (j+1) y el escaneo paraba en
    // dim-1, asi que este caso ni se alcanzaba.
    std::vector<std::vector<long double>> mat = {{5, NAN, 3, 7}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    check_vec("OperF2(18) toma los dos primeros no-NaN", OperF2(18, mat),
              {8.0L, 0.0L, 0.0L});
    check_vec("OperF2(18) borra exactamente los consumidos", mat[0],
              {NAN, NAN, NAN, 7.0L});
}

static void test_operf2_inicio() {
    // `inicio` emula la seccion `auxv(:,k+1:length)` del Fortran: se busca
    // desde esa columna hacia la derecha y lo anterior no se toca. En Python
    // seria el equivalente a `for j in range(inicio, len(row))`.
    std::vector<std::vector<long double>> mat = {{9, 9, 5, 3}, {9, 9, 0, 0}, {9, 9, 0, 0}};
    check_vec("OperF2(18, inicio=2) opera desde la columna 2", OperF2(18, mat, 2),
              {8.0L, 0.0L, 0.0L});
    check_vec("OperF2(18, inicio=2) no toca lo anterior", mat[0],
              {9.0L, 9.0L, NAN, NAN});
}

// -------------------------------------------------------------------------
// Slice 6 (issue #10): parsv borra el operando que consume un unario.
// El operando de `sin`, `cos`, ... vive en la columna k+1; tras evaluarlo hay
// que escribir NaN en auxv[0][k+1] para que la operacion de la izquierda no
// lo vuelva a ver (Algorithm 1). Sin ese borrado, `-x+2` suma la hoja x dos
// veces y `sin(cos(x))+2` engancha cos(x) en vez del 2.
// -------------------------------------------------------------------------

static void test_parsv_consumo_unario() {
    // -x+2 en x=1 -> [1, -1, 0] (hoja x consumida por la negacion).
    std::vector<long double> x1 = {1.0L, 1.0L, 0.0L};
    check_vec("parsv(-x+2) en x=1", parsv({18, 12, 11, 2}, x1), {1.0L, -1.0L, 0.0L});
}

static void test_parsv_cadena_unaria() {
    // sin(cos(x))+2 en x=0.3. Valores esperados por regla de la cadena:
    //   f   = sin(cos(x)) + 2
    //   f'  = cos(cos(x)) * (-sin(x))
    //   f'' = -sin(cos(x))*sin(x)^2 - cos(cos(x))*cos(x)
    // (formulas cerradas de libm, independientes de la implementacion).
    long double x = 0.3L;
    std::vector<long double> esperado = {
        std::sin(std::cos(x)) + 2.0L,
        std::cos(std::cos(x)) * (-std::sin(x)),
        -std::sin(std::cos(x)) * std::sin(x) * std::sin(x) - std::cos(std::cos(x)) * std::cos(x)
    };
    check_vec("parsv(sin(cos(x))+2) en x=0.3", parsv({18, 13, 14, 11, 2}, {x, 1.0L, 0.0L}),
              esperado);
}

// -------------------------------------------------------------------------
// Slice 7 (issue #10): parsv ejecuta los binarios sobre la matriz completa.
// parsv ya no copia la sub-matriz a `auxv321`: le pasa auxv y k+1 para que
// el borrado de OperF2 caiga en el arbol real. La diferencia se ve cuando el
// operando consumido por un binario esta en medio del subarbol de otro:
// (x*x)+1 debe sumar 9+1, no 9+3 (la hoja x que el producto ya se comio).
// -------------------------------------------------------------------------

static void test_parsv_bateria_binaria() {
    std::vector<long double> x2 = {2.0L, 1.0L, 0.0L};
    // Bateria de aceptacion del issue #10; cada esperado es la formula
    // cerrada (libm), no una copia de la implementacion.
    check_vec("parsv(x*x) en x=2", parsv({19, 11, 11}, x2), {4.0L, 4.0L, 2.0L});
    check_vec("parsv(sin(x)+x) en x=2", parsv({18, 13, 11, 11}, x2),
              {std::sin(2.0L) + 2.0L, std::cos(2.0L) + 1.0L, -std::sin(2.0L)});
    check_vec("parsv(x/2) en x=2", parsv({20, 11, 2}, x2), {1.0L, 0.5L, 0.0L});
    check_vec("parsv(2/x) en x=2", parsv({20, 2, 11}, x2), {1.0L, -0.5L, 0.5L});
    check_vec("parsv(x^2) en x=2", parsv({21, 11, 2}, x2), {4.0L, 4.0L, 2.0L});
    check_vec("parsv(2^x) en x=2", parsv({21, 2, 11}, x2),
              {4.0L, 4.0L * std::log(2.0L),
               4.0L * std::log(2.0L) * std::log(2.0L)});
    check_vec("parsv(x+(x+x)) en x=2", parsv({18, 11, 18, 11, 11}, x2),
              {6.0L, 3.0L, 0.0L});
    // Expresion binaria incompleta: sin segundo operando -> NaN.
    check_vec("parsv(x*x sin segundo operando) es NaN", parsv({19, 11}, x2),
              {NAN, NAN, NAN});
}

static void test_parsv_binario_no_reutiliza_hoja() {
    // (x*x)+1 en x=3: el 9 consume las dos hojas x; la suma debe usar 9 y 1.
    // Con la copia de la sub-matriz, la segunda hoja x seguia viva y la suma
    // daba 12 (silenciosamente incorrecto).
    check_vec("parsv((x*x)+1) en x=3", parsv({18, 19, 11, 11, 1}, {3.0L, 1.0L, 0.0L}),
              {10.0L, 6.0L, 2.0L});
    // (2/x)+1 en x=2: 2/x = 1, +1 = 2 (con la hoja viva daria 3).
    check_vec("parsv((2/x)+1) en x=2", parsv({18, 20, 2, 11, 1}, {2.0L, 1.0L, 0.0L}),
              {2.0L, -0.5L, 0.5L});
}

// -------------------------------------------------------------------------
// Slice 8 (issue #10): parada temprana cuando una operacion da NaN.
// Algorithm 1 devuelve NaN en cuanto una operacion produce NaN, en vez de
// seguir evaluando. Con una cola de tokens de mas se nota: [18,16,11,14,11,2]
// es log(x)+cos(x) con un "2" de cola; log(-1) da NaN y, sin cortar, la suma
// salta ese NaN y engancha cos(-1) y la cola 2 (2.5403 en vez de NaN).
// -------------------------------------------------------------------------

static void test_parsv_parada_temprana_nan() {
    std::vector<long double> tokens = {18, 16, 11, 14, 11, 2};
    std::vector<long double> xval   = {-1.0L, 1.0L, 0.0L};  // log(-1) = NaN
    check_vec("parsv(operacion NaN) corta con NaN", parsv(tokens, xval),
              {NAN, NAN, NAN});
}

// -------------------------------------------------------------------------
// Slice 9 (issue #19, B8 de REVIEW.md): pow2dual con rama real y guardas.
// Antes la funcion calculaba con complejos y descartaba la parte imaginaria:
// casos con derivada bien definida daban NaN (x^2 en x=0 -> [0,NaN,NaN]) y los
// indefinidos no tenian una politica explicita. Los valores esperados salen de
// derivar a mano con la regla de la potencia, no de la implementacion.
// -------------------------------------------------------------------------

// p2d / sq2d: atajos locales que copian los vectores de entrada; pow2dual y
// sqrt2dual reciben referencias no-const y no aceptan temporales `{...}`
// (en Python no existe esa distincion: una lista se pasa sin ceremonia).
static std::vector<long double> p2d(std::vector<long double> u, std::vector<long double> v) {
    return pow2dual(u, v);
}
static std::vector<long double> sq2d(std::vector<long double> u) {
    return sqrt2dual(u);
}

static void test_pow2dual_rama_real() {
    // 2^3: f=8, f'=3*2^2=12, f''=6*2=12 (exponente constante).
    check_vec("pow2dual(2^3)", p2d({2.0L, 1.0L, 0.0L}, {3.0L, 0.0L, 0.0L}),
              {8.0L, 12.0L, 12.0L});
    // 4^0.5 = sqrt(4): f=2, f'=1/(2*sqrt(4))=0.25, f''=-1/(4*4^1.5)=-0.03125.
    check_vec("pow2dual(4^0.5)", p2d({4.0L, 1.0L, 0.0L}, {0.5L, 0.0L, 0.0L}),
              {2.0L, 0.25L, -0.03125L});
    // 2^x en x=2 (el exponente tambien varia): f'=f''=4*ln(2)^2, como en parsv.
    ldouble ln2 = std::log(2.0L);
    check_vec("pow2dual(2^x) con exponente variable",
              p2d({2.0L, 0.0L, 0.0L}, {2.0L, 1.0L, 0.0L}),
              {4.0L, 4.0L * ln2, 4.0L * ln2 * ln2});
}

static void test_pow2dual_base_negativa_entera() {
    // (-2)^3: f=-8, f'=3*(-2)^2=12, f''=6*(-2)=-12 (el signo de f'' tambien
    // es menos; el issue #1 lo escribio como +12, era un typo).
    check_vec("pow2dual((-2)^3)", p2d({-2.0L, 1.0L, 0.0L}, {3.0L, 0.0L, 0.0L}),
              {-8.0L, 12.0L, -12.0L});
    // (-2)^2: f=4, f'=2*(-2)=-4, f''=2*1*1=2.
    check_vec("pow2dual((-2)^2)", p2d({-2.0L, 1.0L, 0.0L}, {2.0L, 0.0L, 0.0L}),
              {4.0L, -4.0L, 2.0L});
    // (-2)^-1 = 1/u: f=-0.5, f'=-u'/u^2=-0.25, f''=2*u'^2/u^3=-0.25.
    check_vec("pow2dual((-2)^-1)", p2d({-2.0L, 1.0L, 0.0L}, {-1.0L, 0.0L, 0.0L}),
              {-0.5L, -0.25L, -0.25L});
}

static void test_pow2dual_indefinidos_son_nan() {
    // Base negativa con exponente fraccionario: sin resultado real.
    check_vec("pow2dual((-8)^(1/3)) es NaN",
              p2d({-8.0L, 1.0L, 0.0L}, {1.0L / 3.0L, 0.0L, 0.0L}), {NAN, NAN, NAN});
    check_vec("pow2dual((-4)^0.5) es NaN",
              p2d({-4.0L, 1.0L, 0.0L}, {0.5L, 0.0L, 0.0L}), {NAN, NAN, NAN});
    // Base negativa con exponente entero pero *variable*: ln(-u) no existe en
    // los reales, asi que la regla de la cadena no aplica -> NaN explicito.
    check_vec("pow2dual((-2)^(3+x)) es NaN",
              p2d({-2.0L, 1.0L, 0.0L}, {3.0L, 1.0L, 0.0L}), {NAN, NAN, NAN});
    // 0^0 es indeterminado (el limite depende de por donde se acerque).
    check_vec("pow2dual(0^0) es NaN",
              p2d({0.0L, 1.0L, 0.0L}, {0.0L, 0.0L, 0.0L}), {NAN, NAN, NAN});
}

static void test_pow2dual_base_cero_guardas() {
    // x^1 en x=0: base u=[0,1,0] (u'=1): f=0, f'=u'=1, f''=u''=0.
    check_vec("pow2dual(0^1)", p2d({0.0L, 1.0L, 0.0L}, {1.0L, 0.0L, 0.0L}),
              {0.0L, 1.0L, 0.0L});
    // x^2 en x=0: f=0, f'=0, f''=2*u'^2=2 (antes daba [0,NaN,NaN]).
    check_vec("pow2dual(0^2)", p2d({0.0L, 1.0L, 0.0L}, {2.0L, 0.0L, 0.0L}),
              {0.0L, 0.0L, 2.0L});
    // x^3 en x=0: f=f'=f''=0.
    check_vec("pow2dual(0^3)", p2d({0.0L, 1.0L, 0.0L}, {3.0L, 0.0L, 0.0L}),
              {0.0L, 0.0L, 0.0L});
    // x^0.5 en x=0: valor 0 pero derivadas realmente infinitas -> NaN.
    check_vec("pow2dual(0^0.5)", p2d({0.0L, 1.0L, 0.0L}, {0.5L, 0.0L, 0.0L}),
              {0.0L, NAN, NAN});
    // x^1.5 en x=0: f'=0 (1.5>1) pero f'' sigue siendo infinita -> NaN.
    check_vec("pow2dual(0^1.5)", p2d({0.0L, 1.0L, 0.0L}, {1.5L, 0.0L, 0.0L}),
              {0.0L, 0.0L, NAN});
    // x^-1 en x=0: 1/0, sin valor real -> NaN.
    check_vec("pow2dual(0^-1)", p2d({0.0L, 1.0L, 0.0L}, {-1.0L, 0.0L, 0.0L}),
              {NAN, NAN, NAN});
}

static void test_sqrt2dual_guardas() {
    // sqrt(u) es pow2dual(u, [0.5,0,0]): en u=4 todo finito; en u=0 la
    // derivada es infinita y queda NaN.
    check_vec("sqrt2dual(4)", sq2d({4.0L, 1.0L, 0.0L}), {2.0L, 0.25L, -0.03125L});
    check_vec("sqrt2dual(0)", sq2d({0.0L, 1.0L, 0.0L}), {0.0L, NAN, NAN});
}

// -------------------------------------------------------------------------
// Slice 10 (issue #26): la expresion renderizada es el arbol que se evalua.
// Cada caso fija por separado las dos observaciones publicas:
//   (a) el texto exacto que muestra parse_tree(), y
//   (b) el triple [y, y', y''] que parsv() devuelve segun esa misma formula.
// Los esperados de (b) son derivadas cerradas con libm (regla de la cadena,
// no una copia de la implementacion). Los casos anidan unario dentro de
// binario y al reves, reutilizan x varias veces y cubren los dos ordenes de
// cada operacion: un cambio que rompa el consumo exacto de operandos
// (issue #10) o el orden del render falla aqui.
// -------------------------------------------------------------------------

// check_render_y_triple: aplica a `tokens` las dos comprobaciones del issue.
// parse_tree() recibe `vector&` no-const y lo recorre (en Python seria una
// lista normal, sin esa distincion), asi que se le pasa una copia local y el
// vector original queda intacto para la llamada a parsv().
static void check_render_y_triple(const std::string& nombre,
                                  const std::vector<long double>& tokens,
                                  const std::string& render_esperado,
                                  long double x,
                                  const std::vector<long double>& esperado) {
    std::vector<long double> copia = tokens;   // equivalente a `tokens[:]` en Python
    check_str("parse_tree " + nombre, parse_tree(copia), render_esperado);
    check_vec("parsv " + nombre, parsv(tokens, {x, 1.0L, 0.0L}), esperado);
}

static void test_parsv_render_evaluacion_anidada() {
    // -(x+x) en x=1: la negacion envuelve a la suma completa -> [-2,-2,0].
    check_render_y_triple("-(x+x) en x=1", {12, 18, 11, 11},
                          "-1.0*((x)+(x))", 1.0L,
                          {-2.0L, -2.0L, 0.0L});
    // sin(x+cos(x)) en x=0: unario sobre binario.
    //   f'  = cos(x+cos(x))*(1-sin(x))
    //   f'' = -sin(x+cos(x))*(1-sin(x))^2 - cos(x+cos(x))*cos(x)
    long double s1 = std::sin(1.0L), c1 = std::cos(1.0L);
    check_render_y_triple("sin(x+cos(x)) en x=0", {13, 18, 11, 14, 11},
                          "sin((x)+(cos(x)))", 0.0L,
                          {s1, c1, -(s1 + c1)});
    // La misma forma con los operandos en orden distinto: ambos valen 6 en
    // x=2 y el render conserva el orden (una hoja x por cada factor).
    check_render_y_triple("(x*x)+x en x=2", {18, 19, 11, 11, 11},
                          "((x)*(x))+(x)", 2.0L, {6.0L, 5.0L, 2.0L});
    check_render_y_triple("x+(x*x) en x=2", {18, 11, 19, 11, 11},
                          "(x)+((x)*(x))", 2.0L, {6.0L, 5.0L, 2.0L});
    // Un binario con una funcion no conmutativa en cada lado; ambas valen
    // [1,1,-1] en x=0 porque cos(0)=1 y sin(0)=0.
    check_render_y_triple("(x+1)*cos(x) en x=0", {19, 18, 11, 1, 14, 11},
                          "((x)+(1.0))*(cos(x))", 0.0L, {1.0L, 1.0L, -1.0L});
    check_render_y_triple("cos(x)*(x+1) en x=0", {19, 14, 11, 18, 11, 1},
                          "(cos(x))*((x)+(1.0))", 0.0L, {1.0L, 1.0L, -1.0L});
    // log(exp(x)+x) en x=0: f=ln(e^x+x); f'=(e^x+1)/(e^x+x)=2 y
    // f''=(e^x*(e^x+x)-(e^x+1)^2)/(e^x+x)^2=-3.
    check_render_y_triple("log(exp(x)+x) en x=0", {16, 18, 15, 11, 11},
                          "log((exp(x))+(x))", 0.0L, {0.0L, 2.0L, -3.0L});
    // sqrt(x*x+2) en x=1 (sqrt se renderiza como **(1/2)):
    //   f=sqrt(3), f'=1/sqrt(3), f''=2/(3*sqrt(3)).
    long double r3 = std::sqrt(3.0L);
    check_render_y_triple("sqrt(x*x+2) en x=1", {17, 18, 19, 11, 11, 2},
                          "(((x)*(x))+(2.0))**(1/2)", 1.0L,
                          {r3, 1.0L / r3, 2.0L / (3.0L * r3)});
    // x repetida dentro de un divisor: x/(x+x) es la constante 0.5 y
    // (x+x)/x la constante 2; las derivadas solo se anulan si cada hoja x
    // se consume exactamente una vez.
    check_render_y_triple("x/(x+x) en x=2", {20, 11, 18, 11, 11},
                          "(x)/((x)+(x))", 2.0L, {0.5L, 0.0L, 0.0L});
    check_render_y_triple("(x+x)/x en x=2", {20, 18, 11, 11, 11},
                          "((x)+(x))/(x)", 2.0L, {2.0L, 0.0L, 0.0L});
    // 2^(x*x) en x=1: exponente variable.
    //   f' = f*(v'*ln2) = 4*ln2; f'' = f*(g^2+g') con g=v'*ln2 y g'=v''*ln2.
    long double ln2 = std::log(2.0L);
    check_render_y_triple("2^(x*x) en x=1", {21, 2, 19, 11, 11},
                          "(2.0)**((x)*(x))", 1.0L,
                          {2.0L, 4.0L * ln2, 8.0L * ln2 * ln2 + 4.0L * ln2});
    // (-x)*cos(x) en x=0: el unario es el operando izquierdo -> [0,-1,0].
    check_render_y_triple("(-x)*cos(x) en x=0", {19, 12, 11, 14, 11},
                          "(-1.0*(x))*(cos(x))", 0.0L, {0.0L, -1.0L, 0.0L});
    // -sin(x+1) en x=0: unario sobre el binario completo.
    check_render_y_triple("-sin(x+1) en x=0", {12, 13, 18, 11, 1},
                          "-1.0*(sin((x)+(1.0)))", 0.0L, {-s1, -c1, s1});
    // x+(-x) en x=2: una hoja x por rama; la suma exacta cancela todo.
    check_render_y_triple("x+(-x) en x=2", {18, 11, 12, 11},
                          "(x)+(-1.0*(x))", 2.0L, {0.0L, 0.0L, 0.0L});
    // x/(x+1) en x=1: f'=1/(x+1)^2, f''=-2/(x+1)^3.
    check_render_y_triple("x/(x+1) en x=1", {20, 11, 18, 11, 1},
                          "(x)/((x)+(1.0))", 1.0L, {0.5L, 0.25L, -0.25L});
    // exp(x^2+(x+x)) en x=0: g=x^2+2x, g(0)=0, g'(0)=2, g''(0)=2;
    //   f=e^g -> f'=2 y f''=g''+g'^2=6.
    check_render_y_triple("exp(x^2+(x+x)) en x=0", {15, 18, 21, 11, 2, 18, 11, 11},
                          "exp(((x)**(2.0))+((x)+(x)))", 0.0L, {1.0L, 2.0L, 6.0L});
    // sin(x*x)*x en x=0.5: unario a la izquierda de un producto.
    //   f=sin(x^2)*x; f'=2x^2*cos(x^2)+sin(x^2);
    //   f''=6x*cos(x^2)-4x^3*sin(x^2).
    long double x = 0.5L;
    long double sxx = std::sin(x * x), cxx = std::cos(x * x);
    check_render_y_triple("sin(x*x)*x en x=0.5", {19, 13, 19, 11, 11, 11},
                          "(sin((x)*(x)))*(x)", x,
                          {x * sxx, 2.0L * x * x * cxx + sxx,
                           6.0L * x * cxx - 4.0L * x * x * x * sxx});
    // x-sin(x*x) en x=0.5: la resta es una suma con negacion; la hoja x del
    // minuendo no la consume el sin del sustraendo.
    //   f=x-sin(x^2); f'=1-2x*cos(x^2); f''=4x^2*sin(x^2)-2*cos(x^2).
    check_render_y_triple("x-sin(x*x) en x=0.5", {18, 11, 12, 13, 19, 11, 11},
                          "(x)+(-1.0*(sin((x)*(x))))", x,
                          {x - sxx, 1.0L - 2.0L * x * cxx,
                           4.0L * x * x * sxx - 2.0L * cxx});
    // sin(x*x)+2 en x=0.5: si el producto no consumiera sus dos hojas, la
    // suma engancharia una x ya usada en vez del 2.0 independiente del final
    // (daria sin(x^2)+x, que en x=0.5 vale distinto).
    //   f=sin(x^2)+2; f'=2x*cos(x^2); f''=2*cos(x^2)-4x^2*sin(x^2).
    check_render_y_triple("sin(x*x)+2 en x=0.5", {18, 13, 19, 11, 11, 2},
                          "(sin((x)*(x)))+(2.0)", x,
                          {sxx + 2.0L, 2.0L * x * cxx,
                           2.0L * cxx - 4.0L * x * x * sxx});
    // -cos(x)+1 en x=2: si el unario no consumiera su operando, la suma
    // engancharia la x de dentro del cos en vez del 1.0 final.
    //   f=1-cos(x); f'=sin(x); f''=cos(x).
    check_render_y_triple("-cos(x)+1 en x=2", {18, 12, 14, 11, 1},
                          "(-1.0*(cos(x)))+(1.0)", 2.0L,
                          {1.0L - std::cos(2.0L), std::sin(2.0L), std::cos(2.0L)});
}

int main() {
    std::cout << "== test_dual: issue #2 ==\n";
    test_narg_token12();
    test_parsv_negacion_de_sin();
    test_parsv_expresion_incompleta_es_nan();
    test_tan2dual_primera_derivada();
    test_narg_tabla();
    test_narg_codigo_invalido();
    test_parsv_funciones_conocidas();
    test_parse_tree_token12();
    test_parse_tree_codigo_invalido();
    test_operf2_consumo_en_sitio();
    test_operf2_salta_huecos_nan();
    test_operf2_inicio();
    test_parsv_consumo_unario();
    test_parsv_cadena_unaria();
    test_parsv_bateria_binaria();
    test_parsv_binario_no_reutiliza_hoja();
    test_parsv_parada_temprana_nan();
    test_pow2dual_rama_real();
    test_pow2dual_base_negativa_entera();
    test_pow2dual_indefinidos_son_nan();
    test_pow2dual_base_cero_guardas();
    test_sqrt2dual_guardas();
    test_parsv_render_evaluacion_anidada();
    test_parsv_sin_finite_difference();
    test_parsv_finite_difference_bateria();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // codigo de salida como el de pytest
}
