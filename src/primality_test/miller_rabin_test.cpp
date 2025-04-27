/*──────────────────────────────────────────────────────────────
 *  Teste de Miller–Rabin
 *
 *  Fatoramos  n-1 = d · 2^r   (d ímpar).
 *  Para cada witness  a:
 *      x₀ = a^d mod n
 *      Se x₀ ∈ {1, n-1}  ⇒ possivelmente primo
 *      Caso contrário, repetir  r-1  vezes:
 *          xᵢ = xᵢ₋₁² mod n
 *          Se xᵢ == n-1 ⇒ possivelmente primo
 *          Se xᵢ == 1   ⇒ composto
 *  Se nenhuma iteração encontra n-1 ⇒ composto.
 *──────────────────────────────────────────────────────────────*/
#include "miller_rabin_test.h"
#include <boost/multiprecision/cpp_int.hpp>
#include "../fast_divisibility.h"

using BigInt = boost::multiprecision::cpp_int;

bool MillerRabinTest::isPrime(const BigInt& modulusUnderTest,
                              int witnessIterations,
                              PRNG& randomGenerator)
{
    if (modulusUnderTest <= 1) return false;
    if (modulusUnderTest == 2 || modulusUnderTest == 3) return true;
    if ((modulusUnderTest & 1) == 0) return false;               // par

    /* Divisão rápida por primos pequenos */
    if (isCompositeByTrialDivision(modulusUnderTest))
        return false;

    /* n-1 = oddComponent · 2^powerOfTwoExponent */
    BigInt oddComponent;
    unsigned powerOfTwoExponent;
    const BigInt nMinusOne = modulusUnderTest - 1;
    // Decompor n-1
    decompose(nMinusOne, powerOfTwoExponent, oddComponent);

    for (int iteration = 0; iteration < witnessIterations; ++iteration)
    {
        BigInt candidateWitness =
            generateWitness(modulusUnderTest, randomGenerator);
        
        if (boost::math::gcd(candidateWitness, modulusUnderTest) != 1)
            return false;
        // gcd(a, n) != 1 ⇒ witness não é primo
        BigInt currentPower =
            boost::multiprecision::powm(candidateWitness,
                                        oddComponent,
                                        modulusUnderTest);
        // x₀ = a^d mod n
        if (currentPower == 1 || currentPower == nMinusOne)
            continue;                                            // Próxima witness

        // x₀ ∈ {1, n-1} ⇒ possivelmente primo
        bool hitMinusOne = false;
        for (unsigned j = 1; j < powerOfTwoExponent; ++j)
        {
            currentPower =
                boost::multiprecision::powm(currentPower, 2, modulusUnderTest);

            if (currentPower == nMinusOne) { hitMinusOne = true; break; }
            if (currentPower == 1)          return false;        // Raiz proibida
        }
        if (!hitMinusOne) return false;                          // Composto
    }
    return true;
}
