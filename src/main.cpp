#include "key_generator.h"
#include "pseudo_rng/mersenne_twister.h"
#include "pseudo_rng/blum_blum_shub.h"
#include "primality_test/fermat_test.h"
#include <iomanip>
#include <chrono>
#include <iostream>

using Clock = std::chrono::high_resolution_clock;

void benchmarkPrng(
    const std::string& name,
    std::function<std::unique_ptr<PRNG>()> makePrng,
    unsigned keyBits,
    int runs,
    uint_fast32_t seedBase)
{
    FermatTest fermat;
    std::cout << "\n=== " << name << " / " << keyBits << " bits ===\n";

    double totalMs = 0.0;

    for (int i = 0; i < runs; ++i) {
        auto prng = makePrng();
        KeyGenerator kg(std::move(prng), &fermat, keyBits);

        const auto start = Clock::now();
        kg.generateKeyConcurrent(seedBase + i);
        const double elapsed =
            std::chrono::duration<double, std::milli>(Clock::now() - start).count();

        std::cout << "Run " << i << ": " << elapsed << " ms\n";
        totalMs += elapsed;
    }

    std::cout << "Média: "
              << std::fixed << std::setprecision(2)
              << (totalMs / runs) << " ms\n";
}

int main()
{
    const int runs = 5;
    benchmarkPrng("Mersenne Twister",
                  [](){ return std::make_unique<MersenneTwister>(0); },
                  1024, runs, 12345u);

    benchmarkPrng("Blum-Blum-Shub",
                  [](){ return std::make_unique<BlumBlumShub>(0); },
                  1024, runs, 67891u);

    return 0;
}

