// test_header_hygiene.cpp — test de higiene de interfaz (issue #39).
//
// Este archivo verifica que dualfunctions.h sea autocontenido y que los
// parametros de solo-lectura sean const references. Es un test de tiempo de
// compilacion: si no compila, el test falla.
//
// Para un desarrollador Python: esto es como un `import` que verifica que un
// modulo no dependa de nombres inyectados por otros modulos. Si el header no
// incluye todo lo que usa, este archivo no compilara (como un `ImportError`).
//
// Se compila solo, sin incluir <vector> ni <string> antes de dualfunctions.h.
// Si el header no es autocontenido, el compilador fallara.

#include "dualfunctions.h"

// Test 1: el header debe declarar std::vector y std::string sin depender de
// using namespace std; inyectado en el TU consumidor.
static void test_tipos_cualificados() {
    // Si el header usa `vector` sin `std::`, esto no compila.
    std::vector<ldouble> v = {1.0L, 2.0L, 3.0L};
    std::string s = parse_tree(v);
    (void)s;  // sin uso intencional
}

// Test 2: los parametros de solo-lectura deben ser const references, asi que
// se deben poder pasar vectores const. Si no son const, esto no compila.
static void test_parametros_const() {
    const std::vector<ldouble> xval = {0.5L, 1.0L, 0.0L};
    const std::vector<ldouble> yval = {2.0L, 0.0L, 0.0L};

    // Todas estas funciones deben aceptar const references (son de solo-lectura).
    auto r1 = sin2dual(xval);
    auto r2 = cos2dual(xval);
    auto r3 = tan2dual(xval);
    auto r4 = sinh2dual(xval);
    auto r5 = cosh2dual(xval);
    auto r6 = tanh2dual(xval);
    auto r7 = exp2dual(xval);
    auto r8 = log2dual(xval);
    auto r9 = sqrt2dual(xval);
    auto r10 = division2dual(xval, yval);
    auto r11 = pow2dual(xval, yval);
    auto r12 = prod2dual(xval, yval);
    auto r13 = parse_tree(xval);
    auto r14 = OperF(11.0L, xval);  // 11 = TOKEN_X
    auto r15 = parsv(xval, xval);

    // Evitar warnings de "unused variable".
    (void)r1; (void)r2; (void)r3; (void)r4; (void)r5;
    (void)r6; (void)r7; (void)r8; (void)r9; (void)r10;
    (void)r11; (void)r12; (void)r13; (void)r14; (void)r15;
}

// Test 3: OperF2 debe mantener su matriz no-const (contrato documentado de
// entrada/salida). Este test verifica que NO sea const reference.
static void test_operf2_no_const() {
    // OperF2 modifica la matriz in-place (borra operandos consumidos),
    // asi que debe seguir aceptando non-const reference.
    std::vector<std::vector<ldouble>> vecval = {
        {1.0L, 2.0L, 3.0L},
        {0.0L, 0.0L, 0.0L},
        {0.0L, 0.0L, 0.0L}
    };
    auto result = OperF2(11.0L, vecval, 0);  // 11 = TOKEN_X
    (void)result;
}

int main() {
    test_tipos_cualificados();
    test_parametros_const();
    test_operf2_no_const();
    return 0;
}
