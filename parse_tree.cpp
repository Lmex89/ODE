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
#include "dualfunctions.h"
#include "semilla.h"

using namespace std;
using namespace std::complex_literals;
typedef long long int llint;
typedef  long double ldouble;
typedef string  str;

vector<vector<long double>> x_rango(long double start,long double stop,long double step){

	vector<vector<long double>> ans;
	int final = stop /step;

	for(int i  = (int) start; i < final+2; i++){
		vector<long double> temp = {i*step,1.0,0.0};
		ans.push_back(temp);
	}
	return ans;
}

class FunbObj{

	private:
	vector<ldouble> _vecval;
	string _repre;
	ldouble _fresult;
	vector<vector<ldouble>> _xrange;
	vector<long double> _xinitials = {0.0,1.0,0.0};

	public:
	FunbObj(vector<ldouble> vecval,ldouble start,ldouble stop){
		_xrange = x_rango(start,stop,0.1);
		_repre = parse_tree(vecval);
		_xrange[0] = _xinitials;
		_vecval = vecval;
	}
	string represant(){
		return _repre;
	}
	vector<ldouble> vecvalue(){
		return _vecval;
	}
	long double fitness(){	
		ldouble res = 0 ;
		//vector<long double> xinitials = {0.0,1.0,0.0};
		string fun = parse_tree(_vecval);

		std::size_t poss = fun.find("()");
		if (poss !=std::string::npos){
			res = INFINITY;
			_fresult  = res;
			return _fresult ;
			}
	std::vector<ldouble>::iterator it = std::find(_vecval.begin(),_vecval.end(),11.0);
	if (it == _vecval.end()){
	for (int i = 0 ; i < 4 ; i++){
		ldouble rand_num = (rand() % _vecval.size())  ;
		if ((rand_num <= _vecval.size() - 5) && (rand_num >= 0)){
			_vecval[rand_num] = 11;
		}	
	 }
			res = INFINITY;
			_fresult  = res;
			return _fresult ;
 }				
	vector<ldouble> a_start = parsv(_vecval,_xrange[0]);
	vector<ldouble> b_end = parsv(_vecval,_xrange[_xrange.size()-1]);

	for (	int i = 0 ; i < (int)_xrange.size(); i++){
		vector<ldouble> fitnessval(parsv(_vecval,_xrange[i]));
		//  imprime(_vecval); cout << endl;
		//  cout << parse_tree(_vecval) << endl;
			
		//   cout << endl; cout << i << " " ;
		//  imprime(_xrange[i]) ; cout << "despues de _range" << endl;
		// imprime(fitnessval);
		// cout <<" <<<<<"  << endl;
		// imprime(a_start); 
		// cout  << " (b-a)" ; imprime(b_end) ;
		// cin.get();

		if ( 	!isnan(fitnessval[1]) || !isnan(fitnessval[0]) || !isnan(fitnessval[2] )  ){
				//  imprime(fitnessval) ; cout << endl;
				//  cout << "res :: " ;
					res +=pow(fitnessval[2] + fitnessval[0],4) ;
				//  cout << res<< endl;
			}
			else
{
			res +=1000000;
			}
		}
			res +=300*pow((a_start[0] - 1),4) + 700*pow((a_start[1] - 1),4);
		// cout << "imprimimos res " << endl;
		// cout << res << endl;
		// cin.get();
	if (isnan(res)){
		_fresult = INFINITY;
		return _fresult;
	}
	_fresult = res;
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


void createPopulation(vector<FunbObj>&Population,int tam){

	//vector<ldouble> uno =   {21,11,2,1,1,1,2,11,11,11,11,11,1,1,1,1,1,1,1,1};
	vector<ldouble> uno =   {18,13,17,11,14,14,11,14,14,14,13,14,14,14,14,14,14,14,14,11};
	vector<ldouble> dos =   {18,14,13,17,11,14,20,1,5,1,1,1,1,1,1,1,1,1,1,1,1};
	vector<ldouble> tres =  {18,13,11,13,13,14,14,14,11,1,1,1,1,1,1,1,1,1,1,1};
	vector<ldouble> cuatro ={18,14,11,13,13,11,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	vector<ldouble> c5 =    {18,13,13,13,11,13,11,11,1,1,1,1,1,1,1,1,1,1,1,1};

	FunbObj temp(uno,0,1);
	Population.push_back(temp);
	FunbObj temp1(dos,0,1);
	Population.push_back(temp1);
	FunbObj temp2(tres,0,1);
	Population.push_back(temp2);

	FunbObj temp3(cuatro,0,1);
	Population.push_back(temp3);

	FunbObj temp4(c5,0,1);
	Population.push_back(temp4);

	for (int  i = 0 ; i < tam - 4 ; i++){

		vector<ldouble> temporal(vector_population(20));
		FunbObj temp(temporal,0,1);
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

	// Se recorre hasta el menor de los tres tamanos: un individuo semilla
	// tiene 21 tokens y el resto 20 (B5 de REVIEW.md), y leer y[i]/z[i] de mas
	// era una lectura fuera de rango. En Python seria
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
int main() {

	unsigned t0, t1;
 	t0=clock();
 	// Semilla controlada por ODE_SEED (issue #4): con ODE_SEED=42 todas las
 	// corridas salen identicas (mismo fitness, mismo vector), que es lo que
 	// exige el experimento A/B. Sin la variable se usa el reloj, como antes.
 	// En Python seria: random.seed(os.environ.get("ODE_SEED")).
 	long semilla = resolver_semilla(getenv("ODE_SEED"), (long) time(NULL));
 	cout << "Seed: " << semilla << endl;
 	srand ((unsigned) semilla);

 	int tam = 275;
 	// Una sola poblacion (issue #6): antes habia dos (Population1/Population2)
 	// que se cruzaban entre si; DE/rand/1/bin (paper) evoluciona una unica
 	// poblacion generacion a generacion.
 	vector<FunbObj> Population;
 	createPopulation(Population, tam);

 	// Cr: probabilidad de heredar el gen del mutante en el cruce binomial
 	// (eq. (8)). Antes era `int Cr = 85`, un umbral comparado contra el tamano
 	// de poblacion; el paper (§5, REVIEW D9) usa 0.2.
 	ldouble Cr = 0.2;
 	long double eps = 0.00001;
 	long double Bestfit = INFINITY;
 	vector<ldouble> Bestvect;
 	int gen = 0;
 	bool flag = true;

 	cout <<"_______:::::_____________:::::____________::::_____" << endl;
 	cout <<"_______:::::_____________:::::____________::::_____" << endl;
 	cout <<"                                                   " << endl;

 	while(flag){

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
 			FunbObj candidato(trial, 0, 1);
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

 		cout << "BestCurrent :" << BestCurrent << "  y(x)= " << Population[mejor].represant() << "     " << Bestfit << endl;

 		if (BestCurrent < Bestfit){
 			Bestfit = BestCurrent;
 			Bestvect = Population[mejor].vecvalue();
 			cout <<"()======================Bestfit=========================()" << endl;
 			cout << "f(x)= " << parse_tree(Bestvect) << endl;
 			imprime(Bestvect); cout << endl;
 			cout <<"()======================Bestfit=========================()" << endl;
 		}

 		if ( Bestfit < eps){
 			cout <<"==============================================" << endl;
 			imprime(Bestvect) ;
 			cout  << endl;
 			cout << "f(x)= " << parse_tree(Bestvect) << endl;
 			cout <<"==============================================" << endl;

 			flag = false;
 		}

 		if(gen >= 100000){
 			flag = false;

 		}

 		gen++;
 		cout << "****-> : "<< gen << endl;

 	}

 	t1 = clock();
 	cout << "::::::::::::::::::" << endl;
 	double time = (double(t1-t0)/CLOCKS_PER_SEC);
 	cout << "Execution Time: " << time << endl;
 	cout << " Por Lmex " << endl;

 	return 0;
}
#endif // ODE_NO_MAIN
