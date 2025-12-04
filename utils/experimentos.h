#ifndef EXPERIMENTOS_H
#define EXPERIMENTOS_H

#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

struct Medicao {
    long long tempoTotal;
    long long tempoParalelizavel;
    long long tempoSequencial;
    int quantidadePrimos;
    vector<int> primos;
    int numProcessos;
};

void realizarExperimentos(const vector<int>& tamanhos, int numMedicoes, 
                          Medicao (*funcaoCrivo)(int), const string& sufixo = "") {
    // Pega o número de processos da primeira medição para nomear o arquivo
    Medicao temp = funcaoCrivo(10);
    
    stringstream ss;
    ss << "resultados/resultados_eratostenes" << sufixo << "_" << temp.numProcessos << "procs";
    string nomeBase = ss.str();
    
    string arquivoCSV = nomeBase + ".csv";
    string arquivoTXT = nomeBase + ".txt";
    
    ofstream csv(arquivoCSV);
    ofstream txt(arquivoTXT);
    
    csv << "Tamanho,Medicao,TempoTotal(us),TempoParalelizavel(us),TempoSequencial(us),QuantidadePrimos,FracaoParalelizavel(%),SpeedupMaxTeorico,NumProcessos\n";
    
    txt << "CRIVO DE ERATÓSTENES - ANÁLISE DETALHADA\n";
    txt << "Número de processos: " << temp.numProcessos << "\n\n";
    
    cout << "\nINICIANDO EXPERIMENTOS (" << temp.numProcessos << " processos)\n";
    
    for (int tamanho : tamanhos) {
        cout << "\nProcessando tamanho: " << tamanho << endl;
        
        txt << "TAMANHO DE ENTRADA: " << tamanho << "\n";
        
        vector<Medicao> medicoes;
        
        for (int i = 0; i < numMedicoes; i++) {
            cout << "  Medição " << (i + 1) << "/" << numMedicoes << "...";
            
            Medicao m = funcaoCrivo(tamanho);
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
}

#endif
