// test_token.cpp — contrato publico del alfabeto de tokens (issue #31).
//
// Para un desarrollador Python: este archivo cumple el papel de un modulo
// `test_token.py` sin pytest. Cada helper acumula fallos y `main()` devuelve
// un codigo distinto de cero, equivalente a `pytest.main()`.

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "token.h"

// Contadores del arnes minimo; son el equivalente a las variables globales que
// un modulo de pruebas Python actualizaria entre asserts.
static int g_checks{0};
static int g_fallos{0};

// El reloj de CPU es equivalente a `time.process_time()` en Python y evita que
// la hora de pared cambie el formato de los eventos del arnes.
static const std::clock_t g_log_t0{std::clock()};

// fmt_segundos aisla los manipuladores de formato en un stream temporal para
// que no alteren el siguiente `std::cout`, igual que formatear un f-string en
// Python sin cambiar el comportamiento de `print()` posterior.
static std::string fmt_segundos(double segundos) {
    std::ostringstream salida;
    salida << std::fixed << std::setprecision(2) << segundos;
    return salida.str();
}

// -DODE_NO_MAIN excluye los helpers de parse_tree.cpp junto con su `main()`,
// asi que este adaptador conserva aqui el mismo formato de log. Una referencia
// a `std::ostream` no tiene equivalente directo en Python: permite encadenar
// `<<`, parecido a escribir sobre el mismo logger.
static std::ostream& log_prueba(const char* nivel) {
    const double segundos_cpu = static_cast<double>(std::clock() - g_log_t0)
                              / static_cast<double>(CLOCKS_PER_SEC);
    std::cout << '[' << fmt_segundos(segundos_cpu) << "s] [" << nivel << "] ";
    return std::cout;
}

// registrar_resultado centraliza la salida del arnes para que cada evento use
// el formato estructurado del repositorio, como `logging.info()` en Python.
static void registrar_resultado(const std::string& caso, bool exito) {
    g_checks++;
    if (exito) {
        log_prueba("INFO ") << "token_test_assertion | case=" << caso
                             << " status=pass\n" << std::flush;
    } else {
        g_fallos++;
        log_prueba("WARN ") << "token_test_assertion | case=" << caso
                             << " status=fail\n" << std::flush;
    }
}

// check_int equivale a `assert got == esperado` en Python, sin detener las
// comprobaciones restantes cuando una falla.
static void check_int(const std::string& caso, int got, int esperado) {
    registrar_resultado(caso, got == esperado);
}

// check_str es el equivalente a `assert got == esperado` para `str` de Python.
static void check_str(const std::string& caso, const std::string& got,
                      const std::string& esperado) {
    registrar_resultado(caso, got == esperado);
}

// check_bool registra invariantes, equivalente a `assert condicion` en Python.
static void check_bool(const std::string& caso, bool condicion) {
    registrar_resultado(caso, condicion);
}

// Slice 1: los nombres publicos anclan los codigos que el alfabeto ya usa en
// los arboles en notacion prefija. Los literales son la fuente independiente
// de verdad: la tabla historica de tokens del proyecto (29 con las hojas
// polinomiales del #63 y las constantes pi/e del #64).
static void test_constantes_y_rangos() {
    check_int("constant_1", TOKEN_CONST1, 1);
    check_int("constant_2", TOKEN_CONST2, 2);
    check_int("constant_3", TOKEN_CONST3, 3);
    check_int("constant_4", TOKEN_CONST4, 4);
    check_int("constant_5", TOKEN_CONST5, 5);
    check_int("constant_6", TOKEN_CONST6, 6);
    check_int("constant_7", TOKEN_CONST7, 7);
    check_int("constant_8", TOKEN_CONST8, 8);
    check_int("constant_9", TOKEN_CONST9, 9);
    check_int("constant_10", TOKEN_CONST10, 10);
    check_int("variable_x", TOKEN_X, 11);
    check_int("unary_neg", TOKEN_NEG, 12);
    check_int("unary_sin", TOKEN_SIN, 13);
    check_int("unary_cos", TOKEN_COS, 14);
    check_int("unary_exp", TOKEN_EXP, 15);
    check_int("unary_log", TOKEN_LOG, 16);
    check_int("unary_sqrt", TOKEN_SQRT, 17);
    check_int("unary_tan", TOKEN_TAN, 22);
    check_int("unary_sinh", TOKEN_SINH, 23);
    check_int("unary_cosh", TOKEN_COSH, 24);
    check_int("unary_tanh", TOKEN_TANH, 25);
    check_int("leaf_x2", TOKEN_X2, 26);
    check_int("leaf_x3", TOKEN_X3, 27);
    check_int("leaf_pi", TOKEN_PI, 28);
    check_int("leaf_e", TOKEN_E, 29);
    check_int("binary_add", TOKEN_ADD, 18);
    check_int("binary_mul", TOKEN_MUL, 19);
    check_int("binary_div", TOKEN_DIV, 20);
    check_int("binary_pow", TOKEN_POW, 21);
    check_int("minimum", TOKEN_MIN, 1);
    check_int("maximum", TOKEN_MAX, 29);
    check_int("alphabet_size", TOKENS_ALFABETO, 29);
}

// Slice 2: la aridad publica sigue la gramatica prefija: hojas sin operandos,
// funciones unarias con uno y operaciones binarias con dos. Es la misma tabla
// que usaria un parser Python para saber cuantos hijos debe consumir.
static void test_aridad_de_todo_el_alfabeto() {
    check_int("arity_constant_1", arity(TOKEN_CONST1), 0);
    check_int("arity_constant_2", arity(TOKEN_CONST2), 0);
    check_int("arity_constant_3", arity(TOKEN_CONST3), 0);
    check_int("arity_constant_4", arity(TOKEN_CONST4), 0);
    check_int("arity_constant_5", arity(TOKEN_CONST5), 0);
    check_int("arity_constant_6", arity(TOKEN_CONST6), 0);
    check_int("arity_constant_7", arity(TOKEN_CONST7), 0);
    check_int("arity_constant_8", arity(TOKEN_CONST8), 0);
    check_int("arity_constant_9", arity(TOKEN_CONST9), 0);
    check_int("arity_constant_10", arity(TOKEN_CONST10), 0);
    check_int("arity_variable_x", arity(TOKEN_X), 0);
    check_int("arity_unary_neg", arity(TOKEN_NEG), 1);
    check_int("arity_unary_sin", arity(TOKEN_SIN), 1);
    check_int("arity_unary_cos", arity(TOKEN_COS), 1);
    check_int("arity_unary_exp", arity(TOKEN_EXP), 1);
    check_int("arity_unary_log", arity(TOKEN_LOG), 1);
    check_int("arity_unary_sqrt", arity(TOKEN_SQRT), 1);
    check_int("arity_unary_tan", arity(TOKEN_TAN), 1);
    check_int("arity_unary_sinh", arity(TOKEN_SINH), 1);
    check_int("arity_unary_cosh", arity(TOKEN_COSH), 1);
    check_int("arity_unary_tanh", arity(TOKEN_TANH), 1);
    check_int("arity_leaf_x2", arity(TOKEN_X2), 0);
    check_int("arity_leaf_x3", arity(TOKEN_X3), 0);
    check_int("arity_leaf_pi", arity(TOKEN_PI), 0);
    check_int("arity_leaf_e", arity(TOKEN_E), 0);
    check_int("arity_binary_add", arity(TOKEN_ADD), 2);
    check_int("arity_binary_mul", arity(TOKEN_MUL), 2);
    check_int("arity_binary_div", arity(TOKEN_DIV), 2);
    check_int("arity_binary_pow", arity(TOKEN_POW), 2);
}

// Slice 3: los templates de render se copian de la representacion historica
// del arbol. Los huecos `()` son el contrato que luego rellena el parser, como
// plantillas de strings que un programa Python interpolaria de izquierda a
// derecha.
static void test_render_de_todo_el_alfabeto() {
    check_str("render_constant_1", render(TOKEN_CONST1), "1.0");
    check_str("render_constant_2", render(TOKEN_CONST2), "2.0");
    check_str("render_constant_3", render(TOKEN_CONST3), "3.0");
    check_str("render_constant_4", render(TOKEN_CONST4), "4.0");
    check_str("render_constant_5", render(TOKEN_CONST5), "5.0");
    check_str("render_constant_6", render(TOKEN_CONST6), "6.0");
    check_str("render_constant_7", render(TOKEN_CONST7), "7.0");
    check_str("render_constant_8", render(TOKEN_CONST8), "8.0");
    check_str("render_constant_9", render(TOKEN_CONST9), "9.0");
    check_str("render_constant_10", render(TOKEN_CONST10), "10.0");
    check_str("render_variable_x", render(TOKEN_X), "x");
    check_str("render_unary_neg", render(TOKEN_NEG), "-1.0*()");
    check_str("render_unary_sin", render(TOKEN_SIN), "sin()");
    check_str("render_unary_cos", render(TOKEN_COS), "cos()");
    check_str("render_unary_exp", render(TOKEN_EXP), "exp()");
    check_str("render_unary_log", render(TOKEN_LOG), "log()");
    check_str("render_unary_sqrt", render(TOKEN_SQRT), "()**(1/2)");
    check_str("render_unary_tan", render(TOKEN_TAN), "tan()");
    check_str("render_unary_sinh", render(TOKEN_SINH), "sinh()");
    check_str("render_unary_cosh", render(TOKEN_COSH), "cosh()");
    check_str("render_unary_tanh", render(TOKEN_TANH), "tanh()");
    check_str("render_leaf_x2", render(TOKEN_X2), "x^2");
    check_str("render_leaf_x3", render(TOKEN_X3), "x^3");
    check_str("render_leaf_pi", render(TOKEN_PI), "pi");
    check_str("render_leaf_e", render(TOKEN_E), "e");
    check_str("render_binary_add", render(TOKEN_ADD), "()+()");
    check_str("render_binary_mul", render(TOKEN_MUL), "()*()");
    check_str("render_binary_div", render(TOKEN_DIV), "()/()");
    check_str("render_binary_pow", render(TOKEN_POW), "()**()");
}

// Slice 4: la validez se observa por el rango cerrado completo y por tres
// codigos externos. El bucle equivale a `for token in range(1, 23)` en Python.
static void test_validacion_del_alfabeto() {
    for (int token = TOKEN_MIN; token <= TOKEN_MAX; ++token) {
        check_bool("valid_known_" + std::to_string(token), is_valid(token));
    }
    check_bool("invalid_zero", !is_valid(0));
    check_bool("invalid_above_max", !is_valid(TOKEN_MAX + 1));
    check_bool("invalid_negative", !is_valid(-1));
}

// Slice 5: las categorias deben describir la tabla del dominio y formar una
// particion: cada codigo conocido entra en una sola. Los limites literales son
// la tabla publicada, no una llamada a las funciones bajo prueba.
static void test_categorias_particionan_el_alfabeto() {
    for (int token = TOKEN_MIN; token <= TOKEN_MAX; ++token) {
        // pi/e (28-29) entran en la categoria de constantes: son hojas de
        // valor fijo, igual que los literales 1.0-10.0; no dependen de x.
        const bool esperado_constante = (token >= 1 && token <= 10)
                                      || token == 28 || token == 29;
        const bool esperado_variable = token == 11;
        // Los literales (12-17 y 22-25 unarias; 18-21 binarias; 26-27 hojas
        // polinomiales) son la tabla publicada, no una llamada a las funciones
        // bajo prueba.
        const bool esperado_unario = (token >= 12 && token <= 17)
                                   || (token >= 22 && token <= 25);
        const bool esperado_binario = token >= 18 && token <= 21;
        const bool esperado_polinomio = token == 26 || token == 27;

        check_bool("category_constant_" + std::to_string(token),
                   is_constant(token) == esperado_constante);
        check_bool("category_variable_" + std::to_string(token),
                   is_variable(token) == esperado_variable);
        check_bool("category_unary_" + std::to_string(token),
                   is_unary(token) == esperado_unario);
        check_bool("category_binary_" + std::to_string(token),
                   is_binary(token) == esperado_binario);
        check_bool("category_polynomial_" + std::to_string(token),
                   is_polynomial(token) == esperado_polinomio);

        const int categorias = static_cast<int>(is_constant(token))
                             + static_cast<int>(is_variable(token))
                             + static_cast<int>(is_unary(token))
                             + static_cast<int>(is_binary(token))
                             + static_cast<int>(is_polynomial(token));
        check_int("category_partition_" + std::to_string(token), categorias, 1);
    }
}

// Slice issue #63: is_x_dependent responde si una hoja hace que la expresion
// dependa de x (la variable o una hoja polinomial); fitness() usa la misma
// pregunta para descartar los arboles constantes. Los literales 11/26/27 son
// la tabla publicada.
static void test_dependencia_de_x() {
    check_bool("x_dependent_variable", is_x_dependent(TOKEN_X));
    check_bool("x_dependent_leaf_x2", is_x_dependent(TOKEN_X2));
    check_bool("x_dependent_leaf_x3", is_x_dependent(TOKEN_X3));
    check_bool("constant_1_no_depende_de_x", !is_x_dependent(TOKEN_CONST1));
    check_bool("constant_pi_no_depende_de_x", !is_x_dependent(TOKEN_PI));
    check_bool("constant_e_no_depende_de_x", !is_x_dependent(TOKEN_E));
    check_bool("unary_sin_no_depende_de_x", !is_x_dependent(TOKEN_SIN));
    check_bool("binary_add_no_depende_de_x", !is_x_dependent(TOKEN_ADD));
}

// Slice 6: el sorteo no fija una secuencia concreta porque `rand()` depende de
// la plataforma. La semilla fija (como `random.seed(...)` en Python) permite
// repetir la prueba de su invariante: cada muestra pertenece al alfabeto.
static void test_sorteo_permanece_en_el_alfabeto() {
    std::srand(20261013);
    bool todos_validos = true;
    for (int intento = 0; intento < 1000; ++intento) {
        if (!is_valid(random_token())) {
            todos_validos = false;
        }
    }
    check_bool("random_tokens_valid", todos_validos);
}

int main() {
    log_prueba("INFO ") << "token_test_start | issue=31\n" << std::flush;
    test_constantes_y_rangos();
    test_aridad_de_todo_el_alfabeto();
    test_render_de_todo_el_alfabeto();
    test_validacion_del_alfabeto();
    test_categorias_particionan_el_alfabeto();
    test_dependencia_de_x();
    test_sorteo_permanece_en_el_alfabeto();

    if (g_fallos == 0) {
        log_prueba("INFO ") << "token_test_end | status=passed checks=" << g_checks
                             << " failures=" << g_fallos << "\n" << std::flush;
    } else {
        log_prueba("WARN ") << "token_test_end | status=failed checks=" << g_checks
                             << " failures=" << g_fallos << "\n" << std::flush;
    }
    return g_fallos == 0 ? 0 : 1;  // equivalente a `sys.exit(0 if ok else 1)`.
}
