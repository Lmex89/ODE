// test_header_hygiene.cpp — test de tiempo de compilacion (issue #39)
//
// Verifica que dualfunctions.h sea autocontenido y que los parametros de
// solo-lectura sean const references. Es un test de compilacion pura: no
// enlaza con dualfunctions.cpp, solo verifica que las declaraciones sean
// correctas.
//
// Para un desarrollador Python: esto es como hacer `import modulo` sin
// llamar a ninguna funcion. Si el import falla (el header no incluye todo
// lo que usa), el test falla en tiempo de compilacion.

#include "dualfunctions.h"

// Verificar que los tipos esten cualificados con std::
static_assert(sizeof(std::vector<ldouble>) > 0, "std::vector debe estar disponible");
static_assert(sizeof(str) > 0, "str (std::string) debe estar disponible");

// Verificar que las declaraciones acepten const references (solo declaracion,
// sin implementacion). Si el header declara parametros no-const, esto no
// compilara porque no podra pasar un const vector a un parametro no-const.
void test_const_correctness() {
    // Solo necesitamos que las declaraciones acepten const references.
    // No llamamos a las funciones, solo verificamos que las firmas sean
    // compatibles con const references usando punteros a funcion.
    
    // Funciones unarias: deben aceptar const std::vector<ldouble>&
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(sin2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(cos2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(tan2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(sinh2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(cosh2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(tanh2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(exp2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(log2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&)>(sqrt2dual);
    
    // Funciones binarias: deben aceptar dos const std::vector<ldouble>&
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&, const std::vector<ldouble>&)>(division2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&, const std::vector<ldouble>&)>(pow2dual);
    (void)static_cast<std::vector<ldouble>(*)(const std::vector<ldouble>&, const std::vector<ldouble>&)>(prod2dual);
    
    // parse_tree y OperF: deben aceptar const references
    (void)static_cast<str(*)(const std::vector<ldouble>&)>(parse_tree);
    (void)static_cast<std::vector<ldouble>(*)(ldouble, const std::vector<ldouble>&)>(OperF);
    
    // OperF2 mantiene su matriz no-const (contrato documentado de entrada/salida)
    (void)static_cast<std::vector<long double>(*)(ldouble, std::vector<std::vector<ldouble>>&, int)>(OperF2);
}

int main() {
    // No necesitamos hacer nada en runtime. Si este archivo compila, el
    // header es autocontenido y las firmas son correctas.
    return 0;
}
