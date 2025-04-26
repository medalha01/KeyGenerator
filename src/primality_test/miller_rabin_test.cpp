// primality_test/miller_rabin_test.cpp
#include "miller_rabin_test.h"
#include <boost/multiprecision/cpp_int.hpp> // Necessário para BigInt e powm
#include <boost/multiprecision/number.hpp>  // Necessário para powm
#include "../fast_divisibility.h" // Para isCompositeByTrialDivision

using BigInt = boost::multiprecision::cpp_int;

// Implementação do teste de Miller-Rabin
bool MillerRabinTest::isPrime(const BigInt &n, int iterations, PRNG &prng)
{
    // Casos base simples
    if (n <= 1) return false; // 1 e números negativos não são primos
    if (n == 2 || n == 3) return true; // 2 e 3 são primos
    if ((n & 1) == 0) return false; // Números pares > 2 não são primos

    // Passo 1: Teste de divisão por primos pequenos (otimização)
    if (isCompositeByTrialDivision(n)) {
        // A função retorna true se encontrou um divisor p onde n != p.
        // Isso significa que n é composto.
        return false;
        // Nota: Se n for um dos primos pequenos da wheel, a função acima retorna false,
        // e o teste continua (e passará corretamente).
    }

    // Passo 2: Decompor n-1 em d * 2^r, com d ímpar
    BigInt nMinus1 = n - 1;
    unsigned r = 0;
    BigInt d = 0;
    decompose(nMinus1, r, d); // Usa a função helper da classe base

    // Passo 3: Realizar 'iterations' rodadas do teste
    for (int i = 0; i < iterations; ++i)
    {
        // 3a: Escolher uma testemunha aleatória a no intervalo [2, n-2]
        BigInt witness = generateWitness(n, prng); // Usa a função helper refatorada

        // 3b: Calcular x = a^d mod n
        BigInt x = boost::multiprecision::powm(witness, d, n);

        // 3c: Verificar as condições de Miller-Rabin
        if (x == 1 || x == nMinus1) {
            // Se a^d ≡ 1 (mod n) ou a^d ≡ -1 (mod n), n pode ser primo.
            // Continua para a próxima iteração/testemunha.
            continue;
        }

        // 3d: Loop para verificar a^(d*2^j) mod n para j de 1 a r-1
        bool possiblePrime = false; // Flag para indicar se encontramos a condição a^(d*2^j) ≡ -1
        for (unsigned j = 1; j < r; ++j) // Loop de 1 até r-1
        {
            // Calcular x = x^2 mod n (ou seja, a^(d*2^j) mod n)
            x = boost::multiprecision::powm(x, 2, n);

            if (x == nMinus1) {
                // Se a^(d*2^j) ≡ -1 (mod n), n pode ser primo para esta testemunha.
                // Marca como possível primo e sai do loop interno.
                possiblePrime = true;
                break;
            }

            // Otimização: Se x se tornar 1 neste ponto, significa que o x anterior
            // não era n-1, então n não pode ser primo.
            // (Porque se n é primo, as únicas raízes quadradas de 1 mod n são 1 e n-1)
            if (x == 1) {
                 return false; // n é definitivamente composto
            }
        }

        // 3e: Se o loop terminou sem encontrar x ≡ -1 (mod n), então n é composto
        if (!possiblePrime) {
            // Isso ocorre se x nunca foi n-1 no loop interno.
            // (O caso x=1 é tratado dentro do loop)
            return false; // n é definitivamente composto
        }
        // Se chegou aqui (possiblePrime é true), continua para a próxima iteração.
    }

    // Se n passou em todas as 'iterations' rodadas, é provavelmente primo.
    return true;
}
