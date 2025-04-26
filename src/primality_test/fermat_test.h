#pragma once
#include "primality_test.h"
#include "prng.h"

class FermatTest final : public PrimalityTest
{
private:
    // Gera testemunha a tal que 2 ≤ a ≤ n-2
    [[nodiscard]] BigInt generateWitness(const BigInt& n, PRNG& prng);

public:
    FermatTest() = default;
    ~FermatTest() override = default;

    [[nodiscard]] bool isPrime(
        const BigInt& n, int iterations, PRNG& prng) override;
};

