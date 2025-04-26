// ──────────────────────────────────────────────
// primality_test/primality_test.h
// ──────────────────────────────────────────────
#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include "prng.h"

class PRNG; // fwd‑decl
using BigInt = boost::multiprecision::cpp_int;

/**
 * @brief Abstract probabilistic primality test.
 */
class PrimalityTest
{

protected:
    // Gera testemunha a tal que 2 ≤ a ≤ n-2
    /* -------------------------------------------------------------------------
       Gera testemunha uniforme em [2, n-2] usando PRNG de 32 bits.
       ------------------------------------------------------------------------- */
    [[nodiscard]] BigInt generateWitness(const BigInt &n, PRNG &prng)
    {
        if (n <= 3)
            throw std::invalid_argument("n deve ser maior que 3.");

        BigInt witness = 0;
        const unsigned numBits = boost::multiprecision::msb(n) + 1;
        const unsigned bitsPerCall = 32;

        while (witness < 2)
        { // Garante 2 ≤ witness
            unsigned bitsGerados = 0;
            witness = 0;

            while (bitsGerados < numBits)
            {
                const uint_fast32_t rnd = prng.generate();
                const unsigned bitsRestantes = numBits - bitsGerados;
                const unsigned bitsParaPegar = std::min(bitsPerCall, bitsRestantes);

                const uint_fast32_t mask =
                    bitsParaPegar == 32 ? 0xFFFFFFFFu
                                        : ((1u << bitsParaPegar) - 1u);

                witness |= BigInt(rnd & mask) << bitsGerados;
                bitsGerados += bitsParaPegar;
            }
            witness %= n;
        }
        return witness;
    }

    void decompose(const BigInt &nMinusOne,
                   unsigned &r,
                   BigInt &d) noexcept
    {
        r = 0;
        d = nMinusOne;

        while ((d & 1) == 0)
        {            // Enquanto d for par
            d >>= 1; // divide por 2
            ++r;     // conta quantas potências de 2 extraímos
        }
    }

public:
    virtual ~PrimalityTest() = default;
    [[nodiscard]] virtual bool isPrime(const BigInt &n,
                                       int iterations,
                                       PRNG &rng) = 0;
};
