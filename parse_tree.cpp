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

void Recombination(vector<ldouble>& x,vector<ldouble>& y){
	
	int tam = (x.size() / 2) + 2;
	int num = rand() % tam;

	for (int i = 0 ; i < num; i++){

		int i_random = rand() % tam ;
		ldouble xi = x[i_random];
		ldouble yi = y[i_random];
	/* cout << xi << " xxx" << yi << endl; */
		swap(xi,yi);
	/* cout << xi << "uuuuu " << yi << endl; */
		x[i_random] = xi;
		y[i_random] = yi;
	}
	std::vector<ldouble>::iterator it = std::find(x.begin(),x.end(),11.0);
	 if (it == x.end()){
		for (int i = 0 ; i < 3 ; i++){
			ldouble rand_num = rand() % (int) x.size();
			if (rand_num == 0){
				x[rand_num+1] = 11;
			}
			x[rand_num] = 11;
			
		}

	 }
	std::vector<ldouble>::iterator it1 = std::find(y.begin(),y.end(),11.0);
	 if (it1 == y.end()){
		for (int j = 0 ; j< 3; j++){
		ldouble rand_num = rand() % (int) y.size();
		if (rand_num == 0){
				y[rand_num+1] = 11;
			}
			y[rand_num] = 11;
		
		}
	 }

}

bool compare(FunbObj& x1, FunbObj& y1){
	return (abs(x1.fitness()) < abs(y1.fitness()));
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
 	int tam =275;
 	vector<FunbObj> Population1 ,Population2;
	createPopulation(Population1,tam);
	vector<ldouble> temp1(Population1[0].vecvalue() );
	vector<ldouble> x_initials = {1.0,1.0,0.0};

	// int start = 0;
	// ldouble stop = 1.0;
	// ldouble step = 0.1;
	// int final  = (stop /step) +2;
	// ldouble sum = 0 ;
	// for(int i  = (int) start; i < final; i++){
	// 	vector<long double> x_temp = {i*step,1.0,0.0};
	// 	if( !isnan(parsv(temp1,x_temp)[2])){
	// 		sum += parsv(temp1,x_temp)[2]  ;
	// 	}
		
	// 	cout << parsv(temp1,x_temp)[0] << " "<< i*step <<endl;
	//  	cout << parsv(temp1,x_temp)[1] << endl;
	//  	cout << parsv(temp1,x_temp)[2] << endl;
	// 	cout << "==============================" << endl;
		
	// }
	// cout << "*=====================*" << endl;
	// cout << sum << endl;

	createPopulation(Population2,tam);

	int Cr = 85;
	long double eps =0.00001;
	long double Bestfit = INFINITY;
	int gen = 0;
	bool flag = true;

	cout <<"_______:::::_____________:::::____________::::_____" << endl;
	cout <<"_______:::::_____________:::::____________::::_____" << endl;
	cout <<"                                                   " << endl;

	vector<ldouble> Bestvect1;
	vector<ldouble> Bestvect2;

	while(flag){

	for (int j = 1 ; j < tam -4   ; j++){


		// Tres individuos base *distintos y al azar* para cada mutacion. Antes
		// los indices eran los offsets fijos j, j+2, j+3; el paper (eq. (7))
		// pide indices aleatorios distintos. Se excluye `j` para que el
		// individuo objetivo no sea su propia base.
		int r1, r2, r3;
		indices_distintos((int) Population2.size(), j, r1, r2, r3);
		int s1, s2, s3;
		indices_distintos((int) Population1.size(), j, s1, s2, s3);

		vector<ldouble> temporal1(Mutation(Population2[r1].vecvalue(),Population2[r2].vecvalue(),Population2[r3].vecvalue()));
		vector<ldouble> temporal2(Mutation(Population1[s1].vecvalue(),Population1[s2].vecvalue(),Population1[s3].vecvalue()));
		
		int ix = rand() % Population1.size();
		int iy = rand() % Population1.size();
		if (ix !=0 && iy != 0){
			Population2[ix] = FunbObj(temporal1,0,1);
			Population1[iy] = FunbObj(temporal2,0,1);
			
			}
		
		
		// } else if (Cr_random < 20 ){
		// 	//cout << Cr_random << endl;
		// 	//cout << "dentro del else " << endl;
		// 	//cout << "=================" << endl;
		// 	sort(Population1.begin(),Population1.end(),compare);

		// 	vector<ldouble> temp1(Population1[1].vecvalue() );
		// 	vector<ldouble> temp2(Population2[1].vecvalue() );
		// 	Recombination(temp1,temp2);
		// 		Population2[j] = FunbObj(temp1,0,1);
		// 		Population1[j+1] = FunbObj(temp2,0,1);

		  	}

	//sort(Population.begin(),Population.end(),compare);
	if (gen < 10){
		sort(Population1.begin(),Population1.end(),compare);
		sort(Population2.begin(),Population2.end(),compare);
		Bestvect1 = (Population1[0].vecvalue());
		Bestvect2 = (Population2[0].vecvalue());
	}else{
		

		
		sort(Population1.begin(),Population1.end(),compare);
		sort(Population2.begin(),Population2.end(),compare);
		 FunbObj temporal1(Bestvect1,0,1);
		 FunbObj temporal2(Bestvect2,0,1);
		
		if (Population1[0].fitness()> temporal1.fitness() ){
			Population1[0] =FunbObj(Bestvect1,0,1) ;
			Population2[1] =FunbObj(Bestvect1,0,1) ;
			sort(Population2.begin(),Population2.end(),compare);
		}
		else if (Population2[0].fitness()> temporal2.fitness() ){
			Population2[0] =FunbObj(Bestvect2,0,1) ;
			Population1[1] =FunbObj(Bestvect2,0,1) ;
			sort(Population1.begin(),Population1.end(),compare);
		}
	}
	
	/* imprime(Population[0].vecvalue());
		cout <<"()============antes===========()" << endl;
			vector<ldouble> temp1(Population[0].vecvalue() );
			vector<ldouble> temp2(Population[1].vecvalue() );
			Recombination(temp1,temp2);

			Population[0] = FunbObj(temp1,0,1);
			Population[1] = FunbObj(temp2,0,1);
			cout <<"()=============despues==========()" << endl;
	imprime(Population[0].vecvalue());
	imprime(Population[1].vecvalue());
	cout << endl;
	cout <<"()=============despues==========()despues" << endl;
	sort(Population.begin(),Population.end(),compare); */
	ldouble BestCurrent1 = Population1[1].fitness();
	ldouble BestCurrent2 = Population2[1].fitness();
	
	cout << "BestCurrent1 :" << BestCurrent1 << "  y(x)= " <<  Population1[1].represant() <<"     " << Bestfit<<endl;
	cout << "BestCurrent2 :" << BestCurrent2 << "  y(x)= " <<  Population2[1].represant() <<"      "<< parse_tree(Bestvect1) <<endl;

	if (BestCurrent1 < Bestfit  ){

	/*	cout << "entre deonde de Recombination antes" << endl; */
		/* imprime(Population[0].vecvalue());
		cout << endl; */
		Bestvect1 = (Population1[1].vecvalue());
		
			/* cout << "entre deonde de Recombination despues" << endl;
		imprime(Population[0].vecvalue()); */
		cout <<"()======================Bestfit1=========================()" << endl;
		Bestfit = BestCurrent1;
		vector<ldouble> temp1(Population1[0].vecvalue() );
		vector<ldouble> temp2(Population2[1].vecvalue() );
		Recombination(temp1,temp2);
		Population1[1] = FunbObj(temp1,0,1);
		cout << Population1[1].fitness() << endl;
		cout << "f(x)= " << Population1[1].represant() << endl;
		imprime(Population1[1].vecvalue()); cout << endl;
		cout <<"()======================Bestfit1=========================()" << endl;
		int ix = rand() % Population1.size();
		int iy = rand() % Population1.size();
		if (ix !=0 && iy != 0){
				Population1[ix] = FunbObj(temp1,0,1);
				Population1[iy] = FunbObj(temp2,0,1);
			}
	}

	if (BestCurrent2 < Bestfit ){
		
		Bestvect2 = (Population2[1].vecvalue());
		cout <<"()======================Bestfit2=========================()" << endl;
		Bestfit = BestCurrent2;
		


		cout << Population2[1].fitness() << endl;
		cout << "f(x)= " << Population2[1].represant() << endl;
		imprime(Population2[1].vecvalue()); cout << endl;
		cout <<"()======================Bestfit2=========================()" << endl;

		vector<ldouble> temp1(Population2[0].vecvalue() );
		vector<ldouble> temp2(Population1[1].vecvalue() );
		Recombination(temp1,temp2);
		Population2[1] = FunbObj(temp1,0,1);
		int ix = rand() % Population2.size();
		int iy = rand() % Population2.size();
			if (ix !=0 && iy != 0){
				Population2[ix] = FunbObj(temp1,0,1);
				Population2[iy] = FunbObj(temp2,0,1);
			}
		
		
	}


	int Cr_random = rand() % Population2.size() ;
		//cout << "Cr_random" << Cr_random << endl;
		if (Cr >= Cr_random ){
			//cout << "Cr_random" << Cr_random << endl;

			vector<ldouble> temp1(Bestvect1 );
			vector<ldouble> temp2(Population2[1].vecvalue() );
			Recombination(temp1,temp2);
			for (int i = 0 ; i <= 7 ; i++){
				int ix = rand() % Population2.size() ;
				int iy = rand() % Population1.size()  ;
				
				if (ix !=0 &&  iy != 0  && ix !=1 &&  iy != 1){
					Population2[ix] = FunbObj(temp1,0,1);
					Population1[iy] = FunbObj(temp2,0,1);
			}

			}
		
		}


	/* else{
		for (int j = 1 ; j < tam -2 ; j++){
			//vector<ldouble> temporal(Mutation(Population[j].vecvalue(),Population[j+1].vecvalue(),Population[j+2].vecvalue()));
		//imprime(temporal);
			//Population[j] = FunbObj(temporal,0,1);
			int Cr_random = rand() % 100 +1;

			if (Cr >= Cr_random){

				vector<ldouble> temp1(Population[0].vecvalue() );
				vector<ldouble> temp2(Population[j+2].vecvalue() );
				Recombination_new(temp1,temp2);
				Population[0] = FunbObj(temp1,0,1);
				Population[j+2] = FunbObj(temp2,0,1);
			}


		}

	} */


	if ( Bestfit < eps){
		cout <<"==============================================" << endl;
		imprime(Bestvect1) ;
		cout  << endl;
		cout << "f(x)= " << parse_tree(Bestvect1) << endl;
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
