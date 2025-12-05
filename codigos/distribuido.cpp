#include <iostream>
#include <vector>
#include <mpi.h>
#include <cmath>
#include <iomanip>

using namespace std;

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

    // --- Timers MPI ---
    double t0_total = MPI_Wtime();
    double t0_seq   = MPI_Wtime();

    // --- Primos base ---
    int limite = (int)floor(sqrt((double)n + 1.0));
    vector<int> primosBase;
    if (rank == 0) {
        vector<char> eh_primo(limite + 1, true);
        if (limite >= 0) eh_primo[0] = false;
        if (limite >= 1) eh_primo[1] = false;

        for (int p = 2; p * p <= limite; p++) {
            if (eh_primo[p]) {
                for (int i = p * p; i <= limite; i += p)
                    eh_primo[i] = false;
            }
        }
        for (int i = 2; i <= limite; i++)
            if (eh_primo[i]) primosBase.push_back(i);
    }

    int tamPrimosBase = primosBase.size();
    MPI_Bcast(&tamPrimosBase, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) primosBase.resize(tamPrimosBase);
    if (tamPrimosBase > 0)
        MPI_Bcast(primosBase.data(), tamPrimosBase, MPI_INT, 0, MPI_COMM_WORLD);

    double t1_seq = MPI_Wtime();

    // --- Paralelo ---
    double t0_par = MPI_Wtime();

    long long start_index = 2;
    long long end_index   = n;
    long long total_numbers = max(0LL, end_index - start_index + 1);

    long long chunk = (total_numbers + size - 1) / size;
    long long inicio_local = start_index + rank * chunk;
    long long fim_local    = min(inicio_local + chunk, end_index + 1LL);

    long long local_count = max(0LL, fim_local - inicio_local);
    vector<char> isPrimo_local(local_count, true);

    for (int primo : primosBase) {
        long long p = primo;
        long long start = max((long long)p*p,
                              ((inicio_local + p - 1) / p) * p);
        for (long long i = start; i < fim_local; i += p)
            isPrimo_local[i - inicio_local] = false;
    }

    int primos_locais = 0;
    vector<int> primos_encontrados;
    if (n <= 10000) primos_encontrados.reserve(local_count);

    for (long long i = inicio_local; i < fim_local; i++) {
        if (isPrimo_local[i - inicio_local]) {
            primos_locais++;
            if (n <= 10000) primos_encontrados.push_back(i);
        }
    }

    int total_primos = 0;
    MPI_Reduce(&primos_locais, &total_primos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // --- Coleta dos vetores ---
    int meu_count = primos_encontrados.size();
    vector<int> counts(size);
    MPI_Gather(&meu_count, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> todos_primos;
    vector<int> displs;
    int total_elems = 0;

    if (rank == 0) {
        displs.resize(size);
        for (int i = 0; i < size; i++) {
            displs[i] = total_elems;
            total_elems += counts[i];
        }
        todos_primos.resize(total_elems);
    }

    MPI_Gatherv(
        primos_encontrados.data(), meu_count, MPI_INT,
        (rank == 0 ? todos_primos.data() : nullptr),
        (rank == 0 ? counts.data() : nullptr),
        (rank == 0 ? displs.data() : nullptr),
        MPI_INT,
        0, MPI_COMM_WORLD
    );

    double t1_par = MPI_Wtime();
    double t1_total = MPI_Wtime();

    // --- Redução dos tempos usando MAX ---
    double dur_total_local = t1_total - t0_total;
    double dur_seq_local   = t1_seq - t0_seq;
    double dur_par_local   = t1_par - t0_par;

    double max_total, max_seq, max_par;
    MPI_Reduce(&dur_total_local, &max_total, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&dur_seq_local,   &max_seq,   1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&dur_par_local,   &max_par,   1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        medicao.tempoTotal        = (long long)(max_total * 1e6);
        medicao.tempoSequencial   = (long long)(max_seq   * 1e6);
        medicao.tempoParalelizavel = (long long)(max_par  * 1e6);
        medicao.quantidadePrimos = total_primos;
        medicao.primos = std::move(todos_primos);
        medicao.numProcessos = size;
    }

    return medicao;
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int qtd = 0;
    vector<int> tamanhos;
    int numMedicoes = 5;

    if (rank == 0) {
        if (argc < 3) {
            cerr << "Uso: " << argv[0] << " <qtd> <t1> [t2 ...]\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        qtd = atoi(argv[1]);
        tamanhos.resize(qtd);
        for (int i = 0; i < qtd; i++)
            tamanhos[i] = atoi(argv[2 + i]);
    }

    MPI_Bcast(&qtd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) tamanhos.resize(qtd);
    MPI_Bcast(tamanhos.data(), qtd, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        for (int t : tamanhos) {
            vector<Medicao> ms;
            for (int i = 0; i < numMedicoes; i++)
                ms.push_back(crivoDeEratostenes(t));

            long long somaT = 0, somaP = 0, somaS = 0;
            for (auto& m : ms) {
                somaT += m.tempoTotal;
                somaP += m.tempoParalelizavel;
                somaS += m.tempoSequencial;
            }

            double mediaTotal = (somaT / numMedicoes) / 1000000.0;
            double mediaParal = (somaP / numMedicoes) / 1000000.0;
            double mediaSeq = (somaS / numMedicoes) / 1000000.0;

            cout << "Tamanho " << t << ": "
                 << ms[0].quantidadePrimos << " primos, "
                 << fixed << setprecision(6)
                 << "MediaTotal=" << mediaTotal << "s "
                 << "MediaParal=" << mediaParal << "s "
                 << "MediaSeq=" << mediaSeq << "s" << endl;
        }
    } else {
        for (int t : tamanhos)
            for (int i = 0; i < numMedicoes; i++)
                crivoDeEratostenes(t);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}