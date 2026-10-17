// test_population.cpp — tests del semillero de la poblacion (issue #11) y de
// la configuracion de la busqueda (issue #28).
//
// Para un desarrollador Python: este archivo es como un modulo `test_*.py` de
// pytest, pero sin framework: cada `test_*()` imprime PASS/FAIL y `main()` al
// final hace de resumen (seria el `pytest.main()` + exit code). El binario
// termina con codigo != 0 si algo fallo, igual que pytest.
//
// Se compila con -DODE_NO_MAIN (apaga el main() del programa y deja el de este
// archivo) y junto con dualfunctions.cpp y semilla.cpp; parse_tree.cpp se
// incluye mas abajo en vez de enlazarse, porque FunbObj no tiene header (mismo
// patron que test_grid.cpp: ver el comentario del include).
// En Python no existe "dos main() en un mismo binario": aqui el preprocesador
// `#ifndef ODE_NO_MAIN` del programa deja al compilador elegir que main() incluir.

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "dualfunctions.h"  // ahora autocontenido: incluye <vector>/<string> por si mismo
#include "token.h"          // expone el contrato publico del rango del alfabeto

// En vez de declarar prototipos sueltos, este test incluye parse_tree.cpp
// entero: createPopulation y FunbObj viven ahi sin header. En Python seria
// `from parse_tree import createPopulation, FunbObj`: importar el modulo real.
// El main() del programa queda fuera por -DODE_NO_MAIN (ver el Makefile).
#include "parse_tree.cpp"

// -------------------------------------------------------------------------
// Arnes minimo (mismo estilo que test_dual/test_grid): contadores globales y
// helpers check_* que imprimen PASS/FAIL.
// -------------------------------------------------------------------------

static int g_checks = 0;   // total de asserts ejecutados
static int g_fallos = 0;   // asserts que fallaron

// check_bool: para invariantes que no son igualdades (`assert cond, mensaje`).
static void check_bool(const std::string& nombre, bool cond) {
    g_checks++;
    if (cond) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "\n";
        g_fallos++;
    }
}

// check_int: para igualdades entre enteros; imprime el valor recibido cuando
// falla, como el `assert got == esperado, f"...{got}..."` de pytest.
static void check_int(const std::string& nombre, int got, int esperado) {
    g_checks++;
    if (got == esperado) {
        std::cout << "PASS  " << nombre << "\n";
    } else {
        std::cout << "FAIL  " << nombre << "  (got=" << got
                  << " esperado=" << esperado << ")\n";
        g_fallos++;
    }
}

// -------------------------------------------------------------------------
// Slice 1 (issue #11): todo individuo del semillero mide lo mismo — 20 tokens.
// B5 de REVIEW.md: el seed `dos` tenia 21 y `Mutation()` leia fuera de rango
// cuando ese individuo entraba como operando x. La invariante de diseno es que
// la poblacion inicial es uniforme: los 5 individuos escritos a mano y los
// aleatorios de vector_population(20) miden 20 tokens.
// -------------------------------------------------------------------------

static void test_createPopulation_tamano_uniforme() {
    // srand fijo: createPopulation sortea los individuos aleatorios con rand();
    // fijar la semilla hace el test reproducible (en Python seria
    // `random.seed(20261013)`), igual que en los tests de mutacion/cruce.
    srand(20261013);

    // Configuracion por defecto del Caso 1 del paper (issue #28): dimension 20.
    ConfigBusqueda config;
    config.poblacion = 10;

    // `poblacion` entra por referencia y createPopulation la llena con
    // push_back (en Python seria `poblacion = []` pasada por referencia).
    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    check_bool("createPopulation produce individuos", !poblacion.empty());

    // vecvalue() es la via publica de lectura; _vecval es privado (Python no
    // tiene esta distincion: todo seria publico).
    bool todos_20 = true;
    for (size_t i = 0; i < poblacion.size(); i++) {
        if (poblacion[i].vecvalue().size() != 20) todos_20 = false;
    }
    check_bool("todos los individuos tienen 20 tokens (dimension del paper)", todos_20);
}

// -------------------------------------------------------------------------
// Slice 2 (issue #28): createPopulation crea EXACTAMENTE los individuos que
// pide config.poblacion. Antes el bucle aleatorio era `i < tam - 4` con 5
// semillas fijas, asi que una poblacion de 10 salia de 11 individuos (el
// off-by-one que reporta REVIEW C5: 5 + tam - 4 = tam + 1).
// -------------------------------------------------------------------------

static void test_createPopulation_cantidad_exacta() {
    srand(20261013);

    ConfigBusqueda config;
    config.poblacion = 10;

    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    // 10 es un literal independiente de la implementacion: el contrato es
    // "tantos individuos como pidas", no "los que salgan de la formula".
    check_bool("createPopulation(10) crea exactamente 10 individuos",
               poblacion.size() == 10);
}

// -------------------------------------------------------------------------
// Slice 3 (issue #28): la dimension sale de la configuracion, no de un 20
// fijo. Con ODE_DIM=12 la poblacion entera debe medir 12 tokens y seguir
// usando solo el alfabeto TOKEN_MIN..TOKENS_ALFABETO (el criterio de
// "individuos validos" de #28).
// -------------------------------------------------------------------------
static void test_createPopulation_respeta_la_dimension() {
    srand(20261013);

    ConfigBusqueda config;
    config.poblacion = 8;
    config.dimension = 12;

    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    bool todos_12 = true;
    bool tokens_validos = true;
    for (size_t i = 0; i < poblacion.size(); i++) {
        const vector<ldouble>& v = poblacion[i].vecvalue();
        if (v.size() != 12) todos_12 = false;
        for (size_t j = 0; j < v.size(); j++) {
            if (v[j] < TOKEN_MIN || v[j] > TOKENS_ALFABETO) tokens_validos = false;
        }
    }
    check_bool("cada individuo mide la dimension pedida (12)", todos_12);
    check_bool("todos los tokens estan en el alfabeto TOKEN_MIN..TOKENS_ALFABETO",
               tokens_validos);
}

// -------------------------------------------------------------------------
// Slice 4 (issue #28): el warm start es opt-in. Por defecto la poblacion es
// aleatoria uniforme (ecuacion (6) del paper, REVIEW D6); con
// ODE_WARM_START=1 entran las cinco expresiones escritas a mano del prototipo.
// -------------------------------------------------------------------------

static void test_warm_start_opt_in() {
    // semillas_warm_start() devuelve la lista por referencia constante (en
    // Python seria la misma lista del modulo, sin copiarla).
    const vector<vector<ldouble>>& semillas = semillas_warm_start();

    check_bool("el prototipo aporta cinco semillas warm start", semillas.size() == 5);
    bool miden_20 = true;
    for (size_t i = 0; i < semillas.size(); i++) {
        if (semillas[i].size() != 20) miden_20 = false;
    }
    check_bool("cada semilla mide la dimension del paper (20)", miden_20);

    // Con warm start, las semillas van al frente de la poblacion.
    srand(20261013);
    ConfigBusqueda caliente;
    caliente.poblacion = 10;
    caliente.warm_start = true;
    vector<FunbObj> con_semillas;
    createPopulation(con_semillas, caliente, 0);

    check_bool("warm start produce exactamente la poblacion pedida",
               con_semillas.size() == 10);

    bool al_frente = true;
    for (size_t i = 0; i < semillas.size(); i++) {
        if (con_semillas[i].vecvalue() != semillas[i]) al_frente = false;
    }
    check_bool("las cinco semillas estan al frente de la poblacion", al_frente);

    // Ancla independiente: la primera semilla es la expresion `uno` del
    // prototipo, transcrita aqui como literal para que un cambio accidental de
    // los datos no pase inadvertido (solo la comprobaria el mismo codigo).
    vector<ldouble> uno = {18,13,17,11,14,14,11,14,14,14,13,14,14,14,14,14,14,14,14,11};
    check_bool("la primera semilla es la `uno` del prototipo",
               con_semillas[0].vecvalue() == uno);

    // Sin warm start ninguna semilla entra. srand fijo hace el sorteo
    // deterministico, asi que la comprobacion no es probabilistica.
    srand(20261013);
    ConfigBusqueda frio;
    frio.poblacion = 10;
    vector<FunbObj> sin_semillas;
    createPopulation(sin_semillas, frio, 0);

    bool ninguna = true;
    for (size_t i = 0; i < sin_semillas.size(); i++) {
        for (size_t s = 0; s < semillas.size(); s++) {
            if (sin_semillas[i].vecvalue() == semillas[s]) ninguna = false;
        }
    }
    check_bool("sin warm start no aparece ninguna semilla fija", ninguna);
}

// -------------------------------------------------------------------------
// Slice 5 (issue #28): la reproducibilidad empieza en la poblacion inicial: la
// misma semilla de rand() debe producir exactamente los mismos individuos
// (criterion 3 de #28).
// -------------------------------------------------------------------------
static void test_createPopulation_reproducible() {
    ConfigBusqueda config;
    config.poblacion = 10;

    srand(20261013);
    vector<FunbObj> primera;
    createPopulation(primera, config, 0);

    srand(20261013);
    vector<FunbObj> segunda;
    createPopulation(segunda, config, 0);

    bool iguales = primera.size() == segunda.size();
    for (size_t i = 0; i < primera.size() && iguales; i++) {
        if (primera[i].vecvalue() != segunda[i].vecvalue()) iguales = false;
    }
    check_bool("la misma semilla produce la misma poblacion inicial", iguales);
}

// -------------------------------------------------------------------------
// Slice 6 (issue #28): configuracion de la busqueda. Sin variables de
// entorno, los valores por defecto son los del Caso 1 del paper (REVIEW
// C5-C7): 100 individuos, dimension 20 y 5000 generaciones. En Python esto
// seria un `@dataclass` con defaults, construido por una funcion pura que
// recibe los `os.environ.get(...)` ya leidos.
// -------------------------------------------------------------------------

static void test_config_defaults() {
    // Mismo patron que resolver_semilla: la funcion recibe los getenv() ya
    // resueltos (nullptr = variable ausente) para poder probarse sin tocar el
    // entorno del proceso.
    ConfigBusqueda config = resolver_config(nullptr, nullptr, nullptr, nullptr);

    // Los literales 100/20/5000 vienen del paper (Caso 1), no de la
    // implementacion: son la fuente de verdad independiente del test.
    check_int("poblacion por defecto = 100 (Caso 1 del paper)", config.poblacion, 100);
    check_int("dimension por defecto = 20 (Caso 1 del paper)", config.dimension, 20);
    check_int("generaciones por defecto = 5000 (Caso 1 del paper)",
              config.max_generaciones, 5000);
    check_bool("warm start desactivado por defecto", !config.warm_start);
}

static void test_config_overrides() {
    // Cada variable de entorno pisa su default (equivalente a leer
    // os.environ["ODE_POP"], etc.). "0" en la bandera desactiva el warm start.
    ConfigBusqueda config = resolver_config("50", "30", "77", "0");

    check_int("ODE_POP=50 sobreescribe la poblacion", config.poblacion, 50);
    check_int("ODE_DIM=30 sobreescribe la dimension", config.dimension, 30);
    check_int("ODE_GEN=77 sobreescribe las generaciones", config.max_generaciones, 77);
    check_bool("ODE_WARM_START=0 desactiva el warm start", !config.warm_start);

    // Con la dimension por defecto (20) el warm start si se acepta.
    ConfigBusqueda caliente = resolver_config(nullptr, nullptr, nullptr, "1");
    check_bool("ODE_WARM_START=1 activa el warm start con dimension 20",
               caliente.warm_start);
}

static void test_config_invalida_usa_respaldo() {
    // "abc" y "" no son numeros, "0" esta por debajo del minimo de
    // generaciones y "tal vez" no es una bandera: cada valor invalido cae a su
    // respaldo, la misma regla que usa resolver_semilla con ODE_SEED.
    ConfigBusqueda config = resolver_config("abc", "", "0", "tal vez");

    check_int("ODE_POP invalida usa el default 100", config.poblacion, 100);
    check_int("ODE_DIM invalida usa el default 20", config.dimension, 20);
    check_int("ODE_GEN invalida usa el default 5000", config.max_generaciones, 5000);
    check_bool("ODE_WARM_START invalida usa el default (apagado)",
               !config.warm_start);

    // Una poblacion menor que 4 no puede elegir 3 individuos base distintos
    // del objetivo (indices_distintos): el minimo aceptado es 4.
    check_int("ODE_POP=3 queda por debajo del minimo y usa el default",
              resolver_config("3", nullptr, nullptr, nullptr).poblacion, 100);
    check_int("ODE_POP=4 es aceptable",
              resolver_config("4", nullptr, nullptr, nullptr).poblacion, 4);

    // "17abc" no es un entero puro: se rechaza entero en vez de quedarse con 17.
    check_int("un texto a medias no es un entero valido",
              resolver_config("17abc", nullptr, nullptr, nullptr).poblacion, 100);
}

static void test_config_warm_start_requiere_dimension_20() {
    // Las cinco semillas warm start miden 20 tokens: con otra dimension no son
    // individuos validos para el emparejamiento gen a gen de
    // Mutation()/Recombination(), asi que la peticion se ignora (el valor
    // efectivo queda registrado en la linea de arranque).
    ConfigBusqueda config = resolver_config(nullptr, "30", nullptr, "1");

    check_int("la dimension pedida se conserva (30)", config.dimension, 30);
    check_bool("ODE_WARM_START=1 se ignora con ODE_DIM=30", !config.warm_start);
}

// -------------------------------------------------------------------------
// Slice 7 (issue #82): reinicio por estancamiento. El reinicio reemplaza el
// 80% de la poblacion para inyectar diversidad, pero el indice reservado
// conserva el mejor historico: antes el `FunbObj` del record se construia y
// se descartaba, y el unico indice que sobrevivia era el mejor de la
// generacion actual (no el record).
// -------------------------------------------------------------------------

static void test_reinicio_reemplaza_el_80_por_ciento() {
    srand(20261013);

    // 15 individuos: el 80% es 12, el mismo criterio que el log real
    // `restart | ... replaced=120` con una poblacion de 150.
    ConfigBusqueda config;
    config.poblacion = 15;

    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    // Copia de los vectores originales para contar cuantos cambian.
    vector<vector<ldouble>> antes;
    for (size_t i = 0; i < poblacion.size(); i++){
        antes.push_back(poblacion[i].vecvalue());
    }

    const int indice_record = 3;
    const vector<ldouble> record = poblacion[indice_record].vecvalue();

    const int reemplazados = reiniciar_poblacion(poblacion, indice_record, record, config, 0);

    check_int("el reinicio reemplaza el 80% de la poblacion (12 de 15)", reemplazados, 12);
    check_bool("el reinicio conserva el tamano de la poblacion", poblacion.size() == 15);

    // Con el record igual al vector del indice reservado, solo deben cambiar
    // los reemplazados. El srand fijo hace deterministico el sorteo, asi que
    // la comprobacion no es probabilistica.
    int cambiados = 0;
    for (size_t i = 0; i < poblacion.size(); i++){
        if (poblacion[i].vecvalue() != antes[i]) cambiados++;
    }
    check_int("solo los reemplazados cambian de vector", cambiados, reemplazados);
}

static void test_reinicio_restaura_el_record() {
    srand(20261013);

    ConfigBusqueda config;
    config.poblacion = 10;
    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    // Record simulado: un vector fijo distinto del individuo que hoy ocupa el
    // indice reservado (en una corrida estancada el mejor de la generacion no
    // es el record, que suele vivir en otro indice). Son tokens validos.
    const vector<ldouble> record = {18, 13, 17, 11};
    const int indice_record = 4;
    check_bool("el record simulado no es el individuo del indice reservado",
               poblacion[indice_record].vecvalue() != record);

    reiniciar_poblacion(poblacion, indice_record, record, config, 0);

    check_bool("el indice reservado contiene exactamente el record restaurado",
               poblacion[indice_record].vecvalue() == record);
}

static void test_reinicio_sin_record_no_toca_el_indice_reservado() {
    srand(20261013);

    ConfigBusqueda config;
    config.poblacion = 10;
    vector<FunbObj> poblacion;
    createPopulation(poblacion, config, 0);

    // `sin_record` simula la corrida en la que nunca hubo un fitness finito:
    // el helper no debe restaurar nada ni tocar el indice reservado.
    const int indice_record = 5;
    const vector<ldouble> original = poblacion[indice_record].vecvalue();
    const vector<ldouble> sin_record;

    const int reemplazados = reiniciar_poblacion(poblacion, indice_record, sin_record, config, 0);

    check_int("sin record el reinicio sigue reemplazando el 80% (8 de 10)", reemplazados, 8);
    check_bool("sin record el indice reservado queda intacto",
               poblacion[indice_record].vecvalue() == original);
}

// -------------------------------------------------------------------------
// Slice 8 (issue #127): reinicio adaptativo. A diferencia del legacy, no
// reemplaza posiciones arbitrarias del arreglo: ordena por fitness y
// reemplaza solo a los peores, asi que los sobrevivientes son los individuos
// realmente mas aptos. El reinicio legacy de arriba queda intacto.
// -------------------------------------------------------------------------

// poblacion_fixture: arma individuos {TOKEN_MUL, TOKEN_CONSTk, TOKEN_X}
// (notacion prefija: `19 k 11` = k*x) para las constantes recibidas. Para el
// caso 0, F(k*x) = integral(k*x)^2 + 10*(k-1)^2 (las demas penalizaciones de
// la ec. (4) son cero porque y(0)=0 y la de y'(0)=k la fija la constante),
// que crece estrictamente con k: el fixture tiene un orden de fitness
// conocido a mano, sin recalcular nada con la implementacion. En Python seria
// `[FunbObj([19, k, 11], 0, 1, 0) for k in constantes]`.
static vector<FunbObj> poblacion_fixture(const vector<int>& constantes) {
    vector<FunbObj> poblacion;
    for (size_t i = 0; i < constantes.size(); i++){
        const int token_constante = TOKEN_CONST1 + (constantes[i] - 1);
        poblacion.push_back(FunbObj({static_cast<ldouble>(TOKEN_MUL),
                                     static_cast<ldouble>(token_constante),
                                     static_cast<ldouble>(TOKEN_X)}, 0, 1, 0));
    }
    return poblacion;
}

// copia_vectores: fotografia de los vectores antes del reinicio, para saber
// quien sobrevivio intacto. En Python seria
// `antes = [ind.vecvalue() for ind in poblacion]`.
static vector<vector<ldouble>> copia_vectores(const vector<FunbObj>& poblacion) {
    vector<vector<ldouble>> copia;
    for (size_t i = 0; i < poblacion.size(); i++){
        copia.push_back(poblacion[i].vecvalue());
    }
    return copia;
}

static void test_reinicio_adaptativo_conserva_los_mejores() {
    srand(20261017);

    // La poblacion baraja los k a proposito: los mas aptos no estan en las
    // primeras posiciones (ese es el quirk que el modo legacy conserva).
    vector<FunbObj> poblacion = poblacion_fixture({3, 1, 5, 2, 4});
    const vector<vector<ldouble>> antes = copia_vectores(poblacion);

    // Premisa del fixture: la fitness real respeta k=1 < k=2 < ... < k=5
    // (F(k) = k^2/3 + 10 + 10*(k-1)^2, calculada a mano). Si esto falla, el
    // resto del test no probaria nada.
    check_bool("fixture: F(k=1) < F(k=2) < ... < F(k=5)",
               poblacion[1].fitness() < poblacion[3].fitness() &&
               poblacion[3].fitness() < poblacion[0].fitness() &&
               poblacion[0].fitness() < poblacion[4].fitness() &&
               poblacion[4].fitness() < poblacion[2].fitness());

    // Record = vector del mejor historico: el k=1 (indice 1).
    const vector<ldouble> record = poblacion[1].vecvalue();

    const int reemplazados =
        reiniciar_poblacion_adaptativo(poblacion, record, {0.4L, 0.5L, 3, 0});

    check_int("el reinicio adaptativo reemplaza el 40% (2 de 5)", reemplazados, 2);
    check_bool("conserva el tamano de la poblacion", poblacion.size() == 5);

    // Sobreviven los 3 mas aptos (k=1, k=2, k=3), sin cambios.
    check_bool("sobrevive intacto el k=1 (indice 1)",
               poblacion[1].vecvalue() == antes[1]);
    check_bool("sobrevive intacto el k=2 (indice 3)",
               poblacion[3].vecvalue() == antes[3]);
    check_bool("sobrevive intacto el k=3 (indice 0)",
               poblacion[0].vecvalue() == antes[0]);

    // Se reemplazan exactamente los peores (k=4 en el indice 4 y k=5 en el
    // indice 2): ninguna posicion arbitraria se toca.
    check_bool("se reemplaza el k=4 (segundo peor, indice 4)",
               poblacion[4].vecvalue() != antes[4]);
    check_bool("se reemplaza el k=5 (peor, indice 2)",
               poblacion[2].vecvalue() != antes[2]);

    check_bool("el record sigue presente en la poblacion",
               poblacion[1].vecvalue() == record);
}

static void test_reinicio_adaptativo_inyecta_el_record() {
    srand(20261018);

    // Fixture {2, 1, 4, 3}: el orden por fitness es k=1 (indice 1), k=2
    // (indice 0), k=3 (indice 3), k=4 (indice 2). Con fraccion 0.5 se
    // conservan 2 y se reemplazan 2.
    vector<FunbObj> poblacion = poblacion_fixture({2, 1, 4, 3});
    const vector<vector<ldouble>> antes = copia_vectores(poblacion);

    // Record simulado que NO esta en la poblacion (el caso real de una corrida
    // estancada donde el record historico no ocupa el indice reservado): la
    // expresion 1+x, mas apta que cualquier k*x del fixture.
    const vector<ldouble> record = {static_cast<ldouble>(TOKEN_ADD),
                                    static_cast<ldouble>(TOKEN_CONST1),
                                    static_cast<ldouble>(TOKEN_X)};

    const int reemplazados =
        reiniciar_poblacion_adaptativo(poblacion, record, {0.5L, 0.5L, 3, 0});

    check_int("inyecta el record reemplazando 2 de 4", reemplazados, 2);

    bool record_presente = false;
    for (size_t i = 0; i < poblacion.size(); i++){
        if (poblacion[i].vecvalue() == record) record_presente = true;
    }
    check_bool("el record ausente entra a la poblacion durante el reinicio",
               record_presente);

    // La inyeccion no sacrifica al mejor: el k=1 sigue intacto.
    check_bool("el mejor real (k=1, indice 1) sobrevive intacto",
               poblacion[1].vecvalue() == antes[1]);
    // Los reemplazados siguen siendo los peores (k=3 y k=4).
    check_bool("se reemplaza el k=3 (indice 3)",
               poblacion[3].vecvalue() != antes[3]);
    check_bool("se reemplaza el k=4 (peor, indice 2)",
               poblacion[2].vecvalue() != antes[2]);

    // Un record vacio (la corrida nunca encontro fitness finito) no inyecta
    // nada: los sobrevivientes quedan exactamente como estaban.
    srand(20261018);
    vector<FunbObj> otra = poblacion_fixture({2, 1, 4, 3});
    const vector<vector<ldouble>> antes_otra = copia_vectores(otra);
    const vector<ldouble> sin_record;

    reiniciar_poblacion_adaptativo(otra, sin_record, {0.5L, 0.5L, 3, 0});

    check_bool("sin record, los 2 sobrevivientes quedan intactos",
               otra[1].vecvalue() == antes_otra[1] &&
               otra[0].vecvalue() == antes_otra[0]);
}

// -------------------------------------------------------------------------
// Slice 9 (issue #127): composicion hibrida de los reemplazos. Una parte son
// mutaciones puntuales de 1-2 tokens de un elite (los sobrevivientes) y el
// resto individuos uniformes nuevos, segun ODE_RESTART_MUTATION_SHARE.
// -------------------------------------------------------------------------

// distancia_hamming: posiciones en que difieren dos vectores. En Python seria
// `sum(1 for a, b in zip(x, y) if a != b)`.
static int distancia_hamming(const vector<ldouble>& a, const vector<ldouble>& b) {
    const size_t n = (a.size() < b.size()) ? a.size() : b.size();
    int distancia = 0;
    for (size_t i = 0; i < n; i++){
        if (a[i] != b[i]) distancia++;
    }
    return distancia;
}

// mutaciones_puntuales: de los indices reemplazados, cuantos quedaron a 1-2
// cambios de algun sobreviviente (la firma de una mutacion puntual de elite;
// con vectores de 20 tokens al azar, un uniforme nuevo no cae tan cerca). En
// Python seria `sum(min(distancia(r, s) for s in sobrevivientes) in (1, 2)
// for r in reemplazados)`.
static int mutaciones_puntuales(const vector<FunbObj>& poblacion,
                                const vector<int>& reemplazados_idx,
                                const vector<int>& sobrevivientes_idx) {
    int mutaciones = 0;
    for (size_t r = 0; r < reemplazados_idx.size(); r++){
        int mejor = 1000;
        for (size_t s = 0; s < sobrevivientes_idx.size(); s++){
            const int d = distancia_hamming(
                poblacion[reemplazados_idx[r]].vecvalue(),
                poblacion[sobrevivientes_idx[s]].vecvalue());
            if (d < mejor) mejor = d;
        }
        if (mejor >= 1 && mejor <= 2) mutaciones++;
    }
    return mutaciones;
}

static void test_reinicio_adaptativo_compone_reemplazos_hibridos() {
    ConfigBusqueda config;
    config.poblacion = 10;
    config.dimension = 20;

    // Tres escenarios con la misma poblacion base y la misma semilla: el
    // unico cambio es la cuota de mutacion. 0.5 se parte 2+2, 1.0 es todo
    // mutacion de elite y 0.0 todo uniforme: asi la composicion se verifica
    // sin depender de clasificar a ojo un sorteo probabilistico.
    // En Python seria una lista de dicts o namedtuples.
    struct Escenario {
        long double share;
        int mutaciones_esperadas;
        const char* nombre;
    };
    const Escenario escenarios[] = {
        {0.5L, 2, "share 0.5"},
        {1.0L, 4, "share 1.0"},
        {0.0L, 0, "share 0.0"},
    };

    for (size_t esc = 0; esc < 3; esc++){
        const Escenario& e = escenarios[esc];
        srand(20261019);

        vector<FunbObj> poblacion;
        createPopulation(poblacion, config, 0);
        const vector<vector<ldouble>> antes = copia_vectores(poblacion);
        const vector<ldouble> record;  // sin record: aca importa la composicion

        const int reemplazados = reiniciar_poblacion_adaptativo(
            poblacion, record, {0.4L, e.share, config.dimension, 0});

        // Sobreviviente = vector intacto; reemplazado = vector cambiado.
        vector<int> sobrevivientes_idx;
        vector<int> reemplazados_idx;
        bool longitudes_ok = true;
        for (size_t i = 0; i < poblacion.size(); i++){
            if (poblacion[i].vecvalue() == antes[i]){
                sobrevivientes_idx.push_back(static_cast<int>(i));
            } else {
                reemplazados_idx.push_back(static_cast<int>(i));
            }
            if (poblacion[i].vecvalue().size() != 20) longitudes_ok = false;
        }

        const string etiqueta = string(e.nombre) + ": ";
        check_int(etiqueta + "reemplaza 4 de 10", reemplazados, 4);
        check_bool(etiqueta + "todos los reemplazos conservan 20 tokens",
                   longitudes_ok);
        check_int(etiqueta + "mutaciones de elite esperadas",
                  mutaciones_puntuales(poblacion, reemplazados_idx,
                                       sobrevivientes_idx),
                  e.mutaciones_esperadas);
        check_int(etiqueta + "uniformes nuevos (el resto)",
                  4 - mutaciones_puntuales(poblacion, reemplazados_idx,
                                           sobrevivientes_idx),
                  4 - e.mutaciones_esperadas);
    }
}

int main() {
    std::cout << "== test_population: issues #11 y #28 (semillero y configuracion) ==\n";
    test_createPopulation_tamano_uniforme();
    test_createPopulation_cantidad_exacta();
    test_createPopulation_respeta_la_dimension();
    test_warm_start_opt_in();
    test_createPopulation_reproducible();
    test_config_defaults();
    test_config_overrides();
    test_config_invalida_usa_respaldo();
    test_config_warm_start_requiere_dimension_20();
    test_reinicio_reemplaza_el_80_por_ciento();
    test_reinicio_restaura_el_record();
    test_reinicio_sin_record_no_toca_el_indice_reservado();
    test_reinicio_adaptativo_conserva_los_mejores();
    test_reinicio_adaptativo_inyecta_el_record();
    test_reinicio_adaptativo_compone_reemplazos_hibridos();
    std::cout << (g_fallos == 0 ? "OK" : "FALLARON") << ": "
              << (g_checks - g_fallos) << "/" << g_checks << " asserts\n";
    return g_fallos == 0 ? 0 : 1;  // como el exit code de pytest
}
