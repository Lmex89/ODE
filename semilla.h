// =========================================================================
// semilla.h - Control de la semilla aleatoria de las corridas (issue #4).
//
// El experimento A/B necesita correr el codigo de antes y el de despues con
// la MISMA semilla; por eso la semilla se resuelve aqui a partir de ODE_SEED
// y no del reloj.
//
// Para un desarrollador Python: header + .cpp cumplen juntos el rol de un
// modulo `semilla.py`. En C++ la firma publica vive en el .h (se incluye con
// #include) y la implementacion en el .cpp (se compila y se enlaza); Python
// no tiene esa separacion, solo importa el modulo.
// =========================================================================

#ifndef SEMILLA_H
#define SEMILLA_H

// resolver_semilla: decide con que semilla arranca srand().
// `valor_env` es el texto de la variable de entorno ODE_SEED (o nullptr si no
// esta definida) y `respaldo` la semilla a usar cuando ese texto no empieza
// con un numero.
// Equivalente en Python (salvo que strtol ignora lo que sigue al numero,
// "42abc" -> 42, mientras que int("42abc") lanza ValueError):
//     def resolver_semilla(valor_env, respaldo):
//         try:
//             return int(valor_env)
//         except (TypeError, ValueError):
//             return respaldo
long resolver_semilla(const char* valor_env, long respaldo);

#endif
