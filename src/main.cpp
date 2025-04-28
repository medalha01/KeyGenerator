/*──────────────────────────────────────────────────────────────
 *  Benchmarks:
 *    • Geração de grandes primos (com múltiplas repetições e média)
 *    • Divergências Miller–Rabin × Fermat em inteiros pequenos
 *    • Números de Carmichael
 *──────────────────────────────────────────────────────────────*/
#include "key_generator.h"
#include "pseudo_rng/mersenne_twister.h"
#include "pseudo_rng/naor_reingold_prf.h"
#include "primality_test/fermat_test.h"
#include "primality_test/miller_rabin_test.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <map>
#include <cmath>
#include <limits>

using BigInt = boost::multiprecision::cpp_int;
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

using PrngFactory = std::function<std::unique_ptr<PRNG>()>;

static PrngFactory makeFactory(const std::string &tag, uint32_t initialSeed = 0)
{
    if (tag == "MT")
        return [initialSeed]
        { return std::make_unique<MersenneTwister>(initialSeed); };
    if (tag == "NRPRF")
        return [initialSeed]
        { return std::make_unique<NaorReingoldPRF>(initialSeed); };
    throw std::invalid_argument("Unknown PRNG tag: " + tag);
}

static BigInt generateNBitOdd(unsigned bits, PRNG &prng)
{
    if (bits == 0)
        return 0;
    if (bits > 100000)
        throw std::out_of_range("Bit size too large for practical generation"); // Safety limit

    const unsigned numChunks = static_cast<unsigned>(std::ceil(static_cast<double>(bits) / 32.0));
    BigInt result = 0;
    for (unsigned i = 0; i < numChunks; ++i)
    {
        result <<= 32;
        result |= prng.generate();
    }

    // Mask to get exactly N bits
    BigInt mask = (BigInt(1) << bits) - 1;
    result &= mask;

    // Ensure the number has the desired bit length (set MSB)
    result |= (BigInt(1) << (bits - 1));

    // Ensure it's odd
    result |= 1;

    return result;
}

// Função auxiliar para gerar um primo
static std::pair<BigInt, double>
generatePrime(unsigned bits,
              uint32_t seed,
              PrimalityTest &tester,
              const PrngFactory &factory)
{
    // Cria um PRNG base que será clonado pelo KeyGenerator
    // A posse é transferida para o KeyGenerator
    KeyGenerator generator(factory(), &tester, bits);
    auto start = Clock::now();
    // A 'seed' é usada internamente pelo KeyGenerator para semear os clones
    BigInt prime = generator.generateKeyConcurrent(seed);
    double ms = Duration(Clock::now() - start).count();
    return {prime, ms};
}

// Função auxiliar para testar primalidade
static bool testWithPRNG(const BigInt &n,
                         PrimalityTest &tester,
                         const PrngFactory &factory,
                         int witnessIterations = 10)
{
    auto prng = factory();
    uint32_t derivedSeed = static_cast<uint32_t>(n & 0xFFFFFFFFu);
    if (derivedSeed == 0)
        derivedSeed = 1; // Evita semente 0
    prng->setSeed(derivedSeed);
    return tester.isPrime(n, witnessIterations, *prng); // 10 iterações padrão
}

// Benchmark 1: PRNG Generation Speed
static void runPrngGenerationBenchmark()
{
    std::cout << "\n"
              << std::string(60, '=') << "\n";
    std::cout << "   BENCHMARK: PRNG GENERATION SPEED\n";
    std::cout << "   (Time to generate 1.000 N-bit numbers)\n";
    std::cout << std::string(60, '=') << "\n";

    const std::vector<unsigned> bitSizes =
        {40, 56, 80, 128, 168, 224, 256, 512, 1024, 2048, 4096};
    const int numIntegersToGenerate = 1000;
    const int numBenchmarkReps = 10;
    const uint32_t baseSeed = 0xBEEFCAFE;

    std::cout << " PRNG | Bits | Avg Time / Batch (ms)\n";
    std::cout << "------|------|----------------------\n";

    for (const std::string &prngTag : {"MT", "NRPRF"})
    {

        for (unsigned bits : bitSizes)
        {
            double totalTime = 0.0;
            // Use a consistent factory for this size
            PrngFactory factory = makeFactory(prngTag, baseSeed + bits);

            for (int rep = 0; rep < numBenchmarkReps; ++rep)
            {
                auto prng = factory();
                prng->setSeed(baseSeed + bits + rep);
                auto start = Clock::now();
                for (int i = 0; i < numIntegersToGenerate; ++i)
                {
                    [[maybe_unused]] volatile BigInt temp = generateNBitOdd(bits, *prng);
                }
                totalTime += Duration(Clock::now() - start).count();
            }

            double avgTime = totalTime / numBenchmarkReps;

            std::cout << std::setw(5) << prngTag << " | "
                      << std::setw(4) << bits << " | "
                      << std::setw(20) << std::fixed << std::setprecision(4) << avgTime << '\n';
        }
        std::cout << "------|------|----------------------\n";
    }
}

static void runBenchmarks(const std::string &prngTag)
{
    PrngFactory factory = makeFactory(prngTag);
    MillerRabinTest miller;
    FermatTest fermat;

    const uint32_t baseSeed = 0xA5A5A5A5u;

    // --- Seção A: Geração de Grandes Primos (Modificada) ---

    // Mapeamento de tamanho de bits para número de repetições
    const std::map<unsigned, int> repetitionsMap = {
        {40, 1000}, {56, 500}, {80, 200}, {128, 100}, {168, 50}, {224, 25}, {256, 15}, {512, 10}, {1024, 8}, {2048, 5}, {4096, 2}};
    // Vetor de tamanhos de bits a testar (ordenado pelo map implicitamente, mas podemos definir explicitamente)
    const std::vector<unsigned> bitSizes =
        {40, 56, 80, 128, 168, 224, 256, 512, 1024, 2048, 4096};

    std::cout << "\n=== PRNG: " << prngTag << " — Geração de Grandes Primos (Média de Repetições) ===\n";
    if (prngTag == "NRPRF")
    {
        std::cout << "*** Aviso: Testes com NRPRF, especialmente para bits >= 512, podem ser MUITO lentos! ***\n";
    }
    std::cout << " Bits | Reps | Alg | Média (ms) | Último Prefixo\n";
    std::cout << "------|------|-----|------------|-----------------\n";

    for (unsigned bits : bitSizes)
    {
        int repetitions = 1; // Valor padrão caso não encontre no map
        auto it = repetitionsMap.find(bits);
        if (it != repetitionsMap.end())
        {
            repetitions = it->second;
        }
        else
        {
            std::cerr << "Aviso: Número de repetições não definido para " << bits << " bits. Usando 1.\n";
        }

        double totalTimeMR = 0.0;
        double totalTimeFT = 0.0;
        BigInt lastPrimeMR = 0; // Para guardar o último primo gerado para o prefixo
        BigInt lastPrimeFT = 0;

        for (int i = 0; i < repetitions; ++i)
        {
            // Gera sementes diferentes para cada repetição e cada teste
            uint32_t seedMR = baseSeed + bits + i;
            uint32_t seedFT = baseSeed + bits + i + (repetitions * 10); // Garante sementes distintas para FT

            auto [pMR, tMR] = generatePrime(bits, seedMR, miller, factory);
            auto [pFT, tFT] = generatePrime(bits, seedFT, fermat, factory);

            totalTimeMR += tMR;
            totalTimeFT += tFT;

            // Guarda o último primo gerado em cada categoria
            if (i == repetitions - 1)
            {
                lastPrimeMR = pMR;
                lastPrimeFT = pFT;
            }
        }

        double avgTimeMR = (repetitions > 0) ? totalTimeMR / repetitions : 0.0;
        double avgTimeFT = (repetitions > 0) ? totalTimeFT / repetitions : 0.0;

        auto prefix64 = [bits](const BigInt &n)
        {
            std::ostringstream oss;
            unsigned shift = (bits > 64) ? bits - 64 : 0;
            oss << std::hex << (n >> shift);
            return "0x" + oss.str(); // Adiciona "0x" aqui
        };

        // Imprime a linha para Miller-Rabin
        std::cout << std::setw(5) << bits << " | "
                  << std::setw(4) << repetitions << " | MR  | "
                  << std::setw(10) << std::fixed << std::setprecision(2) << avgTimeMR << " | "
                  << prefix64(lastPrimeMR) << '\n';

        // Imprime a linha para Fermat
        std::cout << std::setw(5) << "" << " | "       // Espaço em branco para alinhar
                  << std::setw(4) << "" << " | FT  | " // Espaço em branco para alinhar
                  << std::setw(10) << std::fixed << std::setprecision(2) << avgTimeFT << " | "
                  << prefix64(lastPrimeFT) << '\n';
        std::cout << "------|------|-----|------------|-----------------\n"; // Separador
    }

    // --- Seção B: Divergências em inteiros pequenos ---
    const std::vector<unsigned> smallBits{16, 24, 32, 256, 512};
    const int sampleCount = 1'000;
    std::mt19937 urbg(0xC0FFEE); // Gerador independente para amostras

    std::cout << "\n=== PRNG: " << prngTag << " — Divergências MR x FT ===\n";
    std::cout << " Bits | Amostras | Discordâncias\n";
    std::cout << "------|----------|--------------\n";

    for (unsigned bits : smallBits)
    {
        std::uniform_int_distribution<uint32_t> dist(
            (bits == 1) ? 1u : (1u << (bits - 1)),
            (bits == 32) ? std::numeric_limits<uint32_t>::max() : ((1u << bits) - 1u));

        int mismatches = 0;
        for (int i = 0; i < sampleCount; ++i)
        {
            BigInt n = BigInt(dist(urbg) | 1u); // Garante ímpar
            if (testWithPRNG(n, miller, factory) !=
                testWithPRNG(n, fermat, factory))
                ++mismatches;
        }
        std::cout << std::setw(5) << bits << " | "
                  << std::setw(8) << sampleCount << " | "
                  << std::setw(12) << mismatches << '\n';
    }
    std::cout << "------|----------|--------------\n";

    // --- Seção C: Carmichael ---
    const uint64_t carmichael[] = {
        561, 1105, 1729, 2465, 6601, 8911, 10585, 15841,
        29341, 41041, 46657, 52633};

    std::cout << "\n=== PRNG: " << prngTag << " — Números de Carmichael ===\n";
    std::cout << "   n   | Fermat   | MillerRabin\n";
    std::cout << "-------|----------|------------\n";

    for (uint64_t n64 : carmichael)
    {
        BigInt n = n64;
        bool ftIsPrime = testWithPRNG(n, fermat, factory);
        bool mrIsPrime = testWithPRNG(n, miller, factory);

        std::cout << std::setw(6) << n64 << " | "
                  << std::setw(8) << (ftIsPrime ? "primo" : "composto") << " | "  // Ajustado setw
                  << std::setw(10) << (mrIsPrime ? "primo" : "composto") << '\n'; // Ajustado setw
    }
    std::cout << "-------|----------|------------\n";
}

// --- main ---
int main()
{
    try
    {
        std::cout << "Starting Benchmarks...\n";

        runPrngGenerationBenchmark();

        runBenchmarks("MT");
        runBenchmarks("NRPRF");

        std::cout << "\nBenchmarks Completetada.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "[ERROR] unknown exception\n";
        return 1;
    }
    return 0;
}