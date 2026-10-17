// =========================================================================
// log_mode.cpp - Implementacion del destino de la salida (issue #111).
//
// Para un desarrollador Python: este archivo seria `log_mode.py`; expone la
// funcion pura `resolver_log` y la clase RAII `RedireccionLog` que instala la
// redireccion al arrancar main() (ver log_mode.h).
// =========================================================================

#include "log_mode.h"

ConfigLog resolver_log(const char* modo_env, const char* ruta_env) {
    // Solo el texto exacto "terminal" devuelve la salida a stdout; la
    // variable ausente (nullptr, como os.environ.get sin default) o
    // cualquier otro valor deja el archivo por defecto, que es el caso
    // normal. En Python seria
    // `salida = "terminal" if modo_env == "terminal" else "archivo"`.
    ConfigLog config;
    config.salida = SalidaLog::Archivo;
    if (modo_env != nullptr && std::string(modo_env) == "terminal") {
        config.salida = SalidaLog::Terminal;
    }

    // Una ruta vacia no es un archivo valido (open("") falla en Python y en
    // C++): se vuelve a la ruta por defecto, la misma regla que config_entera
    // aplica a un valor invalido.
    config.ruta = "ode-solver.log";
    if (ruta_env != nullptr && ruta_env[0] != '\0') {
        config.ruta = ruta_env;
    }
    return config;
}

// RedireccionLog: intercambia el streambuf de cout por el del archivo. Los
// streambuf son los buffers de la STL: `cout.rdbuf(nuevo)` devuelve el
// anterior y desde ahi todo `cout << ...` (los helpers de log y el texto
// libre de ode_case) escribe en el archivo. En Python el equivalente mas
// cercano es contextlib.redirect_stdout(archivo).
RedireccionLog::RedireccionLog(const ConfigLog& config) {
    // Modo terminal (ODE_LOG=terminal): no se abre ningun archivo.
    if (config.salida != SalidaLog::Archivo) return;

    // trunc: cada corrida reescribe el log desde cero (open(ruta, "w") en
    // Python); asi no quedan resultados viejos mezclados con los nuevos.
    archivo_.open(config.ruta, std::ios::out | std::ios::trunc);
    if (!archivo_.is_open()) return;  // main() avisa y sigue por terminal

    buffer_original_ = std::cout.rdbuf(archivo_.rdbuf());
    redirigida_ = true;
}

RedireccionLog::~RedireccionLog() {
    if (!redirigida_) return;
    // Los flushes que main() ya hace alcanzan para el `tail -f`; este
    // vaciado final es del dueno del archivo, no de cada linea. Despues se
    // le devuelve a cout su buffer original; el ofstream se cierra solo.
    std::cout.flush();
    std::cout.rdbuf(buffer_original_);
}
