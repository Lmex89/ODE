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

// Operaciones de dos operandos; en notacion prefija anteceden sus dos hijos.
constexpr int TOKEN_ADD{18};
constexpr int TOKEN_MUL{19};
constexpr int TOKEN_DIV{20};
constexpr int TOKEN_POW{21};

// Estos limites describen el alfabeto contiguo. En Python serian los extremos
// de `range(TOKEN_MIN, TOKEN_MAX + 1)` y su cantidad total.
constexpr int TOKEN_MIN{TOKEN_CONST1};
constexpr int TOKEN_MAX{TOKEN_POW};
constexpr int NUM_TOKENS{TOKEN_MAX - TOKEN_MIN + 1};

// is_valid comprueba pertenencia al alfabeto contiguo, igual que `TOKEN_MIN <=
// token <= TOKEN_MAX` en Python. No necesita tabla ni asignacion dinamica.
inline bool is_valid(int token) {
    return token >= TOKEN_MIN && token <= TOKEN_MAX;
}

// Estos predicados separan las cuatro clases del alfabeto, como funciones
// booleanas Python que clasifican un entero sin modificarlo.
inline bool is_constant(int token) {
    return token >= TOKEN_CONST1 && token <= TOKEN_CONST10;
}

inline bool is_variable(int token) {
    return token == TOKEN_X;
}

inline bool is_unary(int token) {
    return token >= TOKEN_NEG && token <= TOKEN_SQRT;
}

inline bool is_binary(int token) {
    return token >= TOKEN_ADD && token <= TOKEN_POW;
}

// random_token convierte el resultado no negativo de `std::rand()` en un
// desplazamiento del alfabeto. Es el equivalente a `random.randint(1, 21)` en
// Python; la semilla se controla externamente con `std::srand()`.
inline int random_token() {
    return TOKEN_MIN + (std::rand() % NUM_TOKENS);
}

// arity traduce un token a la cantidad de hijos que requiere un arbol prefijo.
// Es el equivalente a consultar un `dict` de aridades en Python; -1 indica un
// codigo fuera del alfabeto, que no pertenece a ninguna regla gramatical.
inline int arity(int token) {
    if (token >= TOKEN_CONST1 && token <= TOKEN_X) {
        return 0;
    }
    if (token >= TOKEN_NEG && token <= TOKEN_SQRT) {
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
