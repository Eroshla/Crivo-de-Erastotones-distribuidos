#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>

using namespace std;
using namespace chrono;

struct Medicao {
    long long tempoTotal;
    long long tempoParalelizavel;
    long long tempoSequencial;
    int quantidadePrimos;
    vector<int> primos;
    int numProcessos;
};

Medicao crivoDeEratostenes(int n) {
    Medicao medicao;
    auto inicio = high_resolution_clock::now();

    auto inicioSeq = high_resolution_clock::now();
    vector<char> eh_primo(n + 1, true);
    if (n >= 0) eh_primo[0] = false;
    if (n >= 1) eh_primo[1] = false;
    int limite = (int)floor(sqrt((double)n));
    for (int p = 2; p <= limite; ++p) {
        if (eh_primo[p]) {
            for (long long i = 1LL * p * p; i <= n; i += p)
                eh_primo[(size_t)i] = false;
        }
    }
    auto fimSeq = high_resolution_clock::now();

    auto inicioParal = high_resolution_clock::now();
    vector<int> primos;
    for (int i = 2; i <= n; ++i)
        if (eh_primo[i]) primos.push_back(i);
    auto fimParal = high_resolution_clock::now();

    auto fim = high_resolution_clock::now();

    medicao.tempoTotal = duration_cast<microseconds>(fim - inicio).count();
    medicao.tempoSequencial = duration_cast<microseconds>(fimSeq - inicioSeq).count();
    medicao.tempoParalelizavel = duration_cast<microseconds>(fimParal - inicioParal).count();
    medicao.quantidadePrimos = primos.size();
    medicao.primos = std::move(primos);
    medicao.numProcessos = 1;

    return medicao;
}

int main(int argc, char** argv){
    int qtd = atoi(argv[1]);
    vector<int> tamanhos(qtd);
    
    for(int i = 0; i < qtd; i++){
        tamanhos[i] = atoi(argv[2 + i]);
    }
    
    cout << "CRIVO DE ERATÓSTENES - SINGULAR\n";
    cout << "Tamanhos: ";
    for(int t : tamanhos) cout << t << " ";
    cout << "\n\n";
    
    for (int tamanho : tamanhos) {
        cout << "Tamanho " << tamanho << ": ";
        
        Medicao m = crivoDeEratostenes(tamanho);
        
        double segundos = m.tempoTotal / 1000000.0;
        
        cout << m.quantidadePrimos << " primos em " 
             << fixed << setprecision(6) << segundos << " s" << endl;
    }

    return 0;
}