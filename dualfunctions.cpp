#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>

#include "token.h"

using namespace std;
typedef long long int llint;
typedef  long double ldouble;
typedef string  str;

// Valores de las constantes matematicas pi y e (issue #64). acos(-1) y exp(1)
// son identidades de libm, asi que no hay literales que puedan quedar mal
// escritos; en Python seria `PI = math.acos(-1)` y `E = math.exp(1)`. El
// `const` de ambito de archivo es como una constante privada del modulo.
const ldouble VALOR_PI = acos(-1.0L);
const ldouble VALOR_E = exp(1.0L);

vector<ldouble> sin2dual(vector<ldouble>& xval){

	vector<ldouble> f = {NAN,NAN,NAN};;

	f[0] = sin(xval[0]);
	f[1] = cos(xval[0])*xval[1];
	f[2] = -sin(xval[0])*pow(xval[1],2) + cos(xval[0])*xval[2];

	return f;
}

vector<ldouble> cos2dual(vector<ldouble>& xval){

	vector<ldouble> f = {NAN,NAN,NAN}; ;

	f[0] = cos(xval[0]);
	f[1] = -sin(xval[0])*xval[1];
	f[2] = -cos(xval[0])*pow(xval[1],2) -sin(xval[0])*xval[2];

	return f;
}

vector<ldouble> tan2dual(vector<ldouble>& xval){

	vector<ldouble> f = {NAN,NAN,NAN}; ;

	f[0] = tan(xval[0]);
	// tan(u)' = u'/cos^2(u): la derivada usa la componente dual xval[1]
	// (du/dx, la regla de la cadena), no la componente real xval[0].
	f[1] = xval[1]/(pow(cos(xval[0]),2));
	f[2] = xval[2]/pow(cos(xval[0]),2) + ((2.0*tan(xval[0]))/pow(cos(xval[0]),2))*pow(xval[1],2);

	return f;
}

// sinh2dual: version de segundo orden de sinh(u). En Python seria
// `[math.sinh(u), math.cosh(u)*du, math.sinh(u)*du**2 + math.cosh(u)*ddu]`;
// xval[1] y xval[2] son u' y u'' (la regla de la cadena), no la parte real.
vector<ldouble> sinh2dual(vector<ldouble>& xval){

	vector<ldouble> f = {NAN,NAN,NAN};

	f[0] = sinh(xval[0]);
	f[1] = cosh(xval[0])*xval[1];
	f[2] = sinh(xval[0])*pow(xval[1],2) + cosh(xval[0])*xval[2];

	return f;
}

// cosh2dual: version de segundo orden de cosh(u). En Python seria
// `[math.cosh(u), math.sinh(u)*du, math.cosh(u)*du**2 + math.sinh(u)*ddu]`.
vector<ldouble> cosh2dual(vector<ldouble>& xval){

	vector<ldouble> f = {NAN,NAN,NAN};

	f[0] = cosh(xval[0]);
	f[1] = sinh(xval[0])*xval[1];
	f[2] = cosh(xval[0])*pow(xval[1],2) + sinh(xval[0])*xval[2];

	return f;
}

// tanh2dual: version de segundo orden de tanh(u). En Python seria
// `[math.tanh(u), du/math.cosh(u)**2,
//   (ddu*math.cosh(u)**2 - 2*du**2*math.sinh(u)*math.cosh(u))/math.cosh(u)**4]`;
// el denominador cosh^4(u) sale de derivar 1/cosh^2(u) por la regla del
// cociente, no de la parte real.
vector<ldouble> tanh2dual(vector<ldouble>& xval){

	vector<ldouble> f = {NAN,NAN,NAN};

	f[0] = tanh(xval[0]);
	f[1] = xval[1]/pow(cosh(xval[0]),2);
	f[2] = (xval[2]*pow(cosh(xval[0]),2) - 2.0*pow(xval[1],2)*sinh(xval[0])*cosh(xval[0]))/pow(cosh(xval[0]),4);

	return f;
}

vector<ldouble> exp2dual(vector<ldouble>& xval){

	vector<ldouble> f(3);

	f[0] = exp(xval[0]);
	f[1] = exp(xval[0])*xval[1];
	f[2] = exp(xval[0])*pow(xval[1],2) + exp(xval[0])*xval[2];

	return f;
}

vector<ldouble> log2dual(vector<ldouble>& xval){

	vector<ldouble> f(3); ;

	f[0] = log(xval[0]);
	f[1] = (xval[1])/xval[0];
	f[2] = xval[2]/xval[0] - pow(xval[1],2)/pow(xval[0],2);

	return f;
}

vector<ldouble> division2dual(vector<ldouble>&x, vector<ldouble>& y){

	vector<ldouble> f(3) ;

	f[0] = (x[0]/y[0]);
	f[1] = ( y[0]*x[1]-x[0]*y[1] )/ pow(y[0],2);
	f[2] = x[2]/y[0]-2.0*x[1]*y[1]/pow(y[0],2)+x[0]*(2.0*pow(y[1],2)/pow(y[0],3)- y[2]/pow(y[0],2));

	return f;
}

// pow2dual: potencia de duales de segundo orden (u^v, con u y v funciones de
// x) con rama real y guardas explicitas en los casos limite (issue #19, B8 de
// REVIEW.md). `x = [u, u', u'']` es la base y `y = [v, v', v'']` el exponente.
//
// La version anterior calculaba con numeros complejos y descartaba siempre la
// parte imaginaria: x^2 en x=0 daba [0,NaN,NaN] aunque la derivada existe, y
// (-2)^(3+x) colaba valores finitos enganosos. Ahora:
//   u > 0  -> reglas reales de la potencia u^v = exp(v*ln u)
//   u = 0  -> limites finitos conocidos y NaN explicito cuando la derivada es
//             realmente infinita o la potencia indeterminada (0^0)
//   u < 0  -> real solo con exponente entero y constante; si no, NaN explicito
// En Python seria la misma cascada de `if/elif` con math.pow/math.log; `NAN`
// es float("nan") (aqui es el macro NAN de <cmath>).
vector<ldouble> pow2dual(vector<ldouble>&x, vector<ldouble>& y){

	// Arranca en NaN: toda rama que no asigne un valor deja NaN, que es la
	// politica explicita para los casos indefinidos (equivale a
	// `f = [float("nan"), float("nan"), float("nan")]`).
	vector<ldouble> f(3, NAN);

	ldouble u0 = x[0], u1 = x[1], u2 = x[2];
	ldouble v0 = y[0], v1 = y[1], v2 = y[2];

	if (u0 > 0.0L){
		// f = u^v, f' = f*(v*u'/u + v'*ln u) y f'' = f*(g^2 + g') con
		// g = d/dx(v*ln u); u > 0 garantiza ln u definido.
		ldouble uv = pow(u0, v0);
		ldouble lu = log(u0);
		ldouble g  = v0*u1/u0 + v1*lu;
		f[0] = uv;
		f[1] = uv*g;
		f[2] = uv*(g*g + 2.0L*v1*u1/u0 + v0*u2/u0 - v0*u1*u1/(u0*u0) + v2*lu);
		return f;
	}

	if (u0 == 0.0L){
		// Base cero: el valor 0^v0 es 0 para v0 > 0; las derivadas son
		// limites que solo existen con exponente constante (v1 = v2 = 0).
		if (v0 > 0.0L && v1 == 0.0L && v2 == 0.0L){
			f[0] = 0.0L;
			if (v0 == 1.0L){          // u^1 = u
				f[1] = u1;
				f[2] = u2;
			} else if (v0 == 2.0L){   // u^2: f'' = 2*u'^2 + 2*u*u'' con u=0
				f[1] = 0.0L;
				f[2] = 2.0L*u1*u1;
			} else if (v0 > 2.0L){    // v0 > 2: f, f' y f'' se anulan
				f[1] = 0.0L;
				f[2] = 0.0L;
			} else if (v0 > 1.0L){    // 1 < v0 < 2: f'=0, f'' infinita
				f[1] = 0.0L;
			}
			// 0 < v0 < 1: f' y f'' infinitas -> queda el NaN inicial.
		}
		return f;
	}

	// u0 < 0: u^v es real solo si el exponente es un entero constante; una
	// base negativa con exponente variable no tiene derivada real (ln(-u) no
	// existe). `floor(v0 + 0.5)` es el mismo redondeo que token_valido().
	ldouble k = floor(v0 + 0.5L);
	if (v0 != k || v1 != 0.0L || v2 != 0.0L){
		return f;   // NaN explicito
	}
	if (k == 0.0L){   // u^0 = 1 para u != 0 (0^0 ya quedo como NaN arriba)
		f[0] = 1.0L;
		f[1] = 0.0L;
		f[2] = 0.0L;
		return f;
	}
	// Regla de la potencia con exponente entero k:
	//   f = u^k, f' = k*u^(k-1)*u', f'' = k*(k-1)*u^(k-2)*u'^2 + k*u^(k-1)*u''
	f[0] = pow(u0, k);
	f[1] = k*pow(u0, k - 1.0L)*u1;
	f[2] = k*(k - 1.0L)*pow(u0, k - 2.0L)*u1*u1 + k*pow(u0, k - 1.0L)*u2;
	return f;
}


vector<ldouble> sqrt2dual(vector<ldouble>& xval){

	vector<ldouble> f(3);
	vector<ldouble> mediodr = {0.500,0.0,0.0};

	f = pow2dual(xval,mediodr);

	return f;

	}

vector<ldouble> prod2dual(vector<ldouble>&x, vector<ldouble>& y){

	vector<ldouble> f(3); ;

	f[0] = (x[0]*y[0]);
	f[1] = ( x[1]*y[0] + x[0]*y[1]);
	f[2] = x[2]*y[0]+ 2.0*x[1]*y[1] +x[0]*y[2];

	return f;
}

/*
 * =========================================================================
 * Esta partiene contine todas las funciones que operan sobre las derivadas
 * =========================================================================
*/
void imprime(vector<ldouble> n ){
	cout << "[";
	vector<ldouble> :: iterator it;
	it = n.begin();
	cout<< *it ;
	if (!n.empty()){
		for ( it=n.begin() +1;it !=n.end(); it++){
			cout << ", " << *it;
		}
	}
	cout << "]" ;
}

str parse_tree( vector<ldouble>& v){
/*
 * Esta funcion devuelve la funcion string, a partir de un vector,
 * crea el arbol para que visualmente se pueda ver la funcion
*/
	vector<string> fun;
	string f_string ="";

	int n = v.size();

	for (int i = 0; i< n ; i++){
		// `ldouble` no tiene equivalente exacto en Python; se convierte al
		// entero que identifica el token antes de consultar la plantilla comun.
		// En Python seria `fun.append(render(int(v[i])))`.
		fun.push_back(render(static_cast<int>(v[i])));
	}
		f_string = fun[0];

// fun es un vector que contiene algo como esto : [()+(),sin(),x,cos(),x]
// es funcion concatena las funciones, de fun

		for (int i = 1 ; i < (int) fun.size(); i++){
			std::size_t poss = f_string.find("()");
			if (poss !=std::string::npos){
				f_string.insert(poss+1,fun[i]);
			}
			// cout<< f_string << endl;
			 }
	return f_string;
}


int narg(ldouble x){
/*
* Esta funcion devuelve el numero de argumentos para evaular una operacion.
* Funciona como la tabla de aridad de la gramatica: 0 = hoja, 1 = funcion
* unaria, 2 = operador binario.
*/
	// `ldouble` no tiene equivalente exacto en Python: aqui el codigo llega
	// como long double y se convierte al `int` que usa la tabla de tokens.
	// En Python seria `return arity(int(x))`; la funcion compartida evita
	// duplicar los rangos del alfabeto en este consumidor.
	return arity(static_cast<int>(x));
}

vector<long double> OperF2(ldouble x,vector<vector<ldouble>>& vecval, int inicio) {
/*
 * Esta funcion opera cuando +*-/ cuando son 2 argumentos y despues aplica
 * NAN a todo lo demás
 *
 * `vecval` es la matriz completa [valor, d1, d2] de todo el arbol y `inicio`
 * es la columna desde la que hay que buscar los operandos (parsv le pasa
 * k+1). Es el equivalente C++ de la seccion `auxv(:,k+1:length)` que el
 * Fortran del paper (Ap. A) pasa POR REFERENCIA: como `vecval` tambien es
 * una referencia (`&`), borrar una columna aqui borra la auxv original de
 * parsv. En Python no existe ese paso por referencia implicito: una copia
 * de la lista no propagaria el borrado; habria que mutar la lista original.
*/

	int cont = 0,dim;
	const long double NANf = sqrt(-1);
	vector<ldouble> temp;
	temp.assign(3,NANf);
	vector<long double> f_result = {NAN,NAN,NAN};;
	vector<vector<ldouble>> auxiliar;
	dim=vecval[0].size();

	for (int i = 0 ; i < 3; i++){
		auxiliar.push_back(temp);
	}

	// Consumo de operandos (Algorithm 1 del paper): se recorre la sub-matriz
	// desde `inicio` y se toman las DOS primeras columnas cuya componente
	// real no sea NaN (las hojas las inicializa parsv; los operandos ya
	// consumidos quedan en NaN). El escaneo llega hasta el final
	// (`j < dim`; antes paraba en `dim - 1` y nunca miraba la ultima
	// columna) y borra la columna que consume (`j`; antes borraba la
	// siguiente, `j + 1`).
	for (int j = inicio; j < dim && cont < 2; j++){
	if(isnan(vecval[0][j])){
		continue;
		}
	else{
		auxiliar[0][cont]=vecval[0][j];
		auxiliar[1][cont]=vecval[1][j];
		auxiliar[2][cont]=vecval[2][j];
		// Borrado EN EL SITIO: NaN en la componente real de la columna
		// consumida, dentro de la matriz compartida con parsv. Asi la
		// operacion de la izquierda ya no ve este operando.
		vecval[0][j]=NANf;

		cont++;
	}
	}


	// Solo se opera si se encontraron dos operandos; si no, f_result queda
	// todo NaN (expresion incompleta, sin dos subarboles que comer).
	if (cont == 2){
	// Cada rama usa el nombre del token, como un `if token == TOKEN_ADD` de
	// Python; asi el dispatch permanece sincronizado con token.h.
	if (x == TOKEN_ADD){

		f_result[0]    = auxiliar[0][0] + auxiliar[0][1];
		f_result[1]    = auxiliar[1][0] + auxiliar[1][1];
		f_result[2]    = auxiliar[2][0] + auxiliar[2][1];
	}

	if (x == TOKEN_MUL) {
		vector<ldouble> x1(3),y(3);

		for (int h = 0; h < 3; h++){
			x1[h]=auxiliar[h][0];
			y[h]=auxiliar[h][1];
		}

		f_result = prod2dual(x1,y);
	}

	if (x == TOKEN_DIV) {
		vector<ldouble> x1(3),y(3);
		for (int h = 0; h < 3; h++){
			x1[h]=auxiliar[h][0];
			y[h]=auxiliar[h][1];
		}

		f_result = division2dual(x1,y);
	}
	if (x == TOKEN_POW) {
		vector<ldouble> x1(3),y(3);
		for (int h = 0; h < 3; h++){
			x1[h]=auxiliar[h][0];
			y[h]=auxiliar[h][1];
		}

		f_result = pow2dual(x1,y);
	}
	}

	return f_result;
}



vector<ldouble> OperF(ldouble x, vector<ldouble>& xval){

/*Esta funcion evalua las funciones ya descritas
 * en la poblacion incial, con un solo argumento.
 */
	vector<ldouble> f_result  = {NAN,NAN,NAN};

	// Los nombres del alfabeto son constantes de modulo; en Python seria
	// comparar contra `TOKEN_X`, `TOKEN_NEG`, etc., sin repetir sus enteros.
	if (x == TOKEN_X){
		f_result[0] = xval[0] ;
		f_result[1] = xval[1] ;
		f_result[2] = xval[2] ;
	}

	if (x == TOKEN_NEG) {
		f_result[0] = -1.0*xval[0] ;
		f_result[1] = -1.0*xval[1] ;
		f_result[2] = -1.0*xval[2] ;
	}
	if (x == TOKEN_SIN) f_result = sin2dual(xval);
	if (x == TOKEN_COS) f_result = cos2dual(xval);
	if (x == TOKEN_EXP) f_result = exp2dual(xval);
	if (x == TOKEN_LOG) f_result = log2dual(xval);
	if (x == TOKEN_SQRT) f_result = sqrt2dual((xval));
	// Despacha el token 22 hacia la regla dual ya existente; en Python seria
	// `if token == TOKEN_TAN: resultado = tan2dual(xval)`.
	if (x == TOKEN_TAN) f_result = tan2dual(xval);
	// Hiperbolicas del issue #61: mismo despacho por nombre de token; en
	// Python seria `if token == TOKEN_SINH: resultado = sinh2dual(xval)` y
	// analogamente para cosh y tanh.
	if (x == TOKEN_SINH) f_result = sinh2dual(xval);
	if (x == TOKEN_COSH) f_result = cosh2dual(xval);
	if (x == TOKEN_TANH) f_result = tanh2dual(xval);
	// Hojas polinomiales (issue #63): x^2 y x^3 se evaluan aplicando la regla
	// dual del producto sobre la entrada [x, 1, 0]. En Python seria
	// `if token == TOKEN_X2: resultado = prod2dual(xval, xval)`. prod2dual
	// recibe referencias no-const, asi que el cuadrado de x^3 se guarda en una
	// variable local antes de multiplicarlo otra vez por x (un temporal no
	// puede pasarse como referencia no-const; en Python no existe esa regla).
	if (x == TOKEN_X2) f_result = prod2dual(xval, xval);
	if (x == TOKEN_X3) {
		vector<ldouble> cuadrado = prod2dual(xval, xval);
		f_result = prod2dual(xval, cuadrado);
	}
	// Constantes matematicas (issue #64): pi y e son hojas de valor fijo con
	// derivadas cero; el resultado no depende de la entrada dual. En Python
	// seria `if token == TOKEN_PI: resultado = [math.pi, 0.0, 0.0]`.
	if (x == TOKEN_PI) f_result = {VALOR_PI, 0.0L, 0.0L};
	if (x == TOKEN_E) f_result = {VALOR_E, 0.0L, 0.0L};

	return f_result;
}


vector<ldouble> parsv(vector<ldouble> x, vector<ldouble> xval){

/*
 * Esta funcion es la principal, opera sobre todo el vector,
 * y evalua si es una funcion completa, para dar un valor f(x)
*/
	int dim, k=0;
	vector<ldouble> f_result(3) ;
	dim = x.size();
	vector<vector<ldouble>> auxv;
	const long double NANf = sqrt(-1);
	vector<ldouble> auxv1(dim);
	auxv1.assign(dim,NANf);

	for (int m = 0; m <(int) xval.size(); m++){
		auxv.push_back(auxv1);
	}


	for (k = 0; k < dim; k++ ){
		// Solo las constantes 1-10 son hojas numericas: su valor inicial es
		// el codigo del token. Con `||` la condicion era siempre verdadera
		// (todo numero cumple al menos una de las dos comparaciones), asi
		// que operadores y funciones tambien arrancaban con valor numerico y
		// las expresiones incompletas producian resultados falsos (p. ej.
		// [13,18] daba sin(18) en lugar de NaN).
		// En Python seria `if 1 <= x[k] <= 10:`.
		if(x[k] >= 1 && x[k] <= 10){
			 auxv[0][k]= (long double) 1.0*x[k];
			 auxv[1][k]= (long double) 0.0;
			 auxv[2][k]= (long double) 0.0;
		 }
		if(x[k] == 11){
			auxv[0][k]= xval[0];
			auxv[1][k]= xval[1];
			auxv[2][k]= xval[2];
		}

		// Hojas polinomiales (issue #63): x^2 y x^3 tienen aridad 0 (no
		// consumen operandos del arbol), pero su valor no es constante: se
		// calculan evaluando OperF con la entrada dual [x, 1, 0], igual que la
		// hoja x. En Python seria
		// `if token in (TOKEN_X2, TOKEN_X3): auxv[:, k] = OperF(token, xval)`.
		if(x[k] == TOKEN_X2 || x[k] == TOKEN_X3){
			vector<ldouble> hoja = OperF(x[k], xval);
			auxv[0][k] = hoja[0];
			auxv[1][k] = hoja[1];
			auxv[2][k] = hoja[2];
		}

		// Constantes pi y e (issue #64): aridad 0 y valor fijo, pero a
		// diferencia de las constantes 1.0-10.0 su valor no es el codigo del
		// token; se inicializan evaluando OperF, igual que las hojas
		// polinomiales, para no duplicar VALOR_PI/VALOR_E en este consumidor.
		// En Python seria
		// `if token in (TOKEN_PI, TOKEN_E): auxv[:, k] = OperF(token, xval)`.
		if(x[k] == TOKEN_PI || x[k] == TOKEN_E){
			vector<ldouble> hoja = OperF(x[k], xval);
			auxv[0][k] = hoja[0];
			auxv[1][k] = hoja[1];
			auxv[2][k] = hoja[2];
		}

		// Token 12 (negacion): NO se inicializa aqui. Es una funcion de un
		// argumento (narg(12) == 1) y el segundo bucle la evalua con
		// OperF(), que niega el valor del subarbol a su derecha (auxv[k+1]).
		// Antes se ponia a mano -xval, y eso negaba la *entrada* [x, y', y'']
		// en vez del argumento: p. ej. -sin(x) daba [-x, -1, 0] en lugar de
		// [-sin(x), -cos(x), sin(x)].
	}


	for (k = dim - 2; k>= 0; k--){
	if( narg(x[k]) == 1  && isnan(auxv[0][k+1]) == false   ){
		// `xuax` es la copia del operando [valor, d1, d2] que se pasa a
		// OperF (que no modifica su entrada). En Python seria
		// `xuax = auxv[:, k+1].copy()`.
		vector<long double> xuax(3) ;
		xuax[0] = auxv[0][k+1];
		xuax[1] = auxv[1][k+1];
		xuax[2] = auxv[2][k+1];

		vector<long double > temp(OperF(x[k],xuax));

		// Consumo EN EL SITIO del operando: la columna k+1 ya se uso, asi
		// que se borra (NaN en la componente real) para que las operaciones
		// de la izquierda no la vuelvan a ver, igual que el Fortran del
		// paper que pasa `auxv(:,k+1)` por referencia (Algorithm 1). Antes el
		// original quedaba vivo y el siguiente binario volvia a "comer" esta
		// misma hoja en lugar de su otro subarbol.
		auxv[0][k+1] = NANf;

		// Parada temprana (Algorithm 1): si la operacion produjo NaN, el
		// arbol entero ya no puede dar un valor finito, asi que se corta
		// aqui en vez de seguir evaluando operaciones a la izquierda (que
		// podrian "saltarse" el NaN y enganchar otros operandos). En Python:
		// `if math.isnan(temp[0]): return [float("nan")]*3`.
		if (isnan(temp[0])){
			return {NANf, NANf, NANf};
		}

		auxv[0][k] = temp[0];
		auxv[1][k] = temp[1];
		auxv[2][k] = temp[2];

	}
		else if( narg(x[k]) == 2  ){

		// OperF2 recibe la matriz completa `auxv` y desde que columna hay
		// que buscar (k+1), igual que el Fortran del paper que pasa
		// `auxv(:,k+1:length)` POR REFERENCIA (Algorithm 1). Antes se
		// copiaba la sub-matriz a `auxv321` y el borrado de los operandos
		// consumidos se quedaba en la copia: el arbol original seguia
		// viendo esas hojas y otro binario las volvia a "comer" (p. ej.
		// (x*x)+1 daba 12 en vez de 10). En Python seria pasar la lista
		// original y el indice, sin rebanarla con `auxv[:, k+1:]`.
		vector<ldouble> temp(OperF2(x[k],auxv,k+1));

		// Misma parada temprana que en el camino unario: un NaN devuelto
		// por la operacion corta la evaluacion (Algorithm 1).
		if (isnan(temp[0])){
			return {NANf, NANf, NANf};
		}

		auxv[0][k]=temp[0];
		auxv[1][k]=temp[1];
		auxv[2][k]=	temp[2];

			}
		}
	f_result[0] = auxv[0][0];
	f_result[1] = auxv[1][0];
	f_result[2] = auxv[2][0];


	return f_result ;
}
