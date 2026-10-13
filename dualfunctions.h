// Include guard: si este header se procesa dos veces en la misma unidad de
// compilacion (p. ej. test_grid.cpp lo incluye directamente y ademas incluye
// parse_tree.cpp, que a su vez lo incluye), la segunda vez se salta. En
// Python el equivalente es el cache de `sys.modules`: un `import` repetido no
// vuelve a ejecutar el modulo. Sin el guard, un argumento por defecto
// (`int inicio = 0`) declarado dos veces es error de compilacion.
#pragma once

using namespace std;
typedef  long double ldouble;
typedef string  str;

vector<ldouble> sin2dual(vector<ldouble>& xval);
vector<ldouble> cos2dual(vector<ldouble>& xval);
vector<ldouble> tan2dual(vector<ldouble>& xval);
vector<ldouble> exp2dual(vector<ldouble>& xval);
vector<ldouble> log2dual(vector<ldouble>& xval);
vector<ldouble> sqrt2dual(vector<ldouble>& xval);
vector<ldouble> division2dual(vector<ldouble>&x, vector<ldouble>& y);
vector<ldouble> pow2dual(vector<ldouble>&x, vector<ldouble>& y);
vector<ldouble> prod2dual(vector<ldouble>&x, vector<ldouble>& y);

void imprime(vector<ldouble> n );
str parse_tree( vector<ldouble>& v);
int narg(ldouble x);
// Equivalente C++ de la seccion `auxv(:,k+1:length)` del Fortran del paper
// (Ap. A): la matriz completa `vecval` se pasa por referencia y `inicio` marca
// desde que columna buscar. Con `inicio = 0` (valor por defecto, como un
// argumento opcional de Python) se conservan las llamadas de dos argumentos.
vector<long double> OperF2(ldouble x,vector<vector<ldouble>>& vecval, int inicio = 0);
vector<ldouble> OperF(ldouble x, vector<ldouble>& xval);
vector<ldouble> parsv(vector<ldouble> x, vector<ldouble> xval);
