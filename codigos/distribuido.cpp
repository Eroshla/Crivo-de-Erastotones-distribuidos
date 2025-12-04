#include <iostream>
#include <vector>
#include <chrono>
#include <mpi.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

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
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    auto inicio = high_resolution_clock::now();
    
    auto inicioSeq = high_resolution_clock::now();
    
    int limite = sqrt(n + 1);
    vector<int> primosBase;
    
    if (rank == 0) {
        vector<bool> eh_primo(limite + 1, true);
        eh_primo[0] = eh_primo[1] = false;
        
        for (int p = 2; p * p <= limite; p++) {
            if (eh_primo[p]) {
                for (int i = p * p; i <= limite; i += p) {
                    eh_primo[i] = false;
                }
            }
        }
        
        for (int i = 2; i <= limite; i++) {
            if (eh_primo[i]) primosBase.push_back(i);
        }
    }
    
    int tamPrimosBase = primosBase.size();
    MPI_Bcast(&tamPrimosBase, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank != 0) {
        primosBase.resize(tamPrimosBase);
    }
    
    if (tamPrimosBase > 0) {
        MPI_Bcast(primosBase.data(), tamPrimosBase, MPI_INT, 0, MPI_COMM_WORLD);
    }
    
    auto fimSeq = high_resolution_clock::now();
    
    auto inicioParal = high_resolution_clock::now();
    
    long long total = n;
    long long chunk = (total + size - 1) / size;
    long long inicio_local = 2 + rank * chunk;
    long long fim_local = min(inicio_local + chunk, (long long)(n + 2));
    
    vector<bool> isPrimo_local(fim_local - inicio_local, true);
    
    for (int primo : primosBase) {
        long long start = max((long long)primo * primo, 
                             ((inicio_local + primo - 1) / primo) * primo);
        
        for (long long i = start; i < fim_local; i += primo) {
            isPrimo_local[i - inicio_local] = false;
        }
    }
    
    int primos_locais = 0;
    vector<int> primos_encontrados;
    
    for (long long i = inicio_local; i < fim_local; i++) {
        if (isPrimo_local[i - inicio_local]) {
            primos_locais++;
            if (n <= 10000) {
                primos_encontrados.push_back(i);
            }
        }
    }
    
    int total_primos = 0;
    MPI_Reduce(&primos_locais, &total_primos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    vector<int> todos_primos;
    if (n <= 10000) {
        if (rank == 0) {
            vector<int> counts(size);
            vector<int> displs(size);
            
            int meu_count = primos_encontrados.size();
            MPI_Gather(&meu_count, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
            
            int total_elementos = 0;
            for (int i = 0; i < size; i++) {
                displs[i] = total_elementos;
                total_elementos += counts[i];
            }
            
            todos_primos.resize(total_elementos);
            MPI_Gatherv(primos_encontrados.data(), meu_count, MPI_INT,
                        todos_primos.data(), counts.data(), displs.data(), MPI_INT,
                        0, MPI_COMM_WORLD);
        } else {
            int meu_count = primos_encontrados.size();
            MPI_Gather(&meu_count, 1, MPI_INT, nullptr, 0, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Gatherv(primos_encontrados.data(), meu_count, MPI_INT,
                        nullptr, nullptr, nullptr, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }
    
    auto fimParal = high_resolution_clock::now();
    auto fimTotal = high_resolution_clock::now();
    
    if (rank == 0) {
        medicao.tempoTotal = duration_cast<microseconds>(fimTotal - inicio).count();
        medicao.tempoSequencial = duration_cast<microseconds>(fimSeq - inicioSeq).count();
        medicao.tempoParalelizavel = duration_cast<microseconds>(fimParal - inicioParal).count();
        medicao.quantidadePrimos = total_primos;
        medicao.primos = (n <= 10000) ? todos_primos : vector<int>();
        medicao.numProcessos = size;
    }
    
    return medicao;
}

int main(int argc, char** argv){
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    
    cout << "[RANK " << rank << "] Processo iniciado no host: " << processor_name 
         << " (PID: " << getpid() << ")" << endl;
    
    int qtd;
    vector<int> tamanhos;
    int numMedicoes = 5;

    if (rank == 0) {
        if (argc < 3) {
            cout << "Uso: mpirun -np N ./distribuido <qtd_tamanhos> <tamanho1> <tamanho2> ..." << endl;
            cout << "Exemplo: mpirun -np 8 ./distribuido 3 1000 10000 100000" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        cout << "\n[RANK 0] CRIVO DE ERATÓSTENES - CONFIGURAÇÃO\n";
        
        qtd = atoi(argv[1]);
        
        if (qtd > 10) {
            cout << "[RANK 0] Limitando a 10 tamanhos" << endl;
            qtd = 10;
        }
        
        if (argc < 2 + qtd) {
            cout << "[RANK 0] Erro: esperado " << qtd << " tamanhos, mas recebeu " << (argc - 2) << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        tamanhos.resize(qtd);
        
        for(int i = 0; i < qtd; i++){
            tamanhos[i] = atoi(argv[2 + i]);
        }
        
        cout << "\n[RANK 0] RESUMO\n";
        cout << "[RANK 0] Tamanhos a testar: ";
        for(int t : tamanhos) cout << t << " ";
        cout << "\n[RANK 0] Medições por tamanho: " << numMedicoes << endl;
        cout << "[RANK 0] Total de processos: " << size << endl;
    }
    
    MPI_Bcast(&qtd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank != 0) {
        tamanhos.resize(qtd);
    }
    
    MPI_Bcast(tamanhos.data(), qtd, MPI_INT, 0, MPI_COMM_WORLD);
    
    cout << "[RANK " << rank << "] Recebeu " << qtd << " tamanhos para processar" << endl;
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        cout << "\n[RANK 0] INICIANDO EXPERIMENTOS (" << size << " processos)\n";
        
        for (int tamanho : tamanhos) {
            cout << "\n[RANK 0] ===== Processando tamanho: " << tamanho << " =====" << endl;
            
            vector<Medicao> medicoes;
            
            for (int i = 0; i < numMedicoes; i++) {
                cout << "[RANK 0] Medição " << (i + 1) << "/" << numMedicoes << "..." << flush;
                
                Medicao m = crivoDeEratostenes(tamanho);
                medicoes.push_back(m);
                
                cout << " OK - Tempo: " << m.tempoTotal << " us (" << m.quantidadePrimos << " primos)" << endl;
            }
            
            long long somaTotal = 0, somaParal = 0, somaSeq = 0;
            for (const auto& m : medicoes) {
                somaTotal += m.tempoTotal;
                somaParal += m.tempoParalelizavel;
                somaSeq += m.tempoSequencial;
            }
            
            double mediaTempo = somaTotal / (double)numMedicoes;
            double mediaParal = somaParal / (double)numMedicoes;
            double mediaSeq = somaSeq / (double)numMedicoes;
            double fracaoParalMedia = (mediaParal > 0) ? (mediaParal / mediaTempo) * 100.0 : 0.0;
            double pMedia = (mediaParal > 0) ? mediaParal / mediaTempo : 0.0;
            double speedupMaxTeorico = (pMedia > 0 && pMedia < 1.0) ? 1.0 / (1.0 - pMedia) : 1.0;
            
            cout << "\n[RANK 0] MÉDIAS para tamanho " << tamanho << ":" << endl;
            cout << "[RANK 0]   Tempo total: " << mediaTempo << " us" << endl;
            cout << "[RANK 0]   Tempo paralelo: " << mediaParal << " us" << endl;
            cout << "[RANK 0]   Tempo sequencial: " << mediaSeq << " us" << endl;
            cout << "[RANK 0]   Fração paralelizável: " << fixed << setprecision(2) << fracaoParalMedia << "%" << endl;
            cout << "[RANK 0]   Speedup máximo teórico: " << setprecision(4) << speedupMaxTeorico << "x" << endl;
        }
        
        cout << "\n[RANK 0] EXPERIMENTOS CONCLUÍDOS\n";
    } else {
        cout << "[RANK " << rank << "] Iniciando modo worker..." << endl;
        for (int t : tamanhos) {
            cout << "[RANK " << rank << "] Processando tamanho " << t << "..." << endl;
            for (int i = 0; i < numMedicoes; i++) {
                cout << "[RANK " << rank << "] Medição " << (i+1) << "/" << numMedicoes << " de tamanho " << t << endl;
                crivoDeEratostenes(t);
            }
        }
        cout << "[RANK " << rank << "] Worker finalizado" << endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    cout << "[RANK " << rank << "] Finalizando..." << endl;
    
    MPI_Finalize();
    return 0;
}
