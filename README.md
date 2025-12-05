# Crivo de Eratóstenes - Implementação Distribuída

Implementação paralela do algoritmo Crivo de Eratóstenes usando MPI (Message Passing Interface) para encontrar números primos de forma eficiente.

## 📋 Descrição

Este projeto contém duas implementações do Crivo de Eratóstenes:

- **Singular**: versão sequencial clássica para benchmark
- **Distribuído**: versão paralela usando MPI que distribui o trabalho entre múltiplos processos

### Algoritmo

O Crivo de Eratóstenes é um algoritmo eficiente para encontrar todos os números primos até um valor N:

1. **Fase sequencial (rank 0)**: calcula os "primos base" até √N usando o crivo clássico
2. **Broadcast**: distribui os primos base para todos os processos via `MPI_Bcast`
3. **Fase paralela**: cada processo marca compostos em sua fatia do intervalo [2, N]
4. **Redução**: agrega contagens e listas de primos encontrados

#### Por que p² ?

O algoritmo usa `p*p` como ponto de partida porque:
- Múltiplos menores (2p, 3p, ..., (p-1)p) já foram eliminados por primos anteriores
- Só é necessário testar primos até √N para eliminar todos os compostos até N
- A condição `p*p <= limite` é equivalente a `p <= √limite` sem calcular raiz quadrada a cada iteração

## 🛠️ Requisitos

- **Compilador C++11** ou superior
- **MPI** (OpenMPI ou MPICH)
- **Make**

### Instalação das dependências (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential libopenmpi-dev openmpi-bin
```

## 🚀 Compilação

```bash
# Compilar ambas as versões
make

# Compilar apenas a versão singular
make singular

# Compilar apenas a versão distribuída
make distribuido

# Limpar executáveis
make clean
```

## 📖 Uso

### Versão Singular

```bash
./singular <qtd> <t1> [t2 ...]
```

**Exemplo:**
```bash
./singular 3 100 1000 10000
```

**Saída:**
```
CRIVO DE ERATÓSTENES - SINGULAR
Tamanhos: 100 1000 10000 

Tamanho 100: 25 primos em 0.000023 s
Tamanho 1000: 168 primos em 0.000145 s
Tamanho 10000: 1229 primos em 0.001834 s
```

### Versão Distribuída

```bash
mpirun -np <num_processos> ./distribuido <qtd> <t1> [t2 ...]
```

**Exemplos:**

```bash
# Com 4 processos
mpirun -np 4 ./distribuido 3 100 1000 10000

# Com 8 processos para tamanhos maiores
mpirun -np 8 ./distribuido 2 1000000 10000000
```

**Saída:**
```
Tamanho 100: 25 primos, MediaTotal=0.000234s MediaParal=0.000089s MediaSeq=0.000012s
Tamanho 1000: 168 primos, MediaTotal=0.000456s MediaParal=0.000234s MediaSeq=0.000034s
Tamanho 10000: 1229 primos, MediaTotal=0.002134s MediaParal=0.001456s MediaSeq=0.000089s
```

### Parâmetros

- `<qtd>`: quantidade de tamanhos a testar
- `<t1> [t2 ...]`: valores de N (limite superior para buscar primos)

## 📊 Estrutura do Código

### `singular.cpp`

Implementação sequencial simples:
- Usa `vector<char>` para marcar primos
- Mede tempos total, sequencial e paralelizável
- Retorna lista completa de primos encontrados

### `distribuido.cpp`

Implementação paralela com MPI:

#### Estrutura `Medicao`
```cpp
struct Medicao {
    long long tempoTotal;
    long long tempoParalelizavel;
    long long tempoSequencial;
    int quantidadePrimos;
    vector<int> primos;
    int numProcessos;
};
```

#### Função `crivoDeEratostenes(int n)`

1. **Geração dos primos base (rank 0)**
   ```cpp
   int limite = (int)floor(sqrt((double)n + 1.0));
   // Crivo clássico até limite
   ```

2. **Broadcast dos primos base**
   ```cpp
   MPI_Bcast(&tamPrimosBase, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(primosBase.data(), tamPrimosBase, MPI_INT, 0, MPI_COMM_WORLD);
   ```

3. **Particionamento do trabalho**
   ```cpp
   long long chunk = (total_numbers + size - 1) / size;
   long long inicio_local = start_index + rank * chunk;
   long long fim_local = min(inicio_local + chunk, end_index + 1LL);
   ```

4. **Marcação local de compostos**
   ```cpp
   for (int primo : primosBase) {
       long long start = max((long long)p*p,
                             ((inicio_local + p - 1) / p) * p);
       for (long long i = start; i < fim_local; i += p)
           isPrimo_local[i - inicio_local] = false;
   }
   ```

5. **Agregação dos resultados**
   ```cpp
   MPI_Reduce(&primos_locais, &total_primos, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
   MPI_Gatherv(...);  // Coleta as listas de primos
   ```

## 🎯 Medições de Desempenho

A versão distribuída realiza 5 medições para cada tamanho e calcula as médias:

- **MediaTotal**: tempo total de execução (incluindo comunicação MPI)
- **MediaParal**: tempo da fase paralelizável (marcação de compostos)
- **MediaSeq**: tempo da fase sequencial (geração dos primos base)

## 🔍 Exemplos de Uso Avançado

### Teste de escalabilidade

```bash
# Testar com diferentes números de processos
for np in 1 2 4 8 16; do
    echo "=== $np processos ==="
    mpirun -np $np ./distribuido 1 10000000
done
```

### Teste de tamanhos crescentes

```bash
mpirun -np 8 ./distribuido 5 100000 500000 1000000 5000000 10000000
```

## 📝 Detalhes de Implementação

### Otimizações

- **vector<char>** em vez de `vector<bool>` para melhor desempenho
- Coleta de primos apenas para N ≤ 10000 (evita overhead de memória)
- Uso de `long long` para suportar valores grandes de N
- Cálculo eficiente do primeiro múltiplo: `((inicio_local + p - 1) / p) * p`

### Comunicação MPI

- **MPI_Bcast**: distribui primos base e parâmetros
- **MPI_Reduce**: soma contagens (MPI_SUM) e encontra tempos máximos (MPI_MAX)
- **MPI_Gather/Gatherv**: coleta listas de primos de todos os processos
- **MPI_Barrier**: sincroniza antes de finalizar

## 🐛 Tratamento de Erros

- Validação de argumentos no rank 0
- `MPI_Abort` para erros fatais
- Guards para evitar acesso fora dos limites (`limite >= 0`, `limite >= 1`)

## 📚 Referências

- [Crivo de Eratóstenes - Wikipedia](https://pt.wikipedia.org/wiki/Crivo_de_Eratóstenes)
- [MPI Documentation](https://www.open-mpi.org/doc/)
- [Tutorial MPI](https://mpitutorial.com/)

## 👥 Autor

Eros Lunardon, Victor Vechi, Igor Souza

## 📄 Licença

Este projeto está sob licença aberta para fins educacionais.
