# Makefile del prototipo ODE (parse_tree.cpp + dualfunctions.cpp).
#
# Para un desarrollador Python: un Makefile equivale a la seccion de build de
# un `pyproject.toml`/`setup.py`; `make` seria como `python -m build` y `make
# clean` como borrar los artefactos (dist/, __pycache__/).

# CXX ?= g++ -> usa g++ por defecto, pero se puede sobreescribir:
# `make CXX=icpc`. En Python seria `os.environ.get("CXX", "g++")`.
CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -O3
LDFLAGS  :=

TARGET := main.exe
SRCS   := dualfunctions.cpp semilla.cpp parse_tree.cpp
# $(SRCS:.cpp=.o) renombra cada fuente a un objeto: seria como
# [s[:-4] + ".o" for s in SRCS] en Python.
OBJS   := $(SRCS:.cpp=.o)

# parse_tree_bckp.cpp NO se compila: es una copia de referencia que tambien
# define main(), y enlazarla daria un error de simbolo duplicado.

.PHONY: all clean run test

# `all` es la regla por defecto: `make` a secas construye el ejecutable.
all: $(TARGET)

# Enlazado ("link") de los .o en el binario final. $@ es el objetivo
# (main.exe) y $^ todas sus dependencias (los .o); es el paso equivalente a
# empaquetar varios modulos Python en un solo distributable.
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

# Compila cada .cpp a un .o por separado (como importar/compilar un modulo
# individual). $< es la primera dependencia, el .cpp de turno.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# parse_tree.cpp incluye dualfunctions.h y semilla.h: si algun header cambia,
# hay que recompilar ese .o. (dualfunctions.cpp no incluye headers, por eso no
# aparece aqui.)
parse_tree.o: parse_tree.cpp dualfunctions.h semilla.h

semilla.o: semilla.cpp semilla.h

# `make run` compila si hace falta y ejecuta el binario. Esta separado de
# `all` porque el programa es estocastico (srand(time(NULL))) y puede tardar
# mucho o imprimir en exceso; conviene correrlo con `timeout` y `tail`.
run: $(TARGET)
	./$(TARGET)

# `make test`: compila y corre los tres binarios de tests: dual numbers
# (issue #2), control de semilla (issue #4) y la mutacion DE/rand/1/bin
# (issue #5). Cada binario enlaza solo lo que necesita; test_mutation si
# enlaza parse_tree.cpp con -DODE_NO_MAIN (ver abajo).
# En Python seria como correr `pytest`: exit code != 0 si algun assert falla.
TESTBINS := test_dual.exe test_semilla.exe test_mutation.exe

test: $(TESTBINS)
	./test_dual.exe
	./test_semilla.exe
	./test_mutation.exe

test_dual.exe: dualfunctions.cpp test_dual.cpp dualfunctions.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) dualfunctions.cpp test_dual.cpp -o $@

test_semilla.exe: semilla.cpp test_semilla.cpp semilla.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) semilla.cpp test_semilla.cpp -o $@

# test_mutation necesita parse_tree.cpp (ahi vive Mutation) y, con el, todo lo
# que ese archivo enlaza: dualfunctions.cpp y semilla.cpp. Para evitar dos
# main() se apaga el del programa con -DODE_NO_MAIN: el preprocesador descarta
# ese bloque y el unico main() del binario es el del test. En Python no hace
# falta nada parecido porque los tests solo importan las funciones.
test_mutation.exe: semilla.cpp dualfunctions.cpp parse_tree.cpp test_mutation.cpp semilla.h dualfunctions.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DODE_NO_MAIN semilla.cpp dualfunctions.cpp parse_tree.cpp test_mutation.cpp -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TESTBINS)
