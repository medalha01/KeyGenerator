// ──────────────────────────────────────────────────────────────────────────────
// main.cpp  —  Experimentos completos:
//   1) geração de primos (40 … 4096 bits)
//   2) comparação MR × Fermat em números pequenos
//   3) teste de Carmichael (pseudo-primos de Fermat)
// ──────────────────────────────────────────────────────────────────────────────
#include "key_generator.h"
#include "pseudo_rng/mersenne_twister.h"
#include "primality_test/miller_rabin_test.h"
#include "primality_test/fermat_test.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
using RNG32 = std::mt19937; // PRNG rápido p/ testes

/*-----------------------------------------------------------------------------
   Gera UM primo de ‘bits’ usando tester; devolve par (prime, tempoMs)
 ---------------------------------------------------------------------------*/
static std::pair<BigInt, double>
generatePrime(unsigned bits, uint_fast32_t seed, PrimalityTest &tester)
{
    auto prng = std::make_unique<MersenneTwister>(seed);
    KeyGenerator kg(std::move(prng), &tester, bits);

    const auto t0 = Clock::now();
    BigInt prime = kg.generateKeyConcurrent(seed);
    double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0)
                    .count();
    return {prime, ms};
}

/*-----------------------------------------------------------------------------
   Seção 1 – tabela de primos para tamanhos grandes (Passo 2)
 ---------------------------------------------------------------------------*/
static void experimentLargePrimes()
{
    const std::vector<unsigned> bitSizes{
        40, 56, 80, 128, 168, 224, 256, 512, 1024, 2048, 4096};

    MillerRabinTest mr;
    FermatTest ft;
    constexpr uint_fast32_t seedBase = 12345;

    std::cout << "\nAlgoritmo,Bits,Número primo (hex),Tempo (ms)\n";

    for (unsigned bits : bitSizes)
    {
        std::cout << "\n=== " << bits << " bits ===\n";

        auto [p1, t1] = generatePrime(bits, seedBase + bits, mr);
        auto [p2, t2] = generatePrime(bits, seedBase + bits + 1, ft);

        std::cout << "Miller–Rabin," << bits << ",0x"
                  << std::hex << p1 << std::dec << ','
                  << std::fixed << std::setprecision(2) << t1 << '\n';

        std::cout << "Fermat," << bits << ",0x"
                  << std::hex << p2 << std::dec << ','
                  << std::fixed << std::setprecision(2) << t2 << '\n';
    }
}

/*-----------------------------------------------------------------------------
   Seção 2 – compara respostas MR × Fermat em números de 16/24/32 bits
   • Gera 1 000 ímpares aleatórios de cada tamanho
   • Conta discordâncias (Fermat diz “primo”, MR diz “composto” ou vice-versa)
 ---------------------------------------------------------------------------*/
static void experimentSmallDisagreements()
{
    MillerRabinTest mr;
    FermatTest ft;
    RNG32 rng(0xBEEF);
    const std::vector<unsigned> smallBits{16, 24, 32};
    constexpr int samplesPerSize = 1000;

    std::cout << "\nBits,Total testados,Discordâncias\n";

    for (unsigned bits : smallBits)
    {
        std::uniform_int_distribution<uint32_t> dist(
            1u << (bits - 1), (1u << bits) - 1);

        int disagreements = 0;

        for (int i = 0; i < samplesPerSize; ++i)
        {
            uint32_t n = dist(rng) | 1u; // garante ímpar
            BigInt candidate = n;

            bool isPrimeMR = mr.isPrime(candidate, 10, *(PRNG *)nullptr);
            bool isPrimeFT = ft.isPrime(candidate, 10, *(PRNG *)nullptr);

            if (isPrimeMR != isPrimeFT)
                ++disagreements;
        }
        std::cout << bits << ',' << samplesPerSize << ',' << disagreements
                  << '\n';
    }
}

/*-----------------------------------------------------------------------------
   Seção 3 – testa números de Carmichael (561, 1105, 1729, …)
   • Fermat precisa falhar (retorna “primo”)
   • Miller–Rabin deve detectar “composto”
 ---------------------------------------------------------------------------*/
static void experimentCarmichael()
{
    const std::vector<uint64_t> carmichael{
        561, 1105, 1729, 2465, 6601, 8911,
        10585, 15841, 29341, 41041, 46657, 52633};

    MillerRabinTest mr;
    FermatTest ft;
    std::cout << "\nNúmero, Fermat, Miller–Rabin\n";

    for (uint64_t n : carmichael)
    {
        BigInt x = n;
        bool pF = ft.isPrime(x, 10, *(PRNG *)nullptr);
        bool pMR = mr.isPrime(x, 10, *(PRNG *)nullptr);

        std::cout << n << ", "
                  << (pF ? "primo " : "composto") << ", "
                  << (pMR ? "primo " : "composto") << '\n';
    }
}

/*-----------------------------------------------------------------------------
   Ponto de entrada: roda as três seções
 ---------------------------------------------------------------------------*/
int main()
{
    experimentLargePrimes();        // Passo 2 original
    experimentSmallDisagreements(); // comparações MR × Fermat
    experimentCarmichael();         // pseudo-primos clássicos

    return 0;
}
