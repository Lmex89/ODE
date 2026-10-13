#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>
#include <complex>
#include <stdlib.h>     /* srand, rand */
#include <time.h>
#include <algorithm>
#include <utility>
#include <sstream>
#include <iomanip>
#include "dualfunctions.h"
#include "semilla.h"

using namespace std;
using namespace std::complex_literals;
typedef long long int llint;
typedef  long double ldouble;
typedef string  str;

// x_rango: construye la rejilla de evaluacion como `npoints` puntos igualmente
// espaciados en [start, stop], ambos extremos incluidos. Antes el tercer
// parametro era un paso y el conteo salia de una division truncada
// (`int final = stop/step`): con paso 0.1 ese truncamiento daba 11 puntos en
// vez de los 100 que pide el paper (REVIEW C1). Ahora el conteo es explicito y
// cada punto se calcula con x_i = start + (stop-start)*i/(npoints-1), que en
// Python seria:
//     [start + (stop - start) * i / (npoints - 1) for i in range(npoints)]
// Cada punto se empaqueta como el 2dual [x, 1, 0] (eq. (14) del paper): x con
// dx/dx = 1 y d2x/dx2 = 0, la convencion que espera parsv().
vector<vector<long double>> x_rango(long double start,long double stop,int npoints){

	vector<vector<long double>> ans;

	for(int i = 0; i < npoints; i++){
		long double pos = start + (stop - start) * i / (npoints - 1);
		vector<long double> temp = {pos,1.0,0.0};
		ans.push_back(temp);
	}
	return ans;
}

class FunbObj{

	private:
	vector<ldouble> _vecval;
	string _repre;
	ldouble _fresult;
	// Cache de la fitness ya calculada (B6 de REVIEW.md): los individuos se
	// reemplazan enteros en el bucle DE (`Population[j] = candidato`), nunca
	// se mutan, asi que el valor cacheado no puede quedar obsoleto. En Python
	// serian `self._cache_valido = False` y `self._fresult`.
	bool _cache_valido = false;
	vector<vector<ldouble>> _xrange;
	vector<long double> _xinitials = {0.0,1.0,0.0};
	// Caso del problema que mide esta fitness (lo elige ODE_CASE en main()):
	//   0 = juguete historico del prototipo: y'' + y = 0, y(0)=1, y'(0)=1
	//   1 = Caso 1 del paper, ecuacion (18): x*y'' + y' - cos(x) = 0,
	//       y(0)=0, y'(0)=1, con solucion exacta la integral del seno Si(x)
	// Es el mismo patron que ODE_SEED: un numero de caso en vez de una bandera
	// por problema. En Python seria `self._caso = caso`.
	int _caso;

	public:
	// caso: selecciona la ODE que evalua fitness() (ver _caso). Tiene valor por
	// defecto 0 para que el resto del programa y los tests que ya construian
	// FunbObj con 3 argumentos sigan comportandose igual. En Python seria
	// `def __init__(self, vecval, start, stop, caso=0)`.
	FunbObj(vector<ldouble> vecval,ldouble start,ldouble stop, int caso = 0){
		// Rejilla de evaluacion: los 100 puntos igualmente espaciados de [0,1]
		// que pide el paper (§5). En Python seria
		// `self._xrange = x_rango(start, stop, 100)`.
		_xrange = x_rango(start,stop,100);
		_repre = parse_tree(vecval);
		_xrange[0] = _xinitials;
		_vecval = vecval;
		_caso = caso;
	}
	string represant(){
		return _repre;
	}
	vector<ldouble> vecvalue(){
		return _vecval;
	}
	long double fitness(){	
		// Cache por individuo: el bucle DE pide la fitness varias veces (la
		// comparacion del trial contra el objetivo y, en el barrido del mejor
		// de la generacion, dos por comparacion); recalcular la rejilla de 100
		// puntos en cada llamada era trabajo repetido. En Python seria
		// `if self._cache_valido: return self._fresult`.
		if (_cache_valido) return _fresult;

		ldouble res = 0 ;
		string fun = parse_tree(_vecval);

		std::size_t poss = fun.find("()");
		if (poss !=std::string::npos){
			_fresult  = INFINITY;
			_cache_valido = true;
			return _fresult ;
			}
	std::vector<ldouble>::iterator it = std::find(_vecval.begin(),_vecval.end(),11.0);
	if (it == _vecval.end()){
		// Sin token x (11) la expresion no es funcion de x: INFINITY sin
		// tocar el vector. Antes se escribian 11s al azar y luego se devolvia
		// INFINITY igual: la evaluacion mutaba al individuo y una segunda
		// llamada podia devolver otra cosa (B6 de REVIEW.md). Los productores
		// (vector_population, Mutation, Recombination) ya garantizan un token
		// x, asi que aqui solo se penaliza.
		_fresult  = INFINITY;
		_cache_valido = true;
		return _fresult ;
 }				
	// Condicion inicial en x=0: [y(0), y'(0), y''(0)]. Antes se calculaba
	// tambien `b_end` (la misma evaluacion en x=1) y nunca se usaba; se
	// elimino (REVIEW C4, plan de correccion item 16).
	vector<ldouble> a_start = parsv(_vecval,_xrange[0]);
	// Cualquier componente NaN invalida el individuo completo. En Python seria
	// `any(math.isnan(componente) for componente in a_start)`; no hay una
	// conversion automatica de NaN a una excepcion en este codigo C++.
	if (isnan(a_start[0]) || isnan(a_start[1]) || isnan(a_start[2])){
		_fresult = INFINITY;
		_cache_valido = true;
		return _fresult;
	}

	for (	int i = 0 ; i < (int)_xrange.size(); i++){
		vector<ldouble> fitnessval(parsv(_vecval,_xrange[i]));
		//  imprime(_vecval); cout << endl;
		//  cout << parse_tree(_vecval) << endl;
			
		//   cout << endl; cout << i << " " ;
		//  imprime(_xrange[i]) ; cout << "despues de _range" << endl;
		// imprime(fitnessval);
		// cout <<" <<<<<"  << endl;
		// imprime(a_start);
		// cin.get();

		// Cualquier componente NaN hace que todo el individuo sea no evaluable.
		// En Python equivale a `if any(math.isnan(c) for c in fitnessval):`
		// seguido de `return math.inf`; asi no existe una penalizacion finita
		// distinta para un punto sin informacion util.
		if (isnan(fitnessval[0]) || isnan(fitnessval[1]) || isnan(fitnessval[2])){
			_fresult = INFINITY;
			_cache_valido = true;
			return _fresult;
		}

		// Residuo de la ODE en el punto de la rejilla, elevado a la cuarta,
		// segun el caso (ver _caso). En Python seria `res += r**4`.
		if (_caso == 1){
			// Caso 1 del paper: x*y'' + y' - cos(x) = 0. Se evita
			// dividir por x=0, que es el primer punto de la rejilla.
			ldouble x = _xrange[i][0];
			res +=pow(x*fitnessval[2] + fitnessval[1] - cos(x),4) ;
		} else {
			// Caso 0 (juguete del prototipo): y'' + y = 0.
			res +=pow(fitnessval[2] + fitnessval[0],4) ;
		}
		}
			// Condiciones iniciales del caso. Caso 0: y(0)=1, y'(0)=1 (la
			// solucion esperada es cos(x)+sin(x)); Caso 1 del paper: y(0)=0,
			// y'(0)=1. Mismos pesos y exponente que el caso 0 para que la
			// magnitud de la fitness siga siendo comparable.
			ldouble y0_objetivo = (_caso == 1) ? 0.0L : 1.0L;
			res +=300*pow((a_start[0] - y0_objetivo),4) + 700*pow((a_start[1] - 1),4);
		// cout << "imprimimos res " << endl;
		// cout << res << endl;
		// cin.get();
	if (isnan(res)){
		_fresult = INFINITY;
		_cache_valido = true;
		return _fresult;
	}
	_fresult = res;
	_cache_valido = true;
	return _fresult;
	}

};

vector<long double> vector_population(int tam){
		vector<ldouble> ans;
 bool flag = true;
 int cont  = 0;


 while(flag){
	 ldouble rand_num = rand() % 21 + 1;
	 ans.push_back(rand_num);
	 cont++;
	 if (cont == tam ){
		 flag = false;
		 break;
	 }

 }

	 std::vector<ldouble>::iterator it = std::find(ans.begin(),ans.end(),11.0);
	 if (it == ans.end()){
	for (int i = 0 ; i < 2 ; i++){
		ldouble rand_num = (rand() % tam) ;
		if ((rand_num <= tam - 4) && (rand_num >= 0)){
			ans[rand_num] = 11;
		}

	 }
 }
	 return ans;
}


void createPopulation(vector<FunbObj>&Population,int tam,int caso){

	//vector<ldouble> uno =   {21,11,2,1,1,1,2,11,11,11,11,11,1,1,1,1,1,1,1,1};
	// Los cinco seeds escritos a mano miden exactamente 20 tokens, igual que
	// los individuos aleatorios de vector_population(20): el semillero tiene
	// que ser uniforme porque Mutation()/Recombination() emparejan genes por
	// posicion (B5 de REVIEW.md: con dos=21 se leia y[20]/z[20] fuera de
	// rango). En Python serian listas de 20 elementos, con
	// `assert len(semilla) == 20` como comprobacion.
	vector<ldouble> uno =   {18,13,17,11,14,14,11,14,14,14,13,14,14,14,14,14,14,14,14,11};
	vector<ldouble> dos =   {18,14,13,17,11,14,20,1,5,1,1,1,1,1,1,1,1,1,1,1};
	vector<ldouble> tres =  {18,13,11,13,13,14,14,14,11,1,1,1,1,1,1,1,1,1,1,1};
	vector<ldouble> cuatro ={18,14,11,13,13,11,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	vector<ldouble> c5 =    {18,13,13,13,11,13,11,11,1,1,1,1,1,1,1,1,1,1,1,1};

	// `caso` se propaga a cada FunbObj: la representacion (los tokens) no
	// cambia entre casos, solo la ODE que mide fitness(). En Python seria
	// `FunbObj(uno, 0, 1, caso)` y asi para las demas semillas.
	FunbObj temp(uno,0,1,caso);
	Population.push_back(temp);
	FunbObj temp1(dos,0,1,caso);
	Population.push_back(temp1);
	FunbObj temp2(tres,0,1,caso);
	Population.push_back(temp2);

	FunbObj temp3(cuatro,0,1,caso);
	Population.push_back(temp3);

	FunbObj temp4(c5,0,1,caso);
	Population.push_back(temp4);

	// Se descuentan las cinco semillas fijas: 5 + (tam - 5) = tam individuos
	// exactos. Antes era `tam - 4` y la poblacion terminaba con un individuo de
	// mas (off-by-one de REVIEW C5: se pedia tam y se creaba tam + 1).
	for (int  i = 0 ; i < tam - 5 ; i++){

		vector<ldouble> temporal(vector_population(20));
		FunbObj temp(temporal,0,1,caso);
		Population.push_back(temp);
	}
}


// mutacion_rand1: nucleo de la mutacion DE/rand/1/bin, ecuacion (7) del paper:
//
//     v = x_r1 + F * (x_r2 - x_r3)
//
// Es una funcion *pura* (sin rand(), sin estado): por eso se puede testear con
// valores a mano. En Python seria `def mutacion_rand1(r1, r2, r3, F): return
// r1 + F * (r2 - r3)`. Se separa de Mutation() para poder probar la formula
// sola, sin depender del sorteo de F.
ldouble mutacion_rand1(ldouble r1, ldouble r2, ldouble r3, ldouble F){
	return r1 + F * (r2 - r3);
}

// token_valido: pasa el valor real de la ecuacion (7) a un token entero del
// alfabeto (1..21). Redondea al entero mas cercano y recorta los extremos.
//
// Detalle C++ sin equivalente directo en Python: se devuelve `int` y no
// `long double` porque un token es un codigo, no una cantidad; el redondeo se
// hace con floor(v + 0.5) (equivalente a round() pero sin la ambiguedad de
// round() con los .5 negativos) y el recorte con dos `if` en vez de min/max
// para que quede explicito. En Python seria:
//     def token_valido(v): return min(21, max(1, floor(v + 0.5)))
int token_valido(ldouble v){
	ldouble redondeado = floor(v + 0.5L);
	if (redondeado < 1.0L) return 1;
	if (redondeado > 21.0L) return 21;
	return (int) redondeado;
}

// indices_distintos: sortea tres indices de poblacion distintos entre si y
// distintos de `excluir` (el individuo objetivo). Son los x_r1, x_r2, x_r3 de
// la ecuacion (7): el paper exige individuos distintos para que la diferencia
// (x_r2 - x_r3) no sea siempre cero.
//
// Los parametros `int& r1` son *referencias*: en C++ equivalen a un parametro
// de salida (Python tendria que devolver una tupla, `return r1, r2, r3`).
// El sorteo se hace por rechazo: se tira rand() hasta que no toque un indice
// repetido. Con poblaciones de ~275 individuos el bucle casi nunca repite.
void indices_distintos(int tam, int excluir, int& r1, int& r2, int& r3){
	// Con menos de 4 individuos no existen 3 indices distintos del objetivo;
	// se devuelven los que haya para no quedarse en un bucle infinito (solo
	// puede pasar si alguien achica la poblacion a mano).
	if (tam < 4) {
		r1 = 0;
		r2 = (tam > 1) ? 1 : 0;
		r3 = (tam > 2) ? 2 : 0;
		return;
	}
	do { r1 = rand() % tam; } while (r1 == excluir);
	do { r2 = rand() % tam; } while (r2 == excluir || r2 == r1);
	do { r3 = rand() % tam; } while (r3 == excluir || r3 == r1 || r3 == r2);
}

// ---------------------------------------------------------------------------
// Operadores DE/rand/1/bin (issue #5).
// ---------------------------------------------------------------------------

// Mutation: version DE/rand/1/bin con dither, ecuacion (7) del paper. Aplica,
// token a token, v = x_r1 + F*(x_r2 - x_r3), donde x_r1/x_r2/x_r3 son los tres
// individuos base que recibe (el llamador garantiza que son distintos) y F es
// un unico aleatorio en [0,1] por llamada (parte "dither": F cambia en cada
// generacion, no es una constante del algoritmo). El real resultante se
// convierte a un token valido con token_valido().
//
// Nota C++ para un desarrollador Python: los tres parametros entran *por
// valor* (una copia de cada vector, como una lista copiada en Python) y la
// funcion devuelve un vector nuevo; nadie muta los padres.
vector<ldouble> Mutation(vector<ldouble>x, vector<ldouble> y,vector<ldouble>z){

	// Se recorre hasta el menor de los tres tamanos: si dos vectores midieran
	// distinto (p. ej. un seed mal escrito, B5 de REVIEW.md), leer y[i]/z[i]
	// de mas seria una lectura fuera de rango. En Python seria
	// `n = min(len(x), len(y), len(z))` (aqui `min` es std::min, de <algorithm>).
	int n = (int) min(x.size(), min(y.size(), z.size()));

	// Se parte de una copia de x: asi, si x es mas largo que y/z, los tokens
	// sobrantes sobreviven intactos en vez de quedar indefinidos.
	vector<ldouble> res(x);

	// Dither: un rand() por llamada. rand()/RAND_MAX da un real en [0,1]
	// (rand() devuelve enteros en [0,RAND_MAX]); en Python seria
	// `F = random.random()`.
	ldouble F = (ldouble) rand() / (ldouble) RAND_MAX;

	for(int i = 0 ; i < n; i++ ){
		res[i] = token_valido(mutacion_rand1(x[i], y[i], z[i], F));
	}

	// Reparacion heredada: la fitness descarta los arboles sin token x (11),
	// asi que si la mutacion los borro todos se fuerza uno en una posicion al
	// azar (un solo token; antes se intentaban 3 posiciones con un rango
	// dudoso). Es una regla propia de este prototipo, no del paper.
	if (res.size() > 0 && std::find(res.begin(), res.end(), 11.0) == res.end()){
		res[rand() % res.size()] = 11;
	}

	return res;
}

// ---------------------------------------------------------------------------
// Cruce binomial (issue #6), ecuacion (8) del paper.
// ---------------------------------------------------------------------------

// gen_binomial: decision *pura* del cruce para un gen, ecuacion (8): con
// probabilidad Cr el trial hereda el gen del mutante (v) y con probabilidad
// 1-Cr el del objetivo (x). La tirada `r` (en [0,1]) llega como parametro para
// poder probar la regla con valores a mano, sin rand(); Recombination() es
// quien sortea un r por gen. En Python seria:
//     def gen_binomial(objetivo, mutante, r, Cr): return mutante if r < Cr else objetivo
// La comparacion es estricta (r < Cr), igual que en el enunciado del issue.
ldouble gen_binomial(ldouble objetivo, ldouble mutante, ldouble r, ldouble Cr){
	return (r < Cr) ? mutante : objetivo;
}

// Recombination: cruce binomial del paper, ecuacion (8). Construye el vector
// trial mezclando, gen a gen, el individuo objetivo (x) con el mutante (v):
// con probabilidad Cr el gen viene del mutante y en caso contrario del
// objetivo. Ademas el indice j_rand se fuerza al mutante para que el trial
// nunca sea una copia exacta del objetivo (asi lo pide el paper; REVIEW D2).
//
// Nota C++ para un desarrollador Python: los padres entran por referencia
// constante (no se copian) y el resultado se devuelve por valor; la funcion no
// modifica a nadie, a diferencia de la version anterior que intercambiaba
// genes "in place" entre dos vectores.
vector<ldouble> Recombination(const vector<ldouble>& objetivo, const vector<ldouble>& mutante, ldouble Cr){

	// Se recorre hasta el menor de los dos tamanos (los individuos semilla
	// tienen 21 tokens y el resto 20): leer mutante[i] de mas seria una
	// lectura fuera de rango. En Python seria
	// `n = min(len(objetivo), len(mutante))`.
	int n = (int) min(objetivo.size(), mutante.size());

	// Se parte de una copia del objetivo: si el objetivo es mas largo que el
	// mutante, sus tokens sobrantes sobreviven intactos.
	vector<ldouble> trial(objetivo);

	// Con vectores vacios no hay genes que cruzar (ni j_rand que sortear).
	if (n <= 0) return trial;

	// j_rand: indice sorteado que siempre toma el gen del mutante.
	// `rand() % n` es `random.randrange(n)` en Python.
	int j_rand = rand() % n;

	for (int i = 0; i < n; i++){
		// Un rand() por gen, como el `if rand() < Cr` del issue: rand()/RAND_MAX
		// da un real en [0,1] (random.random()).
		ldouble r = (ldouble) rand() / (ldouble) RAND_MAX;
		if (i == j_rand){
			trial[i] = mutante[i];
		} else {
			trial[i] = gen_binomial(objetivo[i], mutante[i], r, Cr);
		}
	}

	// Reparacion heredada (igual que en Mutation()): la fitness descarta los
	// arboles sin token x (11), asi que si el cruce los borro todos se fuerza
	// uno en una posicion al azar. Es una regla propia de este prototipo, no
	// del paper.
	if (trial.size() > 0 && std::find(trial.begin(), trial.end(), 11.0) == trial.end()){
		trial[rand() % trial.size()] = 11;
	}

	return trial;
}

// Guarda de compilacion para los tests: `-DODE_NO_MAIN` descarta este bloque
// (y con el, main()) al compilar parse_tree.cpp dentro de un test. En Python no
// existe nada parecido: un modulo no "contiene" el entry point, asi que no hay
// que apagarlo; importarlo simplemente no ejecuta la busqueda.
#ifndef ODE_NO_MAIN

// ===========================================================================
// Log de main(): impresion con formato profesional.
//
// Cada linea sigue el patron por defecto de spdlog, la libreria de logging mas
// usada en C++ (timestamp + nivel + mensaje), pero implementado con iostreams
// de la STL: el repo no tiene gestor de paquetes, asi que no se agrega ninguna
// dependencia externa. Forma de cada linea:
//
//     [  0.12s] [INFO ] de_run_start | seed=42 case=0 population=276 ...
//
// - "[  0.12s]": tiempo de CPU desde el arranque, ancho fijo 6 y 2 decimales.
//   En Python seria f"{t:6.2f}s" con t = time.process_time() - t0.
// - "[INFO ]"/"[WARN ]": nivel del mensaje, siempre 5 caracteres (con relleno)
//   para que los mensajes empiecen en la misma columna; spdlog usa etiquetas
//   como "[info]" y "[warning]".
// - Mensaje: "evento | clave=valor ..." (logging estructurado): legible por
//   una persona y parseable con split() desde un script; en Python seria
//   f"gen={gen} best={fit:.6e}" (una f-string).
//
// Contrato con scripts/ab_experiment.sh: ese script extrae las claves seed,
// gen, generations, best, expr, vector, status y elapsed_cpu_s de estas
// lineas; renombrar una clave obliga a actualizar el awk del script.
//
// C++ Core Guidelines (F.3): se imprime '\n' y un flush explicito
// (cout.flush(), el sys.stdout.flush() de Python) en vez de std::endl. Aqui el
// flush es deliberado, no incidental: la salida se consume en vivo (`tail`) y
// un `timeout` puede matar el proceso en cualquier momento, asi que cada linea
// de main se vacia al emitirse (igual que el sink de consola de spdlog); endl
// queda evitado en el resto del programa, donde nadie lo necesita. '\n' +
// flush() es exactamente lo que endl hace por dentro.
// ===========================================================================

// Instante de arranque para el timestamp de cada linea. `static` a nivel de
// archivo significa visibilidad solo dentro de este .cpp (en Python no hay un
// equivalente exacto: es como un nombre con guion bajo a nivel de modulo, pero
// impuesto por el compilador).
static clock_t g_log_t0 = 0;

// fmt_fijo: numero con `decimales` decimales fijos, como f"{v:.2f}" en Python.
// ostringstream es el buffer de texto de la STL (la f-string de C++): se arma
// el string con el formato pedido por <iomanip> y .str() lo devuelve, sin
// tocar el estado global de cout.
string fmt_fijo(ldouble v, int decimales){
	ostringstream os;
	os << fixed << setprecision(decimales) << v;
	return os.str();
}

// fmt_ciencia: notacion cientifica con 6 decimales (f"{v:.6e}" en Python), el
// formato de la fitness: valores muy chicos o INFINITY, que sale "inf" igual
// que en Python.
string fmt_ciencia(ldouble v){
	ostringstream os;
	os << scientific << setprecision(6) << v;
	return os.str();
}

// fmt_segundos: segundos con 2 decimales y ancho 6 (f"{v:6.2f}" en Python).
// Solo para el timestamp que abre cada linea.
string fmt_segundos(ldouble v){
	ostringstream os;
	os << fixed << setprecision(2) << setw(6) << v;
	return os.str();
}

// fmt_vector: el individuo como "[21,11,14,20]"
// ("[" + ",".join(str(int(t)) for t in v) + "]" en Python). Sin espacios a
// proposito: asi el vector es UN solo campo clave=valor y la linea se puede
// partir con split(). El parametro entra por referencia constante (const&),
// que en C++ evita copiar el vector entero, como pasar una lista en Python
// sin copiarla.
string fmt_vector(const vector<ldouble>& v){
	ostringstream os;
	os << "[";
	for (size_t i = 0; i < v.size(); i++){
		if (i > 0) os << ",";
		os << (long long) v[i];  // los tokens son enteros 1..21
	}
	os << "]";
	return os.str();
}

// segundos_cpu: CPU transcurrida desde g_log_t0 (time.process_time() - t0 en
// Python). clock() mide tiempo de CPU del proceso, no tiempo de pared.
ldouble segundos_cpu(){
	return (ldouble)(clock() - g_log_t0) / CLOCKS_PER_SEC;
}

// log_linea: imprime "[<t>s] [<NIVEL>] " y devuelve cout para que el llamador
// complete el mensaje y cierre con '\n'; en Python seria logging.info(f"...").
// Devolver el stream (ostream&) es el idioma de C++ para encadenar `<<`: el
// llamador sigue escribiendo campos sobre el mismo renglon. El nivel llega con
// el relleno ya puesto ("INFO ").
ostream& log_linea(const char* nivel){
	cout << '[' << fmt_segundos(segundos_cpu()) << "s] [" << nivel << "] ";
	return cout;
}

// Atajos por nivel, como logging.info y logging.warning de Python.
ostream& log_info(){
	return log_linea("INFO ");
}

ostream& log_warn(){
	return log_linea("WARN ");
}

int main() {

	// Arranque del reloj de CPU: alimenta el timestamp de cada linea y el
	// tiempo total del resumen (en Python: t0 = time.process_time()).
	g_log_t0 = clock();

	// Semilla controlada por ODE_SEED (issue #4): con ODE_SEED=42 todas las
	// corridas salen identicas (mismo fitness, mismo vector), que es lo que
	// exige el experimento A/B. Sin la variable se usa el reloj, como antes.
	// En Python seria: random.seed(os.environ.get("ODE_SEED")).
	long semilla = resolver_semilla(getenv("ODE_SEED"), (long) time(NULL));
	srand ((unsigned) semilla);

	// Caso del problema, controlado por ODE_CASE (mismo patron que ODE_SEED):
	//   0 (o ausente) -> juguete historico y'' + y = 0,  y(0)=y'(0)=1
	//   1            -> Caso 1 del paper: x*y'' + y' - cos(x) = 0 (ec. (18)),
	//                   y(0)=0, y'(0)=1, solucion exacta Si(x)
	// En Python seria `caso = int(os.environ.get("ODE_CASE", "0"))` (aqui
	// atoi() convierte el string del entorno a entero, como int()).
	int caso = 0;
	const char* caso_env = getenv("ODE_CASE");
	if (caso_env != NULL){
		caso = (atoi(caso_env) == 1) ? 1 : 0;
	}

	int tam = 275;
	// Una sola poblacion (issue #6): antes habia dos (Population1/Population2)
	// que se cruzaban entre si; DE/rand/1/bin (paper) evoluciona una unica
	// poblacion generacion a generacion.
	vector<FunbObj> Population;
	createPopulation(Population, tam, caso);

	// Cr: probabilidad de heredar el gen del mutante en el cruce binomial
	// (eq. (8)). Antes era `int Cr = 85`, un umbral comparado contra el tamano
	// de poblacion; el paper (§5, REVIEW D9) usa 0.2.
	ldouble Cr = 0.2;
	long double eps = 0.00001;
	long double Bestfit = INFINITY;
	vector<ldouble> Bestvect;
	int gen = 0;
	bool flag = true;
	const int max_gen = 100000;         // tope de generaciones del bucle
	string estado = "max_generations";  // pasa a "converged" si llega a eps

	// Cabecera de la corrida (reemplaza el banner de ASCII art): una sola
	// linea con toda la configuracion en clave=valor. `population=` informa el
	// tamano real de la poblacion ya creada (createPopulation siembra 5
	// individuos fijos ademas de los aleatorios).
	log_info() << "de_run_start | seed=" << semilla
	           << " case=" << caso
	           << " population=" << Population.size()
	           << " cr=" << fmt_fijo(Cr, 2)
	           << " fitness_target=" << fmt_ciencia(eps)
	           << " max_generations=" << max_gen << '\n';

	// Descripcion legible del caso: el dato maquina es el campo `case=` de
	// arriba; este renglon es la explicacion y va como texto libre (sin
	// clave=valor) para no confundir al parser del script A/B.
	log_info() << "ode_case | " << caso << ": ";
	if (caso == 1){
		cout << "paper, ec. (18): x*y'' + y' - cos(x) = 0, y(0)=0, y'(0)=1";
	} else {
		cout << "juguete: y'' + y = 0, y(0)=1, y'(0)=1";
	}
	cout << '\n';
	cout.flush();

	while(flag){

		// La generacion se cuenta 1-based desde el principio: asi el campo
		// `generations=` del resumen final cuenta las generaciones corridas
		// (antes el contador incrementaba al final y el ultimo avance impreso
		// era 100001).
		gen++;

		// Una generacion de DE/rand/1/bin: por cada individuo objetivo se
		// sortean los tres individuos base (distintos entre si, eq. (7)), se
		// muta, se cruza (eq. (8)) y se selecciona el mejor de los dos.
		for (int j = 0; j < (int) Population.size(); j++){

			int r1, r2, r3;
			indices_distintos((int) Population.size(), j, r1, r2, r3);
			vector<ldouble> mutante(Mutation(Population[r1].vecvalue(), Population[r2].vecvalue(), Population[r3].vecvalue()));
			vector<ldouble> trial(Recombination(Population[j].vecvalue(), mutante, Cr));

			// Seleccion DE estandar (greedy, uno a uno): el trial reemplaza al
			// objetivo solo si su fitness es estrictamente menor. Es el
			// `if fitness(trial) < fitness(objetivo): poblacion[j] = trial`
			// clasico de Python; ya no hay elitismo por sorting ni
			// cross-pollination entre poblaciones.
			FunbObj candidato(trial, 0, 1, caso);
			if (candidato.fitness() < Population[j].fitness()){
				Population[j] = candidato;
			}
		}

		// Mejor individuo de la generacion (la fitness mas baja). Se busca con
		// un barrido simple en vez de ordenar la poblacion entera.
		int mejor = 0;
		for (int j = 1; j < (int) Population.size(); j++){
			if (Population[j].fitness() < Population[mejor].fitness()){
				mejor = j;
			}
		}
		ldouble BestCurrent = Population[mejor].fitness();

		// Avance de la generacion: mejor fitness de la generacion, record
		// historico y su expresion. Con flush: aunque es la linea de alta
		// frecuencia (hasta 100000), tiene que verse en vivo (`tail`) y
		// sobrevivir si un `timeout` mata el proceso (ver la cabecera).
		log_info() << "gen=" << gen << " best=" << fmt_ciencia(BestCurrent)
		           << " best_ever=" << fmt_ciencia(Bestfit)
		           << " expr=" << Population[mejor].represant() << '\n';
		cout.flush();

		if (BestCurrent < Bestfit){
			Bestfit = BestCurrent;
			Bestvect = Population[mejor].vecvalue();
			// Record nuevo: una sola linea (antes eran cinco con adornos) con
			// su flush, para que quede en el log aunque un `timeout` mate el
			// proceso a mitad de corrida.
			log_info() << "new_best | gen=" << gen << " best=" << fmt_ciencia(Bestfit)
			           << " expr=" << parse_tree(Bestvect)
			           << " vector=" << fmt_vector(Bestvect) << '\n';
			cout.flush();
		}

		if ( Bestfit < eps){
			estado = "converged";
			flag = false;
		}

		if(gen >= max_gen){
			flag = false;
		}
	}

	// Resultado final: la solucion en una linea con su flush. Solo si hubo
	// algun individuo finito (Bestvect vacio = toda la poblacion dio fitness
	// infinita; no hay nada que mostrar).
	if (!Bestvect.empty()){
		log_info() << "solution | best=" << fmt_ciencia(Bestfit)
		           << " expr=" << parse_tree(Bestvect)
		           << " vector=" << fmt_vector(Bestvect) << '\n';
	} else {
		log_warn() << "no_solution | best=" << fmt_ciencia(Bestfit) << '\n';
	}
	cout.flush();

	// Si el bucle se corto por el tope de generaciones no se alcanzo la
	// tolerancia: es una advertencia, no una solucion.
	if (estado != "converged"){
		log_warn() << "no_convergence | reason=" << estado
		           << " generations=" << gen
		           << " fitness_target=" << fmt_ciencia(eps) << '\n';
	}
	cout.flush();

	// Resumen de la corrida: estado, generaciones, record y tiempo de CPU
	// (el viejo "Execution Time", ahora como campo estructurado).
	log_info() << "de_run_end | status=" << estado
	           << " generations=" << gen
	           << " best=" << fmt_ciencia(Bestfit)
	           << " elapsed_cpu_s=" << fmt_fijo(segundos_cpu(), 2) << '\n';
	cout.flush();

	// Firma del autor, conservada del prototipo original.
	cout << "Por Lmex" << '\n';

	return 0;
}
#endif // ODE_NO_MAIN
