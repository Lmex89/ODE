// =========================================================================
// salida.cpp - Implementacion del destino de la salida (issue #112).
// =========================================================================

#include "salida.h"

#include <cstring>   // strcmp: comparacion exacta de texto, como `==` en Python
#include <iostream>  // std::cout y su buffer: la salida que se redirige
#include <string>

// Ruta por defecto: el log queda en el directorio de trabajo, con el mismo
// nombre que el ejecutable del build. En Python seria una constante de modulo.
constexpr const char* RUTA_LOG_POR_DEFECTO{"ode-solver.log"};

// resolver_salida: ver salida.h. La resolucion es pura para poder testearla
// sin abrir archivos ni correr la busqueda.
ConfigSalida resolver_salida(const char* log_env, const char* ruta_env) {
    // Solo el valor exacto "terminal" devuelve la salida a stdout; cualquier
    // otro texto (o nullptr) conserva el archivo por defecto. En Python seria
    // `if log_env == "terminal": ...`.
    const bool terminal = log_env != nullptr && std::strcmp(log_env, "terminal") == 0;

    // Una ruta vacia no sirve para abrir un archivo: se trata como ausente y
    // se usa el default. En Python seria `ruta_env or "ode-solver.log"`.
    const bool ruta_valida = ruta_env != nullptr && ruta_env[0] != '\0';
    const std::string ruta = ruta_valida ? ruta_env : RUTA_LOG_POR_DEFECTO;

    return ConfigSalida{terminal ? ModoSalida::Terminal : ModoSalida::Archivo, ruta};
}

RedireccionSalida::RedireccionSalida(const ConfigSalida& configuracion) {
    if (configuracion.modo == ModoSalida::Terminal) return;

    // `std::ios::trunc` borra el contenido anterior: cada corrida empieza un
    // log limpio (no quedan lineas de la corrida previa). En Python seria
    // `open(ruta, "w")`.
    archivo_.open(configuracion.ruta, std::ios::out | std::ios::trunc);
    if (!archivo_.is_open()) {
        // Fallo de apertura (directorio inexistente, permisos, ruta que es un
        // directorio): main() avisa y la corrida sigue en la terminal.
        fallo_apertura_ = true;
        return;
    }

    // rdbuf() intercambia el buffer de cout por el del archivo y devuelve el
    // anterior. Todo lo que se escriba en cout (log_info, log_warn y la
    // descripcion libre del caso) cae en el archivo sin tocar cada llamada.
    buffer_original_ = std::cout.rdbuf(archivo_.rdbuf());
}

RedireccionSalida::~RedireccionSalida() {
    if (buffer_original_ == nullptr) return;

    // Vaciar lo pendiente ANTES de restaurar: los flushes explicitos siguen
    // funcionando durante la corrida, y este cubre lo ultimo. En Python seria
    // `archivo.flush()` dentro del finally.
    std::cout.flush();
    std::cout.rdbuf(buffer_original_);
}
