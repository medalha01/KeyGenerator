/*──────────────────────────────────────────────────────────────
 *  Benchmarks:
 *    • Geração de grandes primos
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

using BigInt = boost::multiprecision::cpp_int;
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

using PrngFactory = std::function<std::unique_ptr<PRNG>()>;

static PrngFactory makeFactory(const std::string &tag)
{
    if (tag == "MT")
        return []
        { return std::make_unique<MersenneTwister>(0); };
    if (tag == "NRPRF")
        return []
        { return std::make_unique<NaorReingoldPRF>(0); };
    throw std::invalid_argument("Unknown PRNG tag");
}

static std::pair<BigInt, double>
generatePrime(unsigned bits,
              uint32_t seed,
              PrimalityTest &tester,
              const PrngFactory &factory)
{
    KeyGenerator generator(factory(), &tester, bits);
    auto start = Clock::now();
    BigInt prime = generator.generateKeyConcurrent(seed);
    double ms = Duration(Clock::now() - start).count();
    return {prime, ms};
}

static bool testWithPRNG(const BigInt &n,
                         PrimalityTest &tester,
                         const PrngFactory &factory)
{
    auto prng = factory();
    uint32_t derivedSeed = static_cast<uint32_t>(n & 0xFFFFFFFFu);
    if (derivedSeed == 0)
        derivedSeed = 1;
    prng->setSeed(derivedSeed);
    return tester.isPrime(n, 10, *prng);
}

static void runBenchmarks(const std::string &prngTag)
{
    PrngFactory factory = makeFactory(prngTag);
    MillerRabinTest miller;
    FermatTest fermat;

    const uint32_t baseSeed = 0xA5A5A5A5u;
    const std::vector<unsigned> bitSizes =
        {40, 56, 80, 128, 168, 224, 256, 512, 1024, 2048, 4096};

    std::cout << "\n=== PRNG: " << prngTag << " — grandes primos ===\n";
    std::cout << "bits | alg  | tempo(ms) | prefix\n";

    for (unsigned bits : bitSizes)
    {
        auto [pMR, tMR] = generatePrime(bits, baseSeed + bits, miller, factory);
        auto [pFT, tFT] = generatePrime(bits, baseSeed + bits + 1u, fermat, factory);

        auto prefix64 = [bits](const BigInt &n)
        {
            std::ostringstream oss;
            unsigned shift = (bits > 64) ? bits - 64 : 0;
            oss << std::hex << (n >> shift);
            return oss.str();
        };

        std::cout << std::setw(4) << bits
                  << " | MR | " << std::setw(9) << std::fixed << std::setprecision(2) << tMR
                  << " | 0x" << prefix64(pMR) << '\n';

        std::cout << std::setw(4) << bits
                  << " | FT | " << std::setw(9) << tFT
                  << " | 0x" << prefix64(pFT) << '\n';
    }

    /* Divergências em inteiros pequenos */
    const std::vector<unsigned> smallBits{16, 24, 32};
    const int sampleCount = 1'000;
    std::mt19937 urbg(0xC0FFEE);

    std::cout << "\n=== Divergências MR × FT ===\n";
    std::cout << "bits | amostras | mismatches\n";

    for (unsigned bits : smallBits)
    {
        std::uniform_int_distribution<uint32_t> dist(
            (bits == 1) ? 1u : (1u << (bits - 1)),
            (bits == 32) ? std::numeric_limits<uint32_t>::max() : ((1u << bits) - 1u));

        int mismatches = 0;
        for (int i = 0; i < sampleCount; ++i)
        {
            BigInt n = BigInt(dist(urbg) | 1u);
            if (testWithPRNG(n, miller, factory) !=
                testWithPRNG(n, fermat, factory))
                ++mismatches;
        }
        std::cout << std::setw(4) << bits << " | "
                  << std::setw(9) << sampleCount << " | "
                  << mismatches << '\n';
    }

    /* Carmichael */
    const uint64_t carmichael[] = {
        561, 1105, 1729, 2465, 6601, 8911, 10585, 15841,
        29341, 41041, 46657, 52633};

    std::cout << "\n=== Carmichael ===\n";
    std::cout << "n     | Fermat | MillerRabin\n";

    for (uint64_t n64 : carmichael)
    {
        BigInt n = n64;
        bool ftIsPrime = testWithPRNG(n, fermat, factory);
        bool mrIsPrime = testWithPRNG(n, miller, factory);

        std::cout << std::setw(6) << n64 << " | "
                  << (ftIsPrime ? "prime  " : "comp.") << " | "
                  << (mrIsPrime ? "prime" : "comp.") << '\n';
    }
}

int main()
{
    try
    {
        runBenchmarks("MT");
        runBenchmarks("NRPRF");
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "[ERROR] unknown\n";
        return 1;
    }
    return 0;
}
