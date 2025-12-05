#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

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
    vector<int> erastotones;
    for(int j = 0; j < n; j++){
        erastotones.push_back(j + 2);
    }
    auto fimSeq = high_resolution_clock::now();
    
    auto inicioParal = high_resolution_clock::now();
    for(int j = 0; j < erastotones.size(); j++){
        int divisor = erastotones[j];
        for(int c = erastotones.size() - 1; c >= 0; c--){
            if(c != j && erastotones[c] % divisor == 0){
                erastotones.erase(erastotones.begin() + c);
            }
        }
    }
    auto fimParal = high_resolution_clock::now();
    
    auto fim = high_resolution_clock::now();
    
    medicao.tempoTotal = duration_cast<microseconds>(fim - inicio).count();
    medicao.tempoSequencial = duration_cast<microseconds>(fimSeq - inicioSeq).count();
    medicao.tempoParalelizavel = duration_cast<microseconds>(fimParal - inicioParal).count();
    medicao.quantidadePrimos = erastotones.size();
    medicao.primos = erastotones;
    medicao.numProcessos = 1;
    
    return medicao;
}

int main(int argc, char** argv){
    if (argc < 3) {
        cout << "Uso: ./singular <qtd> <tamanho1> <tamanho2> ..." << endl;
        cout << "Exemplo: ./singular 3 10 100 1000" << endl;
        return 1;
    }
    
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