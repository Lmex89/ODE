#ifndef TOKEN_H
#define TOKEN_H

#include <cstdlib>
#include <string>

// `constexpr` crea constantes comprobadas en compilacion; es el equivalente a
// constantes de modulo en Python, pero C++ puede usarlas sin costo en runtime.
constexpr int TOKEN_CONST1{1};
constexpr int TOKEN_CONST2{2};
constexpr int TOKEN_CONST3{3};
constexpr int TOKEN_CONST4{4};
constexpr int TOKEN_CONST5{5};
constexpr int TOKEN_CONST6{6};
constexpr int TOKEN_CONST7{7};
constexpr int TOKEN_CONST8{8};
constexpr int TOKEN_CONST9{9};
constexpr int TOKEN_CONST10{10};

// TOKEN_X es la unica hoja variable; las constantes anteriores son hojas
// numericas. Igual que enteros de modulo en Python, no poseen estado mutable.
constexpr int TOKEN_X{11};

// Operaciones de un solo operando, como funciones Python que reciben un valor.
constexpr int TOKEN_NEG{12};
constexpr int TOKEN_SIN{13};
constexpr int TOKEN_COS{14};
constexpr int TOKEN_EXP{15};
constexpr int TOKEN_LOG{16};
constexpr int TOKEN_SQRT{17};
// TOKEN_TAN abre el bloque de funciones unarias nuevas al final del alfabeto
// (tan, sinh, cosh, tanh); el rango contiguo puede ampliarse incrementando
// TOKENS_ALFABETO. En Python seria agregar `tan` al registro de funciones y a
// su tabla de aridades.
constexpr int TOKEN_TAN{22};
// Las funciones hiperbolicas (issue #61) continuan el rango contiguo de
// unarias: sinh, cosh y tanh. En Python seria agregarlas al mismo registro de
// funciones y a su tabla de aridades.
constexpr int TOKEN_SINH{23};
constexpr int TOKEN_COSH{24};
constexpr int TOKEN_TANH{25};
// Hojas polinomiales (issue #63): x^2 y x^3 se evaluan como x*x y x*x*x, con
// aridad 0 (no consumen operandos del arbol). En Python serian dos entradas
// mas del registro de hojas cuyo valor depende de x.
constexpr int TOKEN_X2{26};
constexpr int TOKEN_X3{27};
// Constantes matematicas (issue #64): pi y e son hojas de valor fijo (aridad
// 0, cero derivadas) que amplian las constantes numericas 1.0-10.0. En Python
// serian dos entradas mas del registro de hojas, aqui sin depender de x.
constexpr int TOKEN_PI{28};
constexpr int TOKEN_E{29};

// Operaciones de dos operandos; en notacion prefija anteceden sus dos hijos.
constexpr int TOKEN_ADD{18};
constexpr int TOKEN_MUL{19};
constexpr int TOKEN_DIV{20};
constexpr int TOKEN_POW{21};

// TOKENS_ALFABETO es la cantidad de codigos contiguos disponibles. En Python
// equivale a `len(range(TOKEN_MIN, TOKEN_MAX + 1))`; agregar un token nuevo
// solo requiere ampliar este contador y su implementacion.
constexpr int TOKENS_ALFABETO{29};
constexpr int TOKEN_MIN{TOKEN_CONST1};
constexpr int TOKEN_MAX{TOKEN_MIN + TOKENS_ALFABETO - 1};

// is_valid comprueba pertenencia al alfabeto contiguo, igual que `TOKEN_MIN <=
// token <= TOKEN_MAX` en Python. No necesita tabla ni asignacion dinamica.
inline bool is_valid(int token) {
    return token >= TOKEN_MIN && token <= TOKEN_MAX;
}

// Estos predicados separan las cuatro clases del alfabeto, como funciones
// booleanas Python que clasifican un entero sin modificarlo.
inline bool is_constant(int token) {
    // Constantes del alfabeto: los literales numericos 1.0-10.0 y las hojas
    // pi/e (issue #64). En Python seria
    // `token in range(TOKEN_CONST1, TOKEN_CONST10 + 1) or token in (TOKEN_PI, TOKEN_E)`.
    return (token >= TOKEN_CONST1 && token <= TOKEN_CONST10) ||
           token == TOKEN_PI || token == TOKEN_E;
}

inline bool is_variable(int token) {
    return token == TOKEN_X;
}

// is_polynomial reconoce las hojas de potencia fija de x (x^2, x^3): su valor
// es una potencia de x, pero no son la variable x. En Python seria
// `token in (TOKEN_X2, TOKEN_X3)`.
inline bool is_polynomial(int token) {
    return token == TOKEN_X2 || token == TOKEN_X3;
}

// is_x_dependent responde la pregunta que le importa a fitness(): si el token
// es una hoja cuyo valor depende de x (la variable o una hoja polinomial). En
// Python seria `is_variable(token) or is_polynomial(token)`.
inline bool is_x_dependent(int token) {
    return is_variable(token) || is_polynomial(token);
}

inline bool is_unary(int token) {
    return (token >= TOKEN_NEG && token <= TOKEN_SQRT) ||
           (token >= TOKEN_TAN && token <= TOKEN_TANH);
}

inline bool is_binary(int token) {
    return token >= TOKEN_ADD && token <= TOKEN_POW;
}

// random_token convierte el resultado no negativo de `std::rand()` en un
// desplazamiento del alfabeto. Es el equivalente a
// `random.randint(TOKEN_MIN, TOKEN_MAX)` en Python; la semilla se controla
// externamente con `std::srand()`.
inline int random_token() {
    return TOKEN_MIN + (std::rand() % TOKENS_ALFABETO);
}

// arity traduce un token a la cantidad de hijos que requiere un arbol prefijo.
// Es el equivalente a consultar un `dict` de aridades en Python; -1 indica un
// codigo fuera del alfabeto, que no pertenece a ninguna regla gramatical.
inline int arity(int token) {
    // Hojas de valor fijo: los literales 1.0-10.0, la variable x y las
    // constantes pi/e (issue #64). En Python seria
    // `if is_constant(token) or is_variable(token): return 0`.
    if (is_constant(token) || is_variable(token)) {
        return 0;
    }
    // Las hojas polinomiales (issue #63) tambien son hojas: no consumen hijos
    // aunque su valor se calcule a partir de x. En Python seria ampliar la
    // primera condicion con `or is_polynomial(token)`.
    if (is_polynomial(token)) {
        return 0;
    }
    if ((token >= TOKEN_NEG && token <= TOKEN_SQRT) ||
        (token >= TOKEN_TAN && token <= TOKEN_TANH)) {
        return 1;
    }
    if (token >= TOKEN_ADD && token <= TOKEN_POW) {
        return 2;
    }
    return -1;
}

// render devuelve el fragmento de expresion que representa un token. Un
// `switch` es el equivalente a `match token:` de Python; el string vacio hace
// visible que un codigo invalido no tiene plantilla de renderizado.
inline std::string render(int token) {
    switch (token) {
        case TOKEN_CONST1:
            return "1.0";
        case TOKEN_CONST2:
            return "2.0";
        case TOKEN_CONST3:
            return "3.0";
        case TOKEN_CONST4:
            return "4.0";
        case TOKEN_CONST5:
            return "5.0";
        case TOKEN_CONST6:
            return "6.0";
        case TOKEN_CONST7:
            return "7.0";
        case TOKEN_CONST8:
            return "8.0";
        case TOKEN_CONST9:
            return "9.0";
        case TOKEN_CONST10:
            return "10.0";
        case TOKEN_X:
            return "x";
        case TOKEN_NEG:
            return "-1.0*()";
        case TOKEN_SIN:
            return "sin()";
        case TOKEN_COS:
            return "cos()";
        case TOKEN_EXP:
            return "exp()";
        case TOKEN_LOG:
            return "log()";
        case TOKEN_SQRT:
            return "()**(1/2)";
        case TOKEN_TAN:
            return "tan()";
        case TOKEN_SINH:
            return "sinh()";
        case TOKEN_COSH:
            return "cosh()";
        case TOKEN_TANH:
            return "tanh()";
        // Hojas polinomiales (issue #63): se renderizan como la potencia fija
        // de x que representan. No llevan huecos "()" porque no tienen hijos.
        case TOKEN_X2:
            return "x^2";
        case TOKEN_X3:
            return "x^3";
        // Constantes matematicas (issue #64): el simbolo de la constante,
        // tambien sin huecos "()" por ser hojas.
        case TOKEN_PI:
            return "pi";
        case TOKEN_E:
            return "e";
        case TOKEN_ADD:
            return "()+()";
        case TOKEN_MUL:
            return "()*()";
        case TOKEN_DIV:
            return "()/()";
        case TOKEN_POW:
            return "()**()";
        default:
            return "";
    }
}

#endif  // TOKEN_H
