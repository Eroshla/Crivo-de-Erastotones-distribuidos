#include <iostream>
#include <vector>
#include <chrono>
#include "../utils/experimentos.h"

using namespace std;
using namespace chrono;

// FUNÇÃO PRINCIPAL - CRIVO DE ERATÓSTENES (parte que será distribuída)
Medicao crivoDeEratostenes(int n) {
    Medicao medicao;
    auto inicio = high_resolution_clock::now();
    
    // REGIÃO SEQUENCIAL: Inicialização do vetor
    auto inicioSeq = high_resolution_clock::now();
    vector<int> erastotones;
    for(int j = 0; j < n; j++){
        erastotones.push_back(j + 2);
    }
    auto fimSeq = high_resolution_clock::now();
    
    // REGIÃO PARALELIZÁVEL: Marcação de múltiplos
    auto inicioParal = high_resolution_clock::now();
    for(int j = 0; j < erastotones.size(); j++){
        int divisor = erastotones[j];
        for(int c = erastotones.size() - 1; c >= 0; c--){
            if(c != j && erastotones[c] % divisor == 0){
                erastotenes.erase(erastotones.begin() + c);
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

int main(){
    int Qtd_testes;
    vector<int> tamanhos;

    cout << "CRIVO DE ERATÓSTENES - CONFIGURAÇÃO\n";
    cout << "\nQuantos tamanhos diferentes você quer testar? ";
    cin >> Qtd_testes;
    
    tamanhos.resize(Qtd_testes);
    
    cout << "\nDigite os " << Qtd_testes << " tamanhos dos arrays:\n";
    for(int i = 0; i < Qtd_testes; i++){
        cout << "  Tamanho " << (i+1) << ": ";
        cin >> tamanhos[i];
    }
    
    int numMedicoes;
    cout << "\nQuantas medições por tamanho? (recomendado: 5): ";
    cin >> numMedicoes;
    
    cout << "\nRESUMO\n";
    cout << "Tamanhos a testar: ";
    for(int t : tamanhos) cout << t << " ";
    cout << "\nMedições por tamanho: " << numMedicoes << endl;
    
    realizarExperimentos(tamanhos, numMedicoes, crivoDeEratostenes, "_singular");

    return 0;
}