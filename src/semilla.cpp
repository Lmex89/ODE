// =========================================================================
// semilla.cpp - Implementacion del control de semilla (issue #4).
//
// Para un desarrollador Python: este archivo seria `semilla.py`; la funcion
// de abajo es la unica funcion publica del modulo.
// =========================================================================

#include <cstdlib>   // strtol: como `int(texto)` de Python

#include "semilla.h"

// resolver_semilla: devuelve la semilla con la que srand() inicializa la
// secuencia pseudoaleatoria. Recibe el valor de ODE_SEED ya leido (getenv lo
// lee en main(), no aqui) para que sea una funcion pura y testeable.
long resolver_semilla(const char* valor_env, long respaldo) {
    // getenv() devuelve nullptr cuando ODE_SEED no esta definida; en Python
    // seria `os.environ.get("ODE_SEED")` -> None. Sin variable no hay semilla
    // fija, asi que se usa el respaldo (el reloj que main() inyecta).
    if (valor_env == nullptr) return respaldo;

    // strtol interpreta el texto como entero en base 10 (como int() en
    // Python) y deja en `fin` la posicion del primer caracter no numerico.
    char* fin = nullptr;
    long valor = std::strtol(valor_env, &fin, 10);

    // Si no consumio ningun caracter ("", "abc"), el texto no representa un
    // numero: se usa el respaldo. Es el `except ValueError` de Python.
    if (fin == valor_env) return respaldo;

    return valor;
}
