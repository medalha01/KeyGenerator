#include "primality_test/fermat_test.h"
#include <boost/multiprecision/miller_rabin.hpp>
#include <stdexcept>
#include <algorithm>

using boost::multiprecision::cpp_int;
using BigInt = cpp_int;

/* -------------------------------------------------------------------------
   Teste de Fermat: a^(n-1) ≡ 1 (mod n) para várias testemunhas aleatórias.
   ------------------------------------------------------------------------- */
bool FermatTest::isPrime(const BigInt &n, int iterations, PRNG &prng)
{
    if (n <= 1)
        return false;
    if (n <= 3)
        return true;
    if (n % 2 == 0)
        return false;

    const BigInt nMinus1 = n - 1;

    for (int i = 0; i < iterations; ++i)
    {
        const BigInt a = generateWitness(n, prng);
        const BigInt result = boost::multiprecision::powm(a, nMinus1, n);
        if (result != 1)
            return false; // Composto
    }
    return true; // Provavelmente primo
}
