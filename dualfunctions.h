

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
vector<long double> OperF2(ldouble x,vector<vector<ldouble>>& vecval);
vector<ldouble> OperF(ldouble x, vector<ldouble>& xval);
vector<ldouble> parsv(vector<ldouble> x, vector<ldouble> xval);
