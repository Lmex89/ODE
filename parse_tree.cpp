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


vector<ldouble> Mutation(vector<ldouble>x, vector<ldouble> y,vector<ldouble>z){

	vector<ldouble> res;
	for(int i = 0 ; i < (int)x.size(); i++ ){
		res.push_back( abs(x[i]- y[i]));
		if (res[i] <= 0 || res[i]> 21){
			int resta = abs(x[i]- y[i]);
			res[i] = resta % 21 + 1;
		}
		if( (res[i] + rand()*z[i]) > 21  || res[i] < 0 ){
			int suma = res[i] + z[i];
			res[i] = suma % 21 +1;
		}else{
			res[i] =res[i]+z[i];
		}
	}

	std::vector<ldouble>::iterator it = std::find(res.begin(),res.end(),11.0);
	 if (it == res.end()){
		for (int i = 0 ; i < 3 ; i++){
			ldouble rand_num = rand() % (int) x.size();
			if ((rand_num <= res.size() - 4) && (rand_num > 0)){
				res[rand_num] = 11;
			}		
		}

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


int main() {

	unsigned t0, t1;
 	t0=clock();
 	srand (time(NULL));
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


		vector<ldouble> temporal1(Mutation(Population2[j].vecvalue(),Population2[j+2].vecvalue(),Population2[j+3].vecvalue()));
		vector<ldouble> temporal2(Mutation(Population1[j].vecvalue(),Population1[j+2].vecvalue(),Population1[j+3].vecvalue()));
		
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
