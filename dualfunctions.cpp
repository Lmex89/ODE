#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>
#include <complex>

using namespace std;
typedef long long int llint;
typedef  long double ldouble;
typedef string  str;
using namespace std::complex_literals;

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
	f[1] = xval[0]/(pow(cos(xval[0]),2));
	f[2] = xval[2]/pow(cos(xval[0]),2) + ((2.0*tan(xval[0]))/pow(cos(xval[0]),2))*pow(xval[1],2);

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

vector<ldouble> pow2dual(vector<ldouble>&x, vector<ldouble>& y){


	vector<ldouble> f(3);


	using namespace std::complex_literals;
	complex <long double> xz,yz,z1,z2,z3;
	complex <long double>  uno(1.0,0),dos(2.0,0.0);
	xz = x[0] ;
	yz  = y[0];

	ldouble eps = 0.000001;
	z1 = pow(x[0],y[0]);
	//cout << "z1 "<< z1   << endl;
	z2 = z1*(x[1]*y[0]/x[0] + y[1]*log(xz) );
	//cout << "z2 : "<<z2 << endl;
	z3 = z1*((pow(x[1],2)*(-uno + y[0])*y[0])/pow(x[0],2) + (x[2]*y[0]+2.0*x[1]*(uno + log(xz)*y[0])*y[1])/x[0] + log(xz)*(log(xz)*pow(y[1],dos) + y[2]));
	//cout << "z3 " << z3 << endl;
	 if (abs(imag(z1)) < eps){
		f[0] = real(z1);
		}
	else{
		f[0] = real(z1);
	}

	 if (abs(imag(z2)) < eps){
		f[1] = real(z2);
		}
	else{
		f[1] = real(z2);
	}
	 if (abs(imag(z3)) < eps){
		f[2] = real(z3);
		}
	else{
		f[2] = real(z3);
	}

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
	str tmp ;
	string f_string ="";

	int n = v.size();

	for (int i = 0; i< n ; i++){
	    if (v[i]==1) {
            tmp="1.0";
		}
        else if (v[i]==2)
            tmp="2.0";
        else if (v[i]==3)
            tmp="3.0";
        else if (v[i]==4)
            tmp="4.0";
        else if (v[i]==5)
            tmp="5.0";
        else if (v[i]==6)
            tmp="6.0";
        else if(v[i]==7)
            tmp="7.0";
        else if (v[i]==8)
            tmp="8.0";
        else if (v[i]==9)
            tmp="9.0";
        else if (v[i]==10)
            tmp="10.0";
        else if (v[i]==11)
            tmp='x';
        else if (v[i]==12)
            tmp="-1.0*()";
        else if (v[i]==13)
            tmp="sin()";
        else if (v[i]==14)
            tmp="cos()";
        else if (v[i]==15)
            tmp="exp()";
        else if (v[i]==16)
            tmp="log()";
		else if (v[i]==17)
            tmp="()**(1/2)";
        else if (v[i]==18)
            tmp="()+()";
        else if (v[i]==19)
            tmp="()*()";
        else if (v[i]==20)
			tmp="()/()";
		else if (v[i]==21)
			tmp="()**()";

		fun.push_back(tmp)  ;
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
*/
	int f_result ;
	if (x >=  1 && x <= 12) f_result = 0;
	if (x >= 13 && x <= 17) f_result = 1;
	if (x >= 18 && x <= 21) f_result = 2;
	return f_result;
}

vector<long double> OperF2(ldouble x,vector<vector<ldouble>>& vecval) {
/*
 * Esta funcion opera cuando +*-/ cuando son 2 argumentos y despues aplica
 * NAN a todo lo demás
*/

	int j,cont = 0,dim;
	const long double NANf = sqrt(-1);
	vector<ldouble> temp;
	temp.assign(3,NANf);
	vector<long double> f_result = {NAN,NAN,NAN};;
	vector<vector<ldouble>> auxiliar;
	dim=vecval[0].size();

	for (int i = 0 ; i < 3; i++){
		auxiliar.push_back(temp);
	}

	//cout << dim << "Estou en dim " <<endl;
	//cout  << "Estou en dim " <<endl;
	//cout << "estoy en Operf2" << endl;
	//imprime(vecval[0]);
	//imprime(vecval[1]);


	for ( j = 0; j < dim -1; j++){
	if(isnan(vecval[0][j])){
		continue;
		}
	else{
		auxiliar[0][cont]=vecval[0][j];
		auxiliar[1][cont]=vecval[1][j];
		auxiliar[2][cont]=vecval[2][j];
		vecval[0][j+1]=NANf;

		cont++;
	}
	if(cont == 2) break;
	}


	if (x == 18){

		/*	imprime( auxiliar[0]);
			imprime( auxiliar[1]);*/

		f_result[0]    = auxiliar[0][0] + auxiliar[0][1];
		f_result[1]    = auxiliar[1][0] + auxiliar[1][1];
		f_result[2]    = auxiliar[2][0] + auxiliar[2][1];
	}

	if (x == 19) {
		vector<ldouble> x1(3),y(3);

		for (int h = 0; h < 3; h++){
			x1[h]=auxiliar[h][0];
			y[h]=auxiliar[h][1];
		}

		f_result = prod2dual(x1,y);
	}

	if (x == 20) {
		vector<ldouble> x1(3),y(3);
		for (int h = 0; h < 3; h++){
			x1[h]=auxiliar[h][0];
			y[h]=auxiliar[h][1];
		}

		f_result = division2dual(x1,y);
	}
	if (x == 21) {
		vector<ldouble> x1(3),y(3);
		for (int h = 0; h < 3; h++){
			x1[h]=auxiliar[h][0];
			y[h]=auxiliar[h][1];
		}

		f_result = pow2dual(x1,y);
	}

	return f_result;
}



vector<ldouble> OperF(ldouble x, vector<ldouble>& xval){

/*Esta funcion evalua las funciones ya descritas
 * en la poblacion incial, con un solo argumento.
 */
	vector<ldouble> f_result  = {NAN,NAN,NAN};

	if (x == 11){
		f_result[0] = xval[0] ;
		f_result[1] = xval[1] ;
		f_result[2] = xval[2] ;
	}

	if (x == 12) {
		f_result[0] = -1.0*xval[0] ;
		f_result[1] = -1.0*xval[1] ;
		f_result[2] = -1.0*xval[2] ;
	}
	if (x == 13) f_result = sin2dual(xval);
	if (x == 14) f_result = cos2dual(xval);
	if (x == 15) f_result = exp2dual(xval);
	if (x == 16) f_result = log2dual(xval);
	if (x == 17) f_result = sqrt2dual((xval));

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
		if(x[k] >= 1 || x[k] <= 10){
			 auxv[0][k]= (long double) 1.0*x[k];
			 auxv[1][k]= (long double) 0.0;
			 auxv[2][k]= (long double) 0.0;
		 }
		if(x[k] == 11){
			auxv[0][k]= xval[0];
			auxv[1][k]= xval[1];
			auxv[2][k]= xval[2];
		}

		if(x[k] == 12){
			 auxv[0][k] = -1.0*xval[0];
			 auxv[1][k]=  -1.0*xval[1];
			 auxv[2][k]=  -1.0*xval[2];
			 }
	}


	for (k = dim - 2; k>= 0; k--){
	if( narg(x[k]) == 1  && isnan(auxv[0][k+1]) == false   ){
		//cout << "dentro del fi" << endl;

		//imprime(auxv[2]); cout << endl;
		//cout <<" narg1" << endl;

		vector<long double> xuax(3) ;
		xuax[0] = auxv[0][k+1];
		xuax[1] = auxv[1][k+1];
		xuax[2] = auxv[2][k+1];
		//cout <<"dasd"<< xuax[2] << endl;

		vector<long double > temp(OperF(x[k],xuax));
		auxv[0][k] = temp[0];
		auxv[1][k] = temp[1];
		auxv[2][k] = temp[2];


	//	imprime(temp);
		//cout << endl;



	}
		else if( narg(x[k]) == 2  ){


		vector<ldouble> auxvtemp1(auxv[0].begin()+k+1,auxv[0].end());
		vector<ldouble> auxvtemp2(auxv[1].begin()+k+1,auxv[1].end());
		vector<ldouble> auxvtemp3(auxv[2].begin()+k+1,auxv[2].end());

		vector< vector<ldouble>> auxv321;

			auxv321.push_back(auxvtemp1);
			auxv321.push_back(auxvtemp2);
			auxv321.push_back(auxvtemp3);



		//vector<ldouble> auxv23(auxv[2].begin()+k+1,auxv[2].end());
		//cout << auxv2.size() << " los "<< auxv2[0] << endl;
		//imprime(auxv321[0]);
		//cout << "///////" << endl;
		//imprime(auxvtemp1) ;
		//cout << ":::" << endl;
		vector<ldouble> temp(OperF2(x[k],auxv321));

		auxv[0][k]=temp[0];
		auxv[1][k]=temp[1];
		auxv[2][k]=	temp[2];
		/*auxv[0][k+1]=NAN;
		auxv[1][k+1]=NAN;
		auxv[2][k+1]=NAN;
		*/
		//cout << "auxv[k] = "<<auxv[0][k]  << endl;

			}
		}
	f_result[0] = auxv[0][0];
	f_result[1] = auxv[1][0];
	f_result[2] = auxv[2][0];


	return f_result ;
}
