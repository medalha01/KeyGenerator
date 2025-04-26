#include "primality_test/fermat_test.h"
#include <boost/multiprecision/miller_rabin.hpp>
#include <stdexcept>
#include <algorithm>

using boost::multiprecision::cpp_int;
using BigInt = cpp_int;

/* -------------------------------------------------------------------------
   Gera testemunha uniforme em [2, n-2] usando PRNG de 32 bits.
   ------------------------------------------------------------------------- */
BigInt FermatTest::generateWitness(const BigInt& n, PRNG& prng)
{
    if (n <= 3)
        throw std::invalid_argument("n deve ser maior que 3.");

    BigInt witness = 0;
    const unsigned numBits = boost::multiprecision::msb(n) + 1;
    const unsigned bitsPerCall = 32;

    while (witness < 2) {                               // Garante 2 ≤ witness
        unsigned bitsGerados = 0;
        witness = 0;

        while (bitsGerados < numBits) {
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

/* -------------------------------------------------------------------------
   Teste de Fermat: a^(n-1) ≡ 1 (mod n) para várias testemunhas aleatórias.
   ------------------------------------------------------------------------- */
bool FermatTest::isPrime(const BigInt& n, int iterations, PRNG& prng)
{
    if (n <= 1)  return false;
    if (n <= 3)  return true;
    if (n % 2 == 0) return false;

    const BigInt nMinus1 = n - 1;

    for (int i = 0; i < iterations; ++i) {
        const BigInt a = generateWitness(n, prng);
        const BigInt result = boost::multiprecision::powm(a, nMinus1, n);
        if (result != 1) return false;                  // Composto
    }
    return true;                                        // Provavelmente primo
}
