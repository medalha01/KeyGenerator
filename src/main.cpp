/*─────────────────────────────────────────────────────────────────────────────
 *  main.cpp
 *
 *  Executa **três** baterias de testes, para **dois** PRNGs (MT e BBS) e
 *  **dois** testes de primalidade (Miller-Rabin × Fermat):
 *
 *    1.  Geração de grandes primos (40 – 4096 bits) com tempo cronometrado.
 *    2.  Amostras aleatórias de 16/24/32 bits – conta discrepâncias MR × Fermat.
 *    3.  Números de Carmichael – casos clássicos onde Fermat falha.
 *
 *  A saída é sempre a mesma estrutura, apenas mudando o rótulo do PRNG,
 *  facilitando redirecionar para arquivo ou filtrar via scripts.
 *────────────────────────────────────────────────────────────────────────────*/

#include "key_generator.h"
#include "pseudo_rng/mersenne_twister.h"
#include "pseudo_rng/blum_blum_shub.h"
#include "primality_test/fermat_test.h"
#include "primality_test/miller_rabin_test.h" //  supondo que exista

#include <sstream>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

using BigInt = boost::multiprecision::cpp_int;
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

/*─────────────────────────────────────────────────────────────────────────────
 *  1.  Helpers para criar PRNGs a partir de string
 *───────────────────────────────────────────────────────────────────────────*/
using PrngFactory = std::function<std::unique_ptr<PRNG>()>;

static PrngFactory makeFactory(const std::string &label)
{
    if (label == "MT")
        return []
        { return std::make_unique<MersenneTwister>(0); };
    if (label == "BBS")
        return []
        { return std::make_unique<BlumBlumShub>(0); };
    throw std::invalid_argument("PRNG desconhecido: " + label);
}

/*─────────────────────────────────────────────────────────────────────────────
 *  2.  Gera um primo de ‘bits’  ➜  (valor, tempo-ms)
 *───────────────────────────────────────────────────────────────────────────*/
static std::pair<BigInt, double> generatePrime(unsigned bits,
                                               uint_fast32_t seed,
                                               PrimalityTest &tester,
                                               const PrngFactory &makePrng)
{
    auto prng = makePrng();
    prng->setSeed(seed);
    KeyGenerator kg(std::move(prng), &tester, bits);

    const auto t0 = Clock::now();
    BigInt prime = kg.generateKeyConcurrent(seed);
    const double ms = Duration(Clock::now() - t0).count();

    return {prime, ms};
}

/*─────────────────────────────────────────────────────────────────────────────
 *  3.  Aplica tester a n usando um PRNG independente
 *───────────────────────────────────────────────────────────────────────────*/
static bool testWithPrng(const BigInt &n,
                         PrimalityTest &tester,
                         const PrngFactory &makePrng)
{
    auto prng = makePrng();
    prng->setSeed(static_cast<uint_fast32_t>(n & 0xFFFFFFFFu)); // semente simples
    return tester.isPrime(n, 10, *prng);
}

/*─────────────────────────────────────────────────────────────────────────────
 *  4.  Bloco principal de benchmarks para UM PRNG
 *───────────────────────────────────────────────────────────────────────────*/
static void runBenchmarks(const std::string &prngLabel)
{
    /* ---------- Instâncias compartilhadas ---------- */
    PrngFactory factory = makeFactory(prngLabel);
    MillerRabinTest miller;
    FermatTest fermat;

    constexpr uint_fast32_t seedBase = 0xA5A5A5A5u;

    /*-----------------------------------------------------------------------
     *  Seção A – grandes primos
     *---------------------------------------------------------------------*/
    const std::vector<unsigned> bitSizes{
        40, 56, 80, 128, 168, 224, 256, 512, 1024, 2048, 4096};

    std::cout << "\n─────────────────────────────────────────────────────────────\n";
    std::cout << "PRNG = " << prngLabel << "   •   Geração de grandes primos\n";
    std::cout << "Bits | Algoritmo |   Tempo (ms) | Prefixo(16 hex)\n";
    std::cout << "─────┼───────────┼─────────────┼──────────────────────────────\n";

    for (unsigned bits : bitSizes)
    {
        auto [pMR, tMR] = generatePrime(bits, seedBase + bits, miller, factory);
        auto [pFT, tFT] = generatePrime(bits, seedBase + bits + 1, fermat, factory);

        const auto showPrefix = [bits](const BigInt &n) //  <-- [bits]
        {
            std::ostringstream oss;
            /* evita deslocamento negativo quando bits ≤ 64 */
            const unsigned shift = bits > 64 ? bits - 64 : 0;
            oss << std::hex << (n >> shift);
            return oss.str();
        };

        std::cout << std::setw(4) << bits << " | "
                  << "MillerRabin | " << std::setw(11) << std::fixed << std::setprecision(2)
                  << tMR << " | 0x" << showPrefix(pMR) << '\n';

        std::cout << std::setw(4) << bits << " | "
                  << "Fermat      | " << std::setw(11) << tFT << " | 0x"
                  << showPrefix(pFT) << '\n';
    }

    /*-----------------------------------------------------------------------
     *  Seção B – discrepâncias MR × Fermat em inteiros pequenos
     *---------------------------------------------------------------------*/
    const std::vector<unsigned> smallBits{16, 24, 32};
    constexpr int samples = 1'000;
    std::mt19937 urbg(0xC0FFEE);

    std::cout << "\nPRNG = " << prngLabel
              << "   •   Discrepâncias (amostras aleatórias)\n";
    std::cout << "Bits | Amostras | Discordâncias\n";
    std::cout << "─────┼──────────┼──────────────\n";

    for (unsigned bits : smallBits)
    {
        std::uniform_int_distribution<uint32_t> dist(1u << (bits - 1),
                                                     (1u << bits) - 1);

        int disagreements = 0;
        for (int i = 0; i < samples; ++i)
        {
            BigInt n = BigInt(dist(urbg) | 1u); // força ímpar
            bool p1 = testWithPrng(n, miller, factory);
            bool p2 = testWithPrng(n, fermat, factory);
            if (p1 != p2)
                ++disagreements;
        }
        std::cout << std::setw(4) << bits << " | "
                  << std::setw(8) << samples << " | "
                  << std::setw(12) << disagreements << '\n';
    }

    /*-----------------------------------------------------------------------
     *  Seção C – números de Carmichael
     *---------------------------------------------------------------------*/
    const std::vector<uint64_t> carmichael{
        561, 1105, 1729, 2465, 6601, 8911, 10585,
        15841, 29341, 41041, 46657, 52633};

    std::cout << "\nPRNG = " << prngLabel
              << "   •   Números de Carmichael\n";
    std::cout << "n     | Fermat | Miller–Rabin\n";
    std::cout << "──────┼────────┼─────────────\n";

    for (uint64_t n64 : carmichael)
    {
        BigInt n = n64;
        bool isFermatPrime = testWithPrng(n, fermat, factory);
        bool isMillerPrime = testWithPrng(n, miller, factory);

        std::cout << std::setw(6) << n64 << " | "
                  << (isFermatPrime ? "primo " : "composto") << " | "
                  << (isMillerPrime ? "primo" : "composto") << '\n';
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 *  MAIN – executa para MT e BBS
 *───────────────────────────────────────────────────────────────────────────*/
int main()
{
    try
    {
        runBenchmarks("MT");
        runBenchmarks("BBS");
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERRO] " << e.what() << '\n';
        return 1;
    }
    return 0;
}
