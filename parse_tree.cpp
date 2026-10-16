#include <iostream>
#include <array>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <sstream>
#include <iomanip>
#include "dualfunctions.h"
#include "semilla.h"
#include "token.h"

using namespace std;
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

	// Precondicion: npoints >= 2 para evitar division por cero en (npoints-1).
	// Con npoints < 2, la formula x_i = start + (stop-start)*i/(npoints-1)
	// divide por cero o genera un solo punto sin sentido. En Python seria
	// `assert npoints >= 2, "grid requires at least 2 points"`.
	assert(npoints >= 2);

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
	// _fresult es mutable porque fitness() es const pero necesita cachear el
	// resultado. En Python sería @cached_property: método const con estado interno.
	mutable ldouble _fresult;
	// Cache de la fitness ya calculada (B6 de REVIEW.md): los individuos se
	// reemplazan enteros en el bucle DE (`Population[j] = candidato`), nunca
	// se mutan, así que el valor cacheado no puede quedar obsoleto. Es mutable
	// porque fitness() es const pero debe poder actualizar la caché. En Python
	// serían `self._cache_valido = False` y `self._fresult`.
	mutable bool _cache_valido = false;
	vector<vector<ldouble>> _xrange;
	vector<long double> _xinitials = {0.0,1.0,0.0};
	// Factor de integración Delta x de la suma de Riemann de la ec. (4):
	// con los 100 puntos igualmente espaciados de [0,1] y ambos extremos
	// incluidos, el paso es (stop-start)/(npoints-1) = 1/99. En Python sería
	// `self._dx = (stop - start) / (len(self._xrange) - 1)`; long double no
	// tiene equivalente exacto en Python (es más ancho que float).
	long double _dx;
	// Caso del problema que mide esta fitness (lo elige ODE_CASE en main()):
	//   0 = juguete historico del prototipo: y'' + y = 0, y(0)=1, y'(0)=1
	//   1 = Caso 1 del paper, ecuacion (18): x*y'' + y' - cos(x) = 0,
	//       y(0)=0, y'(0)=1, con solucion exacta la integral del seno Si(x)
	//   2 = caso hiperbolico (issue #61): y'' - y = 0, y(0)=1, y'(0)=0,
	//       con solucion exacta cosh(x); la busqueda solo puede alcanzarlo
	//       desde que existe TOKEN_COSH.
	// Es el mismo patron que ODE_SEED: un numero de caso en vez de una bandera
	// por problema. En Python seria `self._caso = caso`.
	int _caso;

	public:
	// caso: selecciona la ODE que evalua fitness() (ver _caso). Tiene valor por
	// defecto 0 para que el resto del programa y los tests que ya construian
	// FunbObj con 3 argumentos sigan comportandose igual. En Python seria
	// `def __init__(self, vecval, start, stop, caso=0)`.
	// El constructor usa move semantics para el vector: std::move(vecval) evita
	// copiar el vector dos veces (una en el parámetro, otra en el miembro).
	// En Python no existe este detalle porque las listas siempre son referencias;
	// en C++ el move evita el overhead de copiar el vector completo.
	FunbObj(vector<ldouble> vecval,ldouble start,ldouble stop, int caso = 0)
	: _vecval(std::move(vecval)), _caso(caso) {
		// Rejilla de evaluacion: los 100 puntos igualmente espaciados de [0,1]
		// que pide el paper (§5). En Python seria
		// `self._xrange = x_rango(start, stop, 100)`.
		_xrange = x_rango(start,stop,100);
		_repre = parse_tree(_vecval);
		_xrange[0] = _xinitials;
		// Delta x de la suma de Riemann que aproxima integral(f^2 dx) en la
		// ec. (4) del paper (seccion 5). x_rango siempre devuelve 100 puntos,
		// asi que el divisor (npoints-1) nunca es cero.
		_dx = (stop - start) / static_cast<long double>(_xrange.size() - 1);
	}
	// represant() es const porque no modifica el estado observable del objeto.
	// En Python sería @property def represant(self): return self._repre.
	string represant() const{
		return _repre;
	}
	// vecvalue() es const porque solo devuelve una copia del vector interno.
	// En Python sería @property def vecvalue(self): return self._vecval.copy().
	vector<ldouble> vecvalue() const{
		return _vecval;
	}
	// fitness() es const con caché mutable: el método es de solo lectura desde
	// fuera, pero internamente puede actualizar _fresult y _cache_valido.
	// En Python sería @cached_property: método const con estado interno cacheado.
	long double fitness() const{	
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
	// La expresion debe depender de x: basta cualquier hoja x-dependiente (la
	// variable x o las hojas polinomiales x^2/x^3 del issue #63), no solo el
	// token 11. En Python seria
	// `if not any(is_x_dependent(int(t)) for t in self._vecval):`.
	bool depende_de_x = std::any_of(_vecval.begin(), _vecval.end(),
	                                [](ldouble t){ return is_x_dependent(static_cast<int>(t)); });
	if (!depende_de_x){
		// Una expresion constante no es funcion de x: INFINITY sin tocar el
		// vector. Antes se escribian 11s al azar y luego se devolvia INFINITY
		// igual: la evaluacion mutaba al individuo y una segunda llamada
		// podia devolver otra cosa (B6 de REVIEW.md). Los productores
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

	// Hoist container size to avoid repeated casts in loop condition
	const int xrange_size = static_cast<int>(_xrange.size());
	for (int i = 0; i < xrange_size; i++){
		vector<ldouble> fitnessval(parsv(_vecval,_xrange[i]));
		//  cout << parse_tree(_vecval) << endl;
			
		//   cout << endl; cout << i << " " ;
		// cout <<" <<<<<"  << endl;
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

		// Residuo de la ODE en el punto de la rejilla, elevado al cuadrado,
		// segun el caso (ver _caso). En Python seria `res += r**2`; la ec. (4)
		// del paper usa cuadrados, no potencias cuartas.
		if (_caso == 1){
			// Caso 1 del paper: x*y'' + y' - cos(x) = 0. Se evita
			// dividir por x=0, que es el primer punto de la rejilla.
			ldouble x = _xrange[i][0];
			res +=pow(x*fitnessval[2] + fitnessval[1] - cos(x),2) ;
		} else if (_caso == 2){
			// Caso 2 (issue #61): y'' - y = 0, la ecuacion hiperbolica. En
			// Python seria `res += (y2 - y)**2`.
			res +=pow(fitnessval[2] - fitnessval[0],2) ;
		} else {
			// Caso 0 (juguete del prototipo): y'' + y = 0.
			res +=pow(fitnessval[2] + fitnessval[0],2) ;
		}
		}
			// Integral de la ec. (4): los residuos al cuadrado suman una suma
			// de Riemann y el factor Delta x la convierte en la aproximacion
			// de integral(f^2 dx). Las penalizaciones de abajo quedan FUERA
			// de la integral. En Python seria `res *= self._dx`.
			res *= _dx;
			// Condiciones iniciales del caso, como terminos lambda*h^2 de la
			// ec. (4): Caso 0 y(0)=1, y'(0)=1 (solucion esperada
			// cos(x)+sin(x)); Caso 1 del paper y(0)=0, y'(0)=1; Caso 2
			// y(0)=1, y'(0)=0 (solucion cosh(x)). El paper fija
			// lambda1 = lambda2 = 10 (seccion 5) y h al cuadrado; antes se
			// usaban pesos 300/700 y potencias cuartas.
			ldouble y0_objetivo  = (_caso == 1) ? 0.0L : 1.0L;
			ldouble dy0_objetivo = (_caso == 2) ? 0.0L : 1.0L;
			res +=10*pow((a_start[0] - y0_objetivo),2) + 10*pow((a_start[1] - dy0_objetivo),2);
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

// ---------------------------------------------------------------------------
// Configuracion de la busqueda (issue #28).
//
// Los valores por defecto son los del Caso 1 del paper (REVIEW.md C5-C7):
// 100 individuos, dimension 20 y 5000 generaciones. Cada uno se puede
// sobreescribir con una variable de entorno, igual que ODE_SEED (issue #4):
// ODE_POP, ODE_DIM, ODE_GEN y ODE_WARM_START. Para un desarrollador Python:
// ConfigBusqueda seria un `@dataclass` con defaults y resolver_config su
// `from_env()`, funcion pura para poder probarla sin tocar el entorno.
// ---------------------------------------------------------------------------

// Valores por defecto (Caso 1 del paper, REVIEW.md C5-C7).
constexpr int POBLACION_DEFECTO = 100;
constexpr int DIMENSION_DEFECTO = 20;
constexpr int GENERACIONES_DEFECTO = 5000;

// Minimos aceptados. DE/rand/1/bin necesita tres individuos base distintos
// del objetivo (indices_distintos), asi que por debajo de 4 la busqueda
// degenera; 1 es la dimension minima de un arbol y 1 el tope minimo de
// generaciones.
constexpr int POBLACION_MINIMA = 4;
constexpr int DIMENSION_MINIMA = 1;
constexpr int GENERACIONES_MINIMAS = 1;

// Longitud de las cinco semillas warm start escritas a mano.
constexpr int DIMENSION_SEMILLAS = 20;

// ConfigBusqueda: los parametros de la corrida en una estructura en vez de
// argumentos sueltos (regla I.23 de docs/cpp-standards.md: firmas cortas).
// Los `= valor` son los defaults, como en un dataclass de Python.
struct ConfigBusqueda {
    int poblacion = POBLACION_DEFECTO;
    int dimension = DIMENSION_DEFECTO;
    int max_generaciones = GENERACIONES_DEFECTO;
    bool warm_start = false;
};

// config_entera: lee una variable de entorno ya obtenida con getenv() (en
// Python seria el string de os.environ.get(nombre), o None) y devuelve su
// valor entero solo si representa un numero >= `minimo`; si falta, no es
// numerica o queda fuera de rango, devuelve el respaldo. Es la regla de
// ODE_SEED: una variable invalida se ignora, no rompe la corrida.
int config_entera(const char* valor_env, int respaldo, int minimo) {
    if (valor_env == nullptr) return respaldo;

    // strtol interpreta el texto como entero en base 10 (como int() de
    // Python) y deja en `fin` la posicion del primer caracter no consumido.
    char* fin = nullptr;
    long valor = strtol(valor_env, &fin, 10);

    // Se exige que TODO el texto sea el numero ("17abc" no vale): es el
    // `except ValueError` de Python. La cota superior evita que un valor
    // enorme desborde al convertirlo a int.
    if (fin == valor_env || *fin != '\0') return respaldo;
    if (valor < static_cast<long>(minimo) || valor > static_cast<long>(INT_MAX)) return respaldo;

    return static_cast<int>(valor);
}

// config_bandera: lee una bandera de entorno: "1" activa, "0" desactiva y
// cualquier otro valor (o la ausencia) devuelve el respaldo. En Python seria
// `{"1": True, "0": False}.get(valor, respaldo)`.
bool config_bandera(const char* valor_env, bool respaldo) {
    if (valor_env == nullptr) return respaldo;
    if (strcmp(valor_env, "1") == 0) return true;
    if (strcmp(valor_env, "0") == 0) return false;
    return respaldo;
}

// resolver_config: arma la configuracion efectiva de la corrida a partir de
// los valores crudos de ODE_POP/ODE_DIM/ODE_GEN/ODE_WARM_START. main() lee el
// entorno con getenv() y se los pasa (por eso es una funcion pura y testeable,
// igual que resolver_semilla).
ConfigBusqueda resolver_config(const char* pop_env, const char* dim_env,
                               const char* gen_env, const char* warm_env) {
    ConfigBusqueda config;
    config.poblacion = config_entera(pop_env, POBLACION_DEFECTO, POBLACION_MINIMA);
    config.dimension = config_entera(dim_env, DIMENSION_DEFECTO, DIMENSION_MINIMA);
    config.max_generaciones = config_entera(gen_env, GENERACIONES_DEFECTO, GENERACIONES_MINIMAS);

    // Las cinco semillas warm start miden DIMENSION_SEMILLAS tokens: con otra
    // dimension no valen como individuos (Mutation()/Recombination() emparejan
    // genes por posicion, B5 de REVIEW.md), asi que se ignora la peticion.
    config.warm_start = config_bandera(warm_env, false) &&
                        config.dimension == DIMENSION_SEMILLAS;

    return config;
}

// vector_population: un individuo aleatorio de exactamente `tam` tokens.
// La longitud la manda la configuracion (dimension del paper: 20) porque
// Mutation()/Recombination() emparejan genes por posicion: longitudes
// dispares leian fuera de rango (B5 de REVIEW.md). En Python seria
// `[random.randint(TOKEN_MIN, TOKEN_MAX) for _ in range(tam)]`; en C++
// `random_token()` concentra esa regla en token.h.
vector<long double> vector_population(int tam){
	// Precondicion: tam >= 1 para evitar division por cero en rand() % tam.
	// Con tam == 0, el bucle de inyeccion de TOKEN_X divide por cero. En Python
	// seria `assert tam >= 1, "individual requires at least 1 token"`.
	assert(tam >= 1);

	vector<ldouble> ans;

	// random_token() aplica el rango definido en token.h; asi este consumidor no
	// duplica el tamano ni los extremos del alfabeto.
	for (int i = 0; i < tam; i++){
		ans.push_back(random_token());
	}

	// La fitness exige alguna hoja que dependa de x (la variable o x^2/x^3,
	// issue #63): si el sorteo no saco el token x, se escribe en dos
	// posiciones al azar de todo el vector (forzarlo sigue siendo una
	// reparacion valida). Antes el indice se limitaba a [0, tam-4] (resto del
	// tamano de poblacion de la version vieja) y las ultimas posiciones nunca
	// se reparaban.
	if (std::find(ans.begin(), ans.end(), TOKEN_X) == ans.end()){
		for (int i = 0; i < 2; i++){
			ans[rand() % tam] = TOKEN_X;
		}
	}
	return ans;
}

// semillas_warm_start: las cinco expresiones escritas a mano del prototipo.
// Antes se sembraban SIEMPRE junto a los individuos aleatorios; ahora son un
// warm start opt-in (ODE_WARM_START=1) para que la inicializacion por defecto
// sea la aleatoria uniforme de la ecuacion (6) del paper (REVIEW D6). Devuelve
// la lista por referencia constante para no copiarla en cada uso (en C++ `&`
// evita la copia; en Python la lista ya se pasaria sin copiar).
const vector<vector<ldouble>>& semillas_warm_start(){
	// `static const` se construye una sola vez, como una constante de modulo
	// en Python, y vive mientras dure el proceso.
	// Los nombres de token.h son constantes de modulo legibles, equivalentes a
	// constantes enteras de Python, y evitan repetir los codigos del alfabeto.
	static const vector<vector<ldouble>> semillas = {
		{TOKEN_ADD, TOKEN_SIN, TOKEN_SQRT, TOKEN_X, TOKEN_COS, TOKEN_COS,
		 TOKEN_X, TOKEN_COS, TOKEN_COS, TOKEN_COS, TOKEN_SIN, TOKEN_COS,
		 TOKEN_COS, TOKEN_COS, TOKEN_COS, TOKEN_COS, TOKEN_COS, TOKEN_COS,
		 TOKEN_COS, TOKEN_X},                                             // uno
		{TOKEN_ADD, TOKEN_COS, TOKEN_SIN, TOKEN_SQRT, TOKEN_X, TOKEN_COS,
		 TOKEN_DIV, TOKEN_CONST1, TOKEN_CONST5, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1},          // dos
		{TOKEN_ADD, TOKEN_SIN, TOKEN_X, TOKEN_SIN, TOKEN_SIN, TOKEN_COS,
		 TOKEN_COS, TOKEN_COS, TOKEN_X, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1},          // tres
		{TOKEN_ADD, TOKEN_COS, TOKEN_X, TOKEN_SIN, TOKEN_SIN, TOKEN_X,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1},          // cuatro
		{TOKEN_ADD, TOKEN_SIN, TOKEN_SIN, TOKEN_SIN, TOKEN_X, TOKEN_SIN,
		 TOKEN_X, TOKEN_X, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1,
		 TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1, TOKEN_CONST1}           // c5
	};
	return semillas;
}

// createPopulation: llena `Population` (que entra vacia) con exactamente
// config.poblacion individuos de config.dimension tokens. Es la inicializacion
// del paper: aleatoria uniforme (ecuacion (6)); las semillas del prototipo
// solo entran con warm start activo. En Python seria
// `poblacion += [FunbObj(vector_population(dim), 0, 1, caso) for _ in range(n)]`.
void createPopulation(vector<FunbObj>& Population, const ConfigBusqueda& config, int caso){

	// Precondiciones del inicializador de poblacion:
	// 1. La poblacion entra vacia y se llena con push_back (en Python seria
	//    `assert not poblacion`). Activa porque no se compila con -DNDEBUG.
	// 2. poblacion >= 1: una solicitud de longitud cero no tiene sentido en el
	//    bucle DE (necesitamos al menos un individuo para evaluar fitness). En
	//    Python seria `assert config.poblacion >= 1, "population must have at
	//    least 1 individual"`.
	assert(Population.empty());
	assert(config.poblacion >= 1);

	// Warm start: se copian las cinco semillas solo si tienen la dimension
	// pedida. El chequeo esta tambien en resolver_config (que apaga la
	// bandera); aqui se compara contra la longitud real de los datos para que
	// un cambio de las semillas no reintroduzca el desajuste de B5.
	const vector<vector<ldouble>>& semillas = semillas_warm_start();
	const int tam_semilla = static_cast<int>(semillas[0].size());
	const bool usar_semillas = config.warm_start &&
	                           config.dimension == tam_semilla;
	if (usar_semillas){
		for (size_t i = 0; i < semillas.size() && static_cast<int>(Population.size()) < config.poblacion; i++){
			// `caso` se propaga a cada FunbObj: los tokens no cambian entre
			// casos, solo la ODE que mide fitness(). En Python seria
			// `FunbObj(semilla, 0, 1, caso)`.
			Population.push_back(FunbObj(semillas[i], 0, 1, caso));
		}
	}

	// El resto (o la poblacion entera sin warm start) es aleatorio: tantos
	// individuos nuevos como falten para llegar a config.poblacion, sin el
	// off-by-one de REVIEW C5.
	while (static_cast<int>(Population.size()) < config.poblacion){
		Population.push_back(FunbObj(vector_population(config.dimension), 0, 1, caso));
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
// alfabeto. Redondea al entero mas cercano y recorta entre TOKEN_MIN y
// TOKENS_ALFABETO, cuyo valor superior coincide con TOKEN_MAX porque el
// alfabeto empieza en 1.
//
// Detalle C++ sin equivalente directo en Python: se devuelve `int` y no
// `long double` porque un token es un codigo, no una cantidad; el redondeo se
// hace con floor(v + 0.5) (equivalente a round() pero sin la ambiguedad de
// round() con los .5 negativos) y el recorte con dos `if` en vez de min/max
// para que quede explicito. En Python seria:
//     def token_valido(v): return min(TOKENS_ALFABETO, max(TOKEN_MIN, floor(v + 0.5)))
int token_valido(ldouble v){
	ldouble redondeado = floor(v + 0.5L);
	if (redondeado < TOKEN_MIN) return TOKEN_MIN;
	if (redondeado > TOKENS_ALFABETO) return TOKENS_ALFABETO;
	return static_cast<int>(redondeado);
}

// indices_distintos: sortea tres indices de poblacion distintos entre si y
// distintos de `excluir` (el individuo objetivo). Son los x_r1, x_r2, x_r3 de
// la ecuacion (7): el paper exige individuos distintos para que la diferencia
// (x_r2 - x_r3) no sea siempre cero.
//
// Los parametros de salida (`int& r1, ...`) se reemplazaron por un valor
// devuelto (F.20/F.21): en Python seria `return r1, r2, r3`, y el llamador lo
// desempaqueta con un structured binding, `r1, r2, r3 = indices_distintos(...)`.
// El sorteo se hace por rechazo: se tira rand() hasta que no toque un indice
// repetido. Con poblaciones de ~275 individuos el bucle casi nunca repite.
array<int, 3> indices_distintos(int tam, int excluir){
	// Con menos de 4 individuos no existen 3 indices distintos del objetivo;
	// se devuelven los que haya para no quedarse en un bucle infinito (solo
	// puede pasar si alguien achica la poblacion a mano).
	if (tam < 4) {
		return {0, (tam > 1) ? 1 : 0, (tam > 2) ? 2 : 0};
	}
	// El orden de los sorteos (r1, r2, r3) es parte del contrato de neutralidad
	// del flujo de rand(): no reordenar ni cambiar el numero de tiradas.
	array<int, 3> indices{};
	do { indices[0] = rand() % tam; } while (indices[0] == excluir);
	do { indices[1] = rand() % tam; } while (indices[1] == excluir || indices[1] == indices[0]);
	do { indices[2] = rand() % tam; } while (indices[2] == excluir || indices[2] == indices[0] || indices[2] == indices[1]);
	return indices;
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
// Nota C++ para un desarrollador Python: los tres parametros entran por
// referencia constante (const&: sin copiar el vector, igual que pasar una
// lista a una funcion en Python) y la funcion devuelve un vector nuevo; el
// const& declara ademas que los padres son de solo lectura (nadie los muta).
vector<ldouble> Mutation(const vector<ldouble>& x, const vector<ldouble>& y, const vector<ldouble>& z){

	// Se recorre hasta el menor de los tres tamanos: si dos vectores midieran
	// distinto (p. ej. un seed mal escrito, B5 de REVIEW.md), leer y[i]/z[i]
	// de mas seria una lectura fuera de rango. En Python seria
	// `n = min(len(x), len(y), len(z))` (aqui `min` es std::min, de <algorithm>).
	const int n = static_cast<int>(min(x.size(), min(y.size(), z.size())));

	// Se parte de una copia de x: asi, si x es mas largo que y/z, los tokens
	// sobrantes sobreviven intactos en vez de quedar indefinidos.
	vector<ldouble> res(x);

	// Dither: un rand() por llamada. rand()/RAND_MAX da un real en [0,1]
	// (rand() devuelve enteros en [0,RAND_MAX]); en Python seria
	// `F = random.random()`.
	ldouble F = static_cast<ldouble>(rand()) / static_cast<ldouble>(RAND_MAX);

	for(int i = 0 ; i < n; i++ ){
		res[i] = token_valido(mutacion_rand1(x[i], y[i], z[i], F));
	}

	// Reparacion heredada: la fitness exige alguna hoja que dependa de x
	// (variable o polinomial, issue #63), asi que si la mutacion borro el
	// token x se fuerza uno en una posicion al azar (un solo token; antes se
	// intentaban 3 posiciones con un rango dudoso). Es una regla propia de
	// este prototipo, no del paper.
	if (res.size() > 0 && std::find(res.begin(), res.end(),
	                                static_cast<ldouble>(TOKEN_X)) == res.end()){
		res[rand() % res.size()] = TOKEN_X;
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

	// Se recorre hasta el menor de los dos tamanos: leer mutante[i] de mas seria
	// lectura fuera de rango. En Python seria
	// `n = min(len(objetivo), len(mutante))`.
	const int n = static_cast<int>(min(objetivo.size(), mutante.size()));

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
		ldouble r = static_cast<ldouble>(rand()) / static_cast<ldouble>(RAND_MAX);
		if (i == j_rand){
			trial[i] = mutante[i];
		} else {
			trial[i] = gen_binomial(objetivo[i], mutante[i], r, Cr);
		}
	}

	// Reparacion heredada (igual que en Mutation()): la fitness exige alguna
	// hoja que dependa de x (variable o polinomial, issue #63), asi que si el
	// cruce borro el token x se fuerza uno en una posicion al azar. Es una
	// regla propia de este prototipo, no del paper.
	if (std::find(trial.begin(), trial.end(),
	                                  static_cast<ldouble>(TOKEN_X)) == trial.end()){
		trial[rand() % trial.size()] = TOKEN_X;
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
		os << static_cast<long long>(v[i]);  // los tokens son enteros TOKEN_MIN..TOKEN_MAX
	}
	os << "]";
	return os.str();
}

// segundos_cpu: CPU transcurrida desde g_log_t0 (time.process_time() - t0 en
// Python). clock() mide tiempo de CPU del proceso, no tiempo de pared.
ldouble segundos_cpu(){
	return static_cast<ldouble>(clock() - g_log_t0) / CLOCKS_PER_SEC;
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
	long semilla = resolver_semilla(getenv("ODE_SEED"), static_cast<long>(time(nullptr)));
	srand(static_cast<unsigned>(semilla));

	// Caso del problema, controlado por ODE_CASE (mismo patron que ODE_SEED):
	//   0 (o ausente) -> juguete historico y'' + y = 0,  y(0)=y'(0)=1
	//   1            -> Caso 1 del paper: x*y'' + y' - cos(x) = 0 (ec. (18)),
	//                   y(0)=0, y'(0)=1, solucion exacta Si(x)
	//   2            -> caso hiperbolico (issue #61): y'' - y = 0,
	//                   y(0)=1, y'(0)=0, solucion exacta cosh(x)
	// En Python seria `caso = int(os.environ.get("ODE_CASE", "0"))` (aqui
	// atoi() convierte el string del entorno a entero, como int()); un valor
	// distinto de 1 o 2 cae de vuelta al caso 0, igual que el resto de ODE_*.
	int caso = 0;
	const char* caso_env = getenv("ODE_CASE");
	if (caso_env != nullptr){
		const int caso_pedido = atoi(caso_env);
		if (caso_pedido == 1 || caso_pedido == 2){
			caso = caso_pedido;
		}
	}

	// Configuracion de la busqueda (issue #28): poblacion, dimension y tope de
	// generaciones del Caso 1 del paper (REVIEW C5-C7), cada uno sobreescribible
	// con ODE_POP/ODE_DIM/ODE_GEN; el warm start (las cinco semillas del
	// prototipo) se activa con ODE_WARM_START=1 y por defecto la poblacion es
	// aleatoria uniforme, como pide la ecuacion (6) del paper. En Python seria
	// `config = ConfigBusqueda.from_env()` leyendo os.environ.
	ConfigBusqueda config = resolver_config(getenv("ODE_POP"), getenv("ODE_DIM"),
	                                        getenv("ODE_GEN"), getenv("ODE_WARM_START"));

	// Una sola poblacion (issue #6): antes habia dos (Population1/Population2)
	// que se cruzaban entre si; DE/rand/1/bin (paper) evoluciona una unica
	// poblacion generacion a generacion.
	vector<FunbObj> Population;
	createPopulation(Population, config, caso);

	// Cr: probabilidad de heredar el gen del mutante en el cruce binomial
	// (eq. (8)). Antes era `int Cr = 85`, un umbral comparado contra el tamano
	// de poblacion; el paper (§5, REVIEW D9) usa 0.2.
	constexpr ldouble Cr = 0.2;
	constexpr long double eps = 0.00001;
	long double Bestfit = INFINITY;
	vector<ldouble> Bestvect;
	int gen = 0;
	bool flag = true;
	const int max_gen = config.max_generaciones;  // tope del bucle (ODE_GEN)
	string estado = "max_generations";  // pasa a "converged" si llega a eps

	// Cabecera de la corrida (reemplaza el banner de ASCII art): una sola
	// linea con toda la configuracion en clave=valor. `population=` informa el
	// tamano real de la poblacion ya creada; `warm_start=` es el valor efectivo
	// (con ODE_DIM distinto de 20 la bandera se ignora, ver resolver_config).
	log_info() << "de_run_start | seed=" << semilla
	           << " case=" << caso
	           << " population=" << Population.size()
	           << " dimension=" << config.dimension
	           << " warm_start=" << (config.warm_start ? 1 : 0)
	           << " cr=" << fmt_fijo(Cr, 2)
	           << " fitness_target=" << fmt_ciencia(eps)
	           << " max_generations=" << max_gen << '\n';

	// Descripcion legible del caso: el dato maquina es el campo `case=` de
	// arriba; este renglon es la explicacion y va como texto libre (sin
	// clave=valor) para no confundir al parser del script A/B.
	log_info() << "ode_case | " << caso << ": ";
	if (caso == 1){
		cout << "paper, ec. (18): x*y'' + y' - cos(x) = 0, y(0)=0, y'(0)=1";
	} else if (caso == 2){
		cout << "hiperbolico (issue #61): y'' - y = 0, y(0)=1, y'(0)=0, solucion cosh(x)";
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
		const int tam_poblacion = static_cast<int>(Population.size());
		for (int j = 0; j < tam_poblacion; j++){

			// Structured binding: desempaqueta el array devuelto por el
			// sorteo (Python: `r1, r2, r3 = indices_distintos(...)`).
			auto [r1, r2, r3] = indices_distintos(tam_poblacion, j);
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
		for (int j = 1; j < tam_poblacion; j++){
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

	// Firma del autor (issue #44), conservada del prototipo original pero ahora
	// como un evento estructurado mas: nivel INFO y flush explicito, para que
	// el consumidor del log no vea lineas sueltas. `author=` es un campo nuevo
	// que el awk de scripts/ab_experiment.sh no parsea (solo mira seed, gen,
	// generations, best, expr, vector, status y elapsed_cpu_s). En Python
	// seria logging.info("signature | author=Lmex").
	log_info() << "signature | author=Lmex" << '\n';
	cout.flush();

	return 0;
}
#endif // ODE_NO_MAIN
