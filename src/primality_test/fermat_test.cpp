/*──────────────────────────────────────────────────────────────
 *  Teste de Primalidade de Fermat
 *
 *  Teorema:  a^(n-1) ≡ 1  (mod n)  se  n  é primo e  gcd(a,n)=1.
 *  Processo:
 *    • Escolher witness   a ∈ [2, n-2]
 *    • Calcular           a^(n-1) mod n
 *    • Resultado ≠ 1  ⇒  n é composto
 *    • Repetir k vezes  ⇒  “provavelmente primo”
 *  Atenção: números de Carmichael passam para QUALQUER witness.
 *──────────────────────────────────────────────────────────────*/
#include "primality_test/fermat_test.h"
#include <boost/multiprecision/number.hpp>

using boost::multiprecision::cpp_int;
using BigInt = cpp_int;

bool FermatTest::isPrime(const BigInt& modulusUnderTest,
                         int witnessIterations,
                         PRNG& randomGenerator)
{
    /* Casos triviais */
    if (modulusUnderTest <= 1)            return false;
    if (modulusUnderTest <= 3)            return true;
    if ((modulusUnderTest & 1) == 0)      return false;         // par > 2

    const BigInt exponent = modulusUnderTest - 1;               // n-1

    for (int iteration = 0; iteration < witnessIterations; ++iteration)
    {
        //const BigInt candidateWitness =
        //    generateWitness(modulusUnderTest, randomGenerator);
        BigInt candidateWitness;
        BigInt g;
        do {
            candidateWitness = generateWitness(modulusUnderTest, randomGenerator);
            g = boost::math::gcd(candidateWitness, modulusUnderTest);
        } while (g != 1);

        /* a^(n-1) mod n */
        const BigInt modExpResult =
            boost::multiprecision::powm(candidateWitness,
                                        exponent,
                                        modulusUnderTest);

        if (modExpResult != 1)                                   // Falhou
            return false;
    }
    return true;                                                 // Provável primo
}
