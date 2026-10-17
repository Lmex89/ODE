// =========================================================================
// salida.h - Destino de la salida de la corrida (issue #112).
//
// Por defecto el solver escribe en `ode-solver.log` del directorio actual;
// `ODE_LOG=terminal` lo devuelve a stdout y `ODE_LOG_FILE=<ruta>` cambia el
// archivo. Para un desarrollador Python: header + .cpp son el modulo
// `salida.py`; la firma publica vive aqui y la implementacion en el .cpp.
// =========================================================================

#ifndef SALIDA_H
#define SALIDA_H

#include <fstream>
#include <string>
#include <streambuf>

// ModoSalida distingue los dos destinos posibles. `enum class` evita que los
// nombres se mezclen con otras constantes (en Python seria un Enum).
enum class ModoSalida {
    Archivo,
    Terminal,
};

// ConfigSalida es el resultado puro de leer el entorno: el modo elegido y la
// ruta candidata del archivo. En Python seria un dataclass o una namedtuple.
struct ConfigSalida {
    ModoSalida modo;
    std::string ruta;
};

// resolver_salida: decide donde escribe el solver a partir de los valores ya
// leidos del entorno (getenv se hace en main(), no aqui), igual que
// resolver_semilla. `log_env` es ODE_LOG y `ruta_env` es ODE_LOG_FILE; ambos
// pueden ser nullptr si la variable no esta definida.
ConfigSalida resolver_salida(const char* log_env, const char* ruta_env);

// RedireccionSalida es RAII: al construirse abre el archivo (si el modo es
// Archivo) y cambia el buffer de std::cout; al destruirse restaura stdout y
// deja que el ofstream cierre el archivo. En Python el equivalente seria un
// context manager (`with redireccion: ...`) o try/finally.
class RedireccionSalida {
public:
    explicit RedireccionSalida(const ConfigSalida& configuracion);
    ~RedireccionSalida();

    // El objeto posee un recurso unico (el archivo y el buffer de cout):
    // copiarlo duplicaria el cierre. En Python no aplica; es la regla C.21.
    RedireccionSalida(const RedireccionSalida&) = delete;
    RedireccionSalida& operator=(const RedireccionSalida&) = delete;
    RedireccionSalida(RedireccionSalida&&) = delete;
    RedireccionSalida& operator=(RedireccionSalida&&) = delete;

    // true solo si se pidio archivo y no se pudo abrir; main() lo usa para
    // avisar antes de cualquier otra linea de log.
    bool fallo_apertura() const { return fallo_apertura_; }

private:
    std::ofstream archivo_;
    // Buffer original de cout (el de la terminal). Es un observador, no un
    // dueno: std::cout sigue siendo el dueno del buffer.
    std::streambuf* buffer_original_{nullptr};
    bool fallo_apertura_{false};
};

#endif
