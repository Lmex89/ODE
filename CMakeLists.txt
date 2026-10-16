# CMakeLists.txt del prototipo ODE (issue #47: linea base de CMake).
#
# Para un desarrollador Python: este archivo es el equivalente CMake del
# Makefile y convive con el. CMake configura y compila fuera de las fuentes
# (todo el build cae en build/); los comandos son:
#
#   cmake -B build            # configurar       (como leer pyproject.toml)
#   cmake --build build       # compilar         (como `python -m build`)
#   ctest --test-dir build    # correr los tests (como `pytest`)
#
# El Makefile sigue vigente y no se toca en este issue: ambos caminos compilan
# los mismos fuentes y producen el mismo main.exe.

cmake_minimum_required(VERSION 3.16)

project(ODE CXX)

# Mismos flags que el Makefile: CXXFLAGS := -std=c++17 -Wall -O3.
# CMAKE_CXX_EXTENSIONS OFF pide -std=c++17 estricto (no -std=gnu++17), igual
# que la variable CXXFLAGS del Makefile.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -O3)

# ---------------------------------------------------------------------------
# Ejecutable principal: mismos fuentes que $(SRCS) del Makefile. Conserva el
# nombre main.exe (en Linux CMake no anade sufijo al objetivo), asi que
# build/main.exe es el mismo binario que `make` deja en la raiz.
# parse_tree_bckp.cpp queda fuera a proposito: tambien define main() y
# enlazarlo daria un simbolo duplicado.
# ---------------------------------------------------------------------------
add_executable(main.exe
    dualfunctions.cpp
    semilla.cpp
    parse_tree.cpp
)

# ---------------------------------------------------------------------------
# Tests: los mismos 9 binarios de `make test`. Cada test define su propio
# main() y devuelve codigo distinto de cero si falla algun assert (el
# equivalente al exit code de pytest).
# ---------------------------------------------------------------------------

enable_testing()

# test_dual (issue #2): numeros duales; enlaza solo dualfunctions.cpp.
add_executable(test_dual dualfunctions.cpp test_dual.cpp)
add_test(NAME test_dual COMMAND test_dual)

# test_semilla (issue #4): resolucion de ODE_SEED; solo semilla.cpp.
add_executable(test_semilla semilla.cpp test_semilla.cpp)
add_test(NAME test_semilla COMMAND test_semilla)

# test_mutation (issue #5), test_crossover (issue #6) y test_token (issue #31)
# enlazan parse_tree.cpp (ahi viven Mutation y Recombination) y, con el, sus
# dependencias. -DODE_NO_MAIN descarta el main() del programa para que el
# unico main() de cada binario sea el del test (misma tecnica que el Makefile).
foreach(nombre IN ITEMS test_mutation test_crossover test_token)
    add_executable(${nombre}
        semilla.cpp
        dualfunctions.cpp
        parse_tree.cpp
        ${nombre}.cpp
    )
    target_compile_definitions(${nombre} PRIVATE ODE_NO_MAIN)
    add_test(NAME ${nombre} COMMAND ${nombre})
endforeach()

# test_funbobj_const (issue #42), test_grid (issue #7) y test_population
# (issue #11) incluyen parse_tree.cpp desde su propio .cpp (FunbObj no tiene
# header): parse_tree.cpp NO se compila aparte o habria simbolos duplicados.
# Necesitan el directorio de fuentes en el include path, ahora que el .cpp
# incluido se busca primero junto al archivo que lo incluye, y -DODE_NO_MAIN
# para que gane el main() del test.
foreach(nombre IN ITEMS test_funbobj_const test_grid test_population)
    add_executable(${nombre}
        semilla.cpp
        dualfunctions.cpp
        ${nombre}.cpp
    )
    target_compile_definitions(${nombre} PRIVATE ODE_NO_MAIN)
    target_include_directories(${nombre} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${nombre} COMMAND ${nombre})
endforeach()

# test_header_hygiene (issue #39): test de tiempo de compilacion; si el
# binario compila, el test pasa.
add_executable(test_header_hygiene test_header_hygiene.cpp)
add_test(NAME test_header_hygiene COMMAND test_header_hygiene)
