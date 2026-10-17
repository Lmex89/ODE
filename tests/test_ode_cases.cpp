// tests/test_ode_cases.cpp — Issue #103: cases 3-5 (linear oscillators)
//
// Each case has a known exact solution expressed as a token vector.
// The test verifies that fitness() < 1e-6 for the exact solution,
// confirming both the residual and initial conditions are correct.

#include <iostream>
#include <vector>
#include <cmath>

#include "../src/token.h"

// FunbObj has no header; compile parse_tree.cpp with -DODE_NO_MAIN
// (same pattern as test_grid.cpp, test_funbobj_const.cpp).
#include "../src/parse_tree.cpp"

using namespace std;
using ldouble = long double;

static int g_checks = 0;
static int g_fallos = 0;

static void check_bool(const string& nombre, bool cond) {
    g_checks++;
    if (cond) {
        cout << "PASS  " << nombre << "\n";
    } else {
        cout << "FAIL  " << nombre << "\n";
        g_fallos++;
    }
}

// -------------------------------------------------------------------------
// Case 3: Damped oscillator
//   ODE: y'' + 2y' + 2y = 0
//   ICs: y(0)=1, y'(0)=-1
//   Solution: y = e^(-x) * cos(x)
//
// Token vector (prefix notation):
//   MUL(EXP(NEG(X)), COS(X))
//   = {19, 15, 12, 11, 14, 11}
// -------------------------------------------------------------------------

static void test_caso3_solucion_exacta() {
    // e^(-x) * cos(x) in prefix: MUL EXP NEG X COS X
    vector<ldouble> tokens = {TOKEN_MUL, TOKEN_EXP, TOKEN_NEG, TOKEN_X, TOKEN_COS, TOKEN_X};
    FunbObj f(tokens, 0.0L, 1.0L, 3);
    const long double fit = f.fitness();

    check_bool("fitness_caso3(e^(-x)*cos(x)) es finita", std::isfinite(fit));
    check_bool("fitness_caso3(e^(-x)*cos(x)) < 1e-6 (solucion exacta)",
               fit < 1e-6L);
}

// -------------------------------------------------------------------------
// Case 4: Forced oscillator
//   ODE: y'' + y = e^x
//   ICs: y(0)=0, y'(0)=0
//   Solution: y = (e^x - cos(x) - sin(x)) / 2
//
// Token vector (prefix notation):
//   DIV(ADD(EXP(X), NEG(ADD(COS(X), SIN(X)))), CONST2)
//   = {20, 18, 15, 11, 12, 18, 14, 11, 13, 11, 2}
// -------------------------------------------------------------------------

static void test_caso4_solucion_exacta() {
    // (e^x - cos(x) - sin(x)) / 2
    vector<ldouble> tokens = {TOKEN_DIV, TOKEN_ADD, TOKEN_EXP, TOKEN_X,
                               TOKEN_NEG, TOKEN_ADD, TOKEN_COS, TOKEN_X,
                               TOKEN_SIN, TOKEN_X, TOKEN_CONST2};
    FunbObj f(tokens, 0.0L, 1.0L, 4);
    const long double fit = f.fitness();

    check_bool("fitness_caso4((e^x-cos-sin)/2) es finita", std::isfinite(fit));
    check_bool("fitness_caso4((e^x-cos-sin)/2) < 1e-6 (solucion exacta)",
               fit < 1e-6L);
}

// -------------------------------------------------------------------------
// Case 5: Resonance
//   ODE: y'' - y = e^x
//   ICs: y(0)=0, y'(0)=0
//   Solution: y = (2x*e^x - e^x + e^(-x)) / 4
//
// Token vector (prefix notation):
//   DIV(ADD(MUL(EXP(X), ADD(MUL(CONST2,X), NEG(CONST1))), EXP(NEG(X))), CONST4)
//   = {20, 18, 19, 15, 11, 18, 19, 2, 11, 12, 1, 15, 12, 11, 4}
// -------------------------------------------------------------------------

static void test_caso5_solucion_exacta() {
    // (2x*e^x - e^x + e^(-x)) / 4
    vector<ldouble> tokens = {TOKEN_DIV, TOKEN_ADD,
                               TOKEN_MUL, TOKEN_EXP, TOKEN_X,
                               TOKEN_ADD, TOKEN_MUL, TOKEN_CONST2, TOKEN_X,
                               TOKEN_NEG, TOKEN_CONST1,
                               TOKEN_EXP, TOKEN_NEG, TOKEN_X,
                               TOKEN_CONST4};
    FunbObj f(tokens, 0.0L, 1.0L, 5);
    const long double fit = f.fitness();

    check_bool("fitness_caso5((2xe^x-e^x+e^-x)/4) es finita", std::isfinite(fit));
    check_bool("fitness_caso5((2xe^x-e^x+e^-x)/4) < 1e-6 (solucion exacta)",
               fit < 1e-6L);
}

int main() {
    cout << "== test_ode_cases: issue #103 (cases 3-5, linear oscillators) ==\n";
    test_caso3_solucion_exacta();
    test_caso4_solucion_exacta();
    test_caso5_solucion_exacta();
    cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
         << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;
}
