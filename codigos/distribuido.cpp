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

int main(){
    MPI_Init(NULL, NULL);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int qtd;
    vector<int> tamanhos;
    int numMedicoes = 5;

    if (rank == 0) {
        cout << "CRIVO DE ERATÓSTENES - CONFIGURAÇÃO\n";
        cout << "\nQuantos tamanhos diferentes você quer testar? ";
        cin >> qtd;
        
        if (qtd > 10) {
            cout << "Limitando a 10 tamanhos" << endl;
            qtd = 10;
        }
    }
    
    MPI_Bcast(&qtd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    tamanhos.resize(qtd);
    
    if (rank == 0) {
        cout << "\nDigite os " << qtd << " tamanhos dos arrays:\n";
        for(int i = 0; i < qtd; i++){
            cout << "  Tamanho " << (i+1) << ": ";
            cin >> tamanhos[i];
        }
        
        cout << "\nRESUMO\n";
        cout << "Tamanhos a testar: ";
        for(int t : tamanhos) cout << t << " ";
        cout << "\nMedições por tamanho: " << numMedicoes << endl;
    }
    
    MPI_Bcast(tamanhos.data(), qtd, MPI_INT, 0, MPI_COMM_WORLD);
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        stringstream ss;
        ss << "resultados/resultados_eratostenes_distribuido_" << size << "procs";
        string nomeBase = ss.str();
        
        string arquivoCSV = nomeBase + ".csv";
        string arquivoTXT = nomeBase + ".txt";
        
        ofstream csv(arquivoCSV);
        ofstream txt(arquivoTXT);
        
        csv << "Tamanho,Medicao,TempoTotal(us),TempoParalelizavel(us),TempoSequencial(us),QuantidadePrimos,FracaoParalelizavel(%),SpeedupMaxTeorico,NumProcessos\n";
        
        txt << "CRIVO DE ERATÓSTENES - ANÁLISE DETALHADA\n";
        txt << "Número de processos: " << size << "\n\n";
        
        cout << "\nINICIANDO EXPERIMENTOS (" << size << " processos)\n";
        
        for (int tamanho : tamanhos) {
            cout << "\nProcessando tamanho: " << tamanho << endl;
            txt << "TAMANHO DE ENTRADA: " << tamanho << "\n";
            
            vector<Medicao> medicoes;
            
            for (int i = 0; i < numMedicoes; i++) {
                cout << "  Medição " << (i + 1) << "/" << numMedicoes << "...";
                
                Medicao m = crivoDeEratostenes(tamanho);
                medicoes.push_back(m);
                
                double fracaoParal = (m.tempoParalelizavel > 0) ? 
                    (double)m.tempoParalelizavel / m.tempoTotal * 100.0 : 0.0;
                double p = (m.tempoParalelizavel > 0) ? 
                    (double)m.tempoParalelizavel / m.tempoTotal : 0.0;
                double speedupMaxTeorico = (p > 0 && p < 1.0) ? 1.0 / (1.0 - p) : 1.0;
                
                csv << tamanho << "," 
                    << (i + 1) << ","
                    << m.tempoTotal << ","
                    << m.tempoParalelizavel << ","
                    << m.tempoSequencial << ","
                    << m.quantidadePrimos << ","
                    << fixed << setprecision(2) << fracaoParal << ","
                    << setprecision(4) << speedupMaxTeorico << ","
                    << m.numProcessos << "\n";
                
                txt << "Medição #" << (i + 1) << ":\n";
                txt << "  tempoTotal_us: " << m.tempoTotal 
                    << ", tempoParalelizavel_us: " << m.tempoParalelizavel 
                    << ", tempoSequencial_us: " << m.tempoSequencial 
                    << ", quantidadePrimos: " << m.quantidadePrimos << "\n";
                
                cout << " OK (" << m.quantidadePrimos << " primos)" << endl;
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
            
            csv << tamanho << ",MEDIA,"
                << fixed << setprecision(2) << mediaTempo << ","
                << mediaParal << ","
                << mediaSeq << ","
                << medicoes[0].quantidadePrimos << ","
                << fracaoParalMedia << ","
                << setprecision(4) << speedupMaxTeorico << ","
                << medicoes[0].numProcessos << "\n\n";
            
            txt << "MÉDIA das " << numMedicoes << " medições:\n";
            txt << "{\n";
            txt << "  tamanho: " << tamanho << ",\n";
            txt << "  numProcessos: " << medicoes[0].numProcessos << ",\n";
            txt << fixed << setprecision(2);
            txt << "  tempoTotal_medio_us: " << mediaTempo << ",\n";
            txt << "  tempoParalelizavel_medio_us: " << mediaParal << ",\n";
            txt << "  tempoSequencial_medio_us: " << mediaSeq << ",\n";
            txt << "  quantidadePrimos: " << medicoes[0].quantidadePrimos << ",\n";
            txt << "  fracaoParalelizavel_pct: " << fracaoParalMedia << ",\n";
            txt << setprecision(4);
            txt << "  speedupMaximoTeorico: " << speedupMaxTeorico << "\n";
            txt << "}\n\n";
            
            cout << "  Tempo médio: " << mediaTempo << " us" << endl;
            cout << "  Fração paralelizável: " << fixed << setprecision(2) << fracaoParalMedia << "%" << endl;
            cout << "  Speedup máximo teórico: " << setprecision(4) << speedupMaxTeorico << "x" << endl;
        }
        
        txt << "FIM DA ANÁLISE\n";
        
        csv.close();
        txt.close();
        
        cout << "\nEXPERIMENTOS CONCLUÍDOS\n";
        cout << "Resultados salvos em: " << arquivoCSV << " e " << arquivoTXT << endl;
    } else {
        for (int t : tamanhos) {
            for (int i = 0; i < numMedicoes; i++) {
                crivoDeEratostenes(t);
            }
        }
    }
    
    MPI_Finalize();
    return 0;
}
