#include <iostream>
#include <vector>
#include <chrono>
#include <mpi.h>
#include <cmath>
#include "../utils/experimentos.h"

using namespace std;
using namespace chrono;

Medicao crivoDeEratostenes(int n) {
    Medicao medicao;
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    auto inicio = high_resolution_clock::now();
    
    auto inicioSeq = high_resolution_clock::now();
    vector<int> isPrimo(n + 2);
    if (rank == 0) {
        for(int i = 0; i < n + 2; i++) isPrimo[i] = 1;
        isPrimo[0] = isPrimo[1] = 0;
    }
    MPI_Bcast(isPrimo.data(), n + 2, MPI_INT, 0, MPI_COMM_WORLD);
    auto fimSeq = high_resolution_clock::now();
    
    auto inicioParal = high_resolution_clock::now();
    int limite = sqrt(n + 1);
    int chunk = (limite - 2 + size - 1) / size;
    int ini = 2 + rank * chunk;
    int fim = min(ini + chunk, limite + 1);
    
    for (int p = ini; p < fim; p++) {
        if (isPrimo[p]) {
            for (int i = p * p; i <= n + 1; i += p) {
                isPrimo[i] = 0;
            }
        }
    }
    
    vector<int> resultado(n + 2);
    MPI_Allreduce(isPrimo.data(), resultado.data(), n + 2, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    auto fimParal = high_resolution_clock::now();
    
    vector<int> primos;
    if (rank == 0) {
        for (int i = 2; i <= n + 1; i++) {
            if (resultado[i]) primos.push_back(i);
        }
    }
    
    auto fim = high_resolution_clock::now();
    
    if (rank == 0) {
        medicao.tempoTotal = duration_cast<microseconds>(fim - inicio).count();
        medicao.tempoSequencial = duration_cast<microseconds>(fimSeq - inicioSeq).count();
        medicao.tempoParalelizavel = duration_cast<microseconds>(fimParal - inicioParal).count();
        medicao.quantidadePrimos = primos.size();
        medicao.primos = primos;
        medicao.numProcessos = size;
    }
    
    return medicao;
}

int main(){
    MPI_Init(NULL, NULL);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int qtd;
    vector<int> tamanhos;

    if (rank == 0) {
        cout << "Quantos tamanhos testar? ";
        cin >> qtd;
    }
    
    MPI_Bcast(&qtd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    tamanhos.resize(qtd);
    
    if (rank == 0) {
        for(int i = 0; i < qtd; i++){
            cout << "Tamanho " << (i+1) << ": ";
            cin >> tamanhos[i];
        }
    }
    
    MPI_Bcast(tamanhos.data(), qtd, MPI_INT, 0, MPI_COMM_WORLD);
    
    int medicoes;
    if (rank == 0) {
        cout << "Quantas medicoes? ";
        cin >> medicoes;
    }
    
    MPI_Bcast(&medicoes, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        realizarExperimentos(tamanhos, medicoes, crivoDeEratostenes, "_distribuido");
    } else {
        for (int t : tamanhos) {
            for (int i = 0; i < medicoes; i++) {
                crivoDeEratostenes(t);
            }
        }
    }
    
    MPI_Finalize();
    return 0;
}