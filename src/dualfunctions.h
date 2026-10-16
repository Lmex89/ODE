// Include guard: si este header se procesa dos veces en la misma unidad de
// compilacion (p. ej. test_grid.cpp lo incluye directamente y ademas incluye
// parse_tree.cpp, que a su vez lo incluye), la segunda vez se salta. En
// Python el equivalente es el cache de `sys.modules`: un `import` repetido no
// vuelve a ejecutar el modulo. Sin el guard, un argumento por defecto
// (`int inicio = 0`) declarado dos veces es error de compilacion.
#pragma once

// El header incluye todo lo que usa (SF.7): vector para los duales y string
// para el renderizado. En Python seria `import math, typing` al principio del
// modulo, no depender de que otro modulo los haya importado antes.
#include <vector>
#include <string>

typedef long double ldouble;
typedef std::string str;

// Las funciones unarias reciben el dual [f, f', f''] por const reference
// (SF.10, F.16): solo leen la entrada y devuelven un vector nuevo. En Python
// seria `def sin2dual(xval: list[float]) -> list[float]` (inmutable por
// convencion; aqui el `const` lo exige el compilador).
std::vector<ldouble> sin2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> cos2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> tan2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> sinh2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> cosh2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> tanh2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> exp2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> log2dual(const std::vector<ldouble>& xval);
std::vector<ldouble> sqrt2dual(const std::vector<ldouble>& xval);

// Las funciones binarias tambien son const: leen ambos operandos sin mutarlos
// (F.17). En Python no existe esa distincion (todo es referencia), pero la
// semantica es la misma: `def division2dual(x, y)` no modifica x ni y.
std::vector<ldouble> division2dual(const std::vector<ldouble>& x, const std::vector<ldouble>& y);
std::vector<ldouble> pow2dual(const std::vector<ldouble>& x, const std::vector<ldouble>& y);
std::vector<ldouble> prod2dual(const std::vector<ldouble>& x, const std::vector<ldouble>& y);

str parse_tree(const std::vector<ldouble>& v);
int narg(ldouble x);

// Equivalente C++ de la seccion `auxv(:,k+1:length)` del Fortran del paper
// (Ap. A): la matriz completa `vecval` se pasa por referencia NO-CONST y
// `inicio` marca desde que columna buscar. Con `inicio = 0` (valor por
// defecto, como un argumento opcional de Python) se conservan las llamadas de
// dos argumentos.
//
// ATENCION: `vecval` NO es const porque OperF2 borra operandos consumidos
// (poniendo NaN en la componente real de la columna consumida). Este es el
// contrato documentado de entrada/salida del Algorithm 1 del paper. En Python
// seria pasar la lista original y mutarla in-place, no una copia.
std::vector<long double> OperF2(ldouble x, std::vector<std::vector<ldouble>>& vecval, int inicio = 0);

std::vector<ldouble> OperF(ldouble x, const std::vector<ldouble>& xval);
std::vector<ldouble> parsv(std::vector<ldouble> x, std::vector<ldouble> xval);
