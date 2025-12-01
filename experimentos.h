#ifndef EXPERIMENTOS_H
#define EXPERIMENTOS_H

#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>

using namespace std;

struct Medicao {
    long long tempoTotal;
    int quantidadePrimos;
    vector<int> primos;
};

void realizarExperimentos(const vector<int>& tamanhos, int numMedicoes, 
                          Medicao (*funcaoCrivo)(int)) {
    ofstream csv("resultados_eratostenes.csv");
    ofstream txt("resultados_eratostenes.txt");
    
    csv << "Tamanho,Medicao,TempoTotal(us),QuantidadePrimos\n";
    
    txt << "CRIVO DE ERATÓSTENES - ANÁLISE DETALHADA\n";
    
    cout << "\nINICIANDO EXPERIMENTOS\n";
    
    for (int tamanho : tamanhos) {
        cout << "\nProcessando tamanho: " << tamanho << endl;
        
        txt << "TAMANHO DE ENTRADA: " << tamanho << "\n";
        
        vector<Medicao> medicoes;
        
        for (int i = 0; i < numMedicoes; i++) {
            cout << "  Medição " << (i + 1) << "/" << numMedicoes << "...";
            
            Medicao m = funcaoCrivo(tamanho);
            medicoes.push_back(m);
            
            csv << tamanho << "," 
                << (i + 1) << ","
                << m.tempoTotal << ","
                << m.quantidadePrimos << "\n";
            
            txt << "Medição #" << (i + 1) << ":\n";
            txt << "{\n";
            txt << "  tamanho: " << tamanho << ",\n";
            txt << "  medicao: " << (i + 1) << ",\n";
            txt << "  tempoTotal_us: " << m.tempoTotal << ",\n";
            txt << "  quantidadePrimos: " << m.quantidadePrimos << ",\n";
            txt << "  primos: [\n    ";
            
            for(int p = 0; p < m.primos.size(); p++){
                txt << setw(6) << m.primos[p];
                if(p < m.primos.size() - 1) txt << ",";
                
                if((p + 1) % 10 == 0 && p < m.primos.size() - 1){
                    txt << "\n    ";
                }
            }
            txt << "\n  ]\n";
            txt << "}\n\n";
            
            cout << " OK (" << m.quantidadePrimos << " primos)" << endl;
        }
        
        long long somaTotal = 0;
        for (const auto& m : medicoes) {
            somaTotal += m.tempoTotal;
        }
        
        double mediaTempo = somaTotal / (double)numMedicoes;
        
        csv << tamanho << ",MEDIA,"
            << fixed << setprecision(2) << mediaTempo << ","
            << medicoes[0].quantidadePrimos << "\n\n";
        
        txt << "MÉDIA das " << numMedicoes << " medições:\n";
        txt << "{\n";
        txt << "  tamanho: " << tamanho << ",\n";
        txt << fixed << setprecision(2);
        txt << "  tempoTotal_medio_us: " << mediaTempo << ",\n";
        txt << "  quantidadePrimos: " << medicoes[0].quantidadePrimos << "\n";
        txt << "}\n\n";
        
        cout << "  Tempo médio: " << mediaTempo << " us" << endl;
    }
    
    txt << "FIM DA ANÁLISE\n";
    
    csv.close();
    txt.close();
    
    cout << "\nEXPERIMENTOS CONCLUÍDOS\n";
}

#endif
