#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>
#include <complex>

#include "dualfunctions.h"

using namespace std;
typedef long long int llint;
typedef  long double ldouble; 
typedef string  str;


int main() {
	
	unsigned t0, t1;
	t0=clock();

	vector<ldouble> num1 = {18,13,11,19,14,11,2};
	vector<ldouble> feval = {0.0,1.0,0.0};

	cout << "[f(x),f'(x),f''(x)] = "<< parse_tree(num1) << " : " ;
	imprime(parsv(num1,feval)) ;
	cout << endl;
	
	t1 = clock();
	cout << "::::::::::::::::::" << endl;
	double time = (double(t1-t0)/CLOCKS_PER_SEC);
	cout << "Execution Time: " << time << endl;
	cout << " Por Lmex " << endl;
	
	return 0;
}
