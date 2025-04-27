#pragma once
/*──────────────────────────────────────────────────────────────
 *  Classe-base para testes probabilísticos de primalidade.
 *──────────────────────────────────────────────────────────────*/
#include <boost/multiprecision/cpp_int.hpp>
#include "prng.h"
#include <algorithm>
#include <stdexcept>

using BigInt = boost::multiprecision::cpp_int;

class PrimalityTest
{
protected:
    /** Gera witness uniforme no intervalo [2, modulusUnderTest-2].           */
    BigInt generateWitness(const BigInt& modulusUnderTest, PRNG& prng)
    {
        if (modulusUnderTest <= 3)
            throw std::invalid_argument("modulusUnderTest must be > 3");

        /* Tamanho do intervalo interno: [0, modulus-4]  ⇒  modulus-3 valores */
        const BigInt intervalSize = modulusUnderTest - 3;
        const unsigned requiredBits =
            boost::multiprecision::msb(intervalSize) + 65;      // +64 ⇢ menos viés

        const unsigned bitsPerPRNGCall = 32;
        BigInt rawRandomValue{0};
        unsigned bitsCollected = 0;

        while (bitsCollected < requiredBits)
        {
            uint32_t prngChunk = prng.generate();
            unsigned bitsToTake =
                std::min(bitsPerPRNGCall, requiredBits - bitsCollected);
            uint32_t mask =
                (bitsToTake == 32) ? 0xFFFFFFFFu : ((1u << bitsToTake) - 1u);
            rawRandomValue |= BigInt(prngChunk & mask) << bitsCollected;
            bitsCollected  += bitsToTake;
        }
        /* Mapear para [2, modulus-2] */
        return 2 + (rawRandomValue % intervalSize);
    }

    /** Decompõe  n-1 = oddComponent · 2^powerOfTwoExponent,  oddComponent ímpar. */
    void decompose(const BigInt& nMinusOne,
                   unsigned&      powerOfTwoExponent,
                   BigInt&        oddComponent) noexcept
    {
        oddComponent        = nMinusOne;
        powerOfTwoExponent  = 0;
        while ((oddComponent & 1) == 0) { oddComponent >>= 1; ++powerOfTwoExponent; }
    }

public:
    virtual ~PrimalityTest() = default;

    virtual bool isPrime(const BigInt& modulusUnderTest,
                         int           witnessIterations,
                         PRNG&         randomGenerator) = 0;
};
