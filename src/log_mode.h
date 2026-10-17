// =========================================================================
// log_mode.h - Destino de la salida del solver (issue #111).
//
// Para un desarrollador Python: header + .cpp cumplen juntos el rol de un
// modulo `log_mode.py` (misma separacion que semilla.h/semilla.cpp): la firma
// publica vive en el .h (se incluye con #include) y la implementacion en el
// .cpp (se compila y se enlaza).
//
// El solver escribe a un archivo por defecto (`ode-solver.log` en el
// directorio actual, truncado en cada corrida). `ODE_LOG=terminal` devuelve
// la salida a stdout y no crea archivo; `ODE_LOG_FILE` cambia la ruta. El
// modo se resuelve en una funcion pura (resolver_log) para poder probarlo sin
// correr la busqueda diferencial completa (ver tests/test_log_mode.cpp).
// =========================================================================

#ifndef LOG_MODE_H
#define LOG_MODE_H

#include <fstream>
#include <iostream>
#include <string>

// SalidaLog: destino de la salida del programa. En Python seria un Enum:
//     class SalidaLog(Enum):
//         TERMINAL = "terminal"
//         ARCHIVO = "archivo"
enum class SalidaLog { Terminal, Archivo };

// ConfigLog: resultado de resolver el destino. Es un struct de datos puro
// (en Python seria un dataclass o una NamedTuple: ConfigLog(salida, ruta)).
struct ConfigLog {
    SalidaLog salida;
    std::string ruta;
};

// resolver_log: decide destino y ruta a partir de los valores crudos de
// ODE_LOG y ODE_LOG_FILE que main() lee con getenv() (por eso es una funcion
// pura y testeable, igual que resolver_semilla y resolver_config). nullptr
// significa "variable ausente", como en os.environ.get(...).
ConfigLog resolver_log(const char* modo_env, const char* ruta_env);

// RedireccionLog: redirige std::cout al archivo al construir y lo restaura
// al destruir (RAII: el `with` de Python, pero atado al scope de la
// variable; nadie llama a cerrar ni a restaurar a mano). Si el archivo no
// se puede abrir, el objeto queda sin redirigir y main() decide como avisar.
class RedireccionLog {
public:
    // `explicit` evita conversiones implicitas de ConfigLog a RedireccionLog
    // (en Python no existe esa regla; es una exigencia del compilador: sin
    // explicit, pasar un ConfigLog donde se espera una redireccion
    // compilaria en silencio).
    explicit RedireccionLog(const ConfigLog& config);

    // No copiable: dos objetos no pueden restaurar el mismo streambuf. En
    // Python no hay copias del objeto, solo referencias; en C++ se pide
    // borrando estas dos operaciones.
    RedireccionLog(const RedireccionLog&) = delete;
    RedireccionLog& operator=(const RedireccionLog&) = delete;

    // Al salir del scope: vaciar lo pendiente, devolver su buffer a cout y
    // cerrar el archivo (el ofstream se cierra solo al destruirse).
    ~RedireccionLog();

    // redirigida: true si std::cout quedo apuntando al archivo (abrio bien).
    bool redirigida() const { return redirigida_; }

private:
    // ofstream: el archivo con RAII de la STL; se cierra solo al destruirse
    // (el `with open(ruta, "w") as archivo:` de Python).
    std::ofstream archivo_;
    // Buffer original de std::cout para restaurarlo al salir. En Python el
    // equivalente mas cercano es contextlib.redirect_stdout(...).
    std::streambuf* buffer_original_ = nullptr;
    bool redirigida_ = false;
};

#endif
