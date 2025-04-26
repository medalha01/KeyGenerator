#include "primality_test.h"
#include <boost/multiprecision/miller_rabin.hpp>

using BigInt = boost::multiprecision::cpp_int;

class MillerRabinTest : public PrimalityTest
{
public:
    bool isPrime(const BigInt &n, int iterations, PRNG &prng) override
    {
        if (n == 3 || n == 2)
            return true; // 3 é primo
        if (n < 2)
            return false; // n < 3 é composto
        if ((n & 1) == 0)
            return false; // pares > 2 são compostos

        const BigInt nMinus1 = n - 1;
        unsigned r = 0;
        BigInt d = 0;

        // Decomposição de n-1 = d * 2^r
        decompose(nMinus1, r, d);

        for (int i = 0; i < iterations; ++i)
        {
            BigInt witness = generateWitness(n, prng); // sorteia a testemunha
            BigInt x = powm(witness, d, n);            // x = aᵈ mod n
            if (x == 1 || x == nMinus1)
                continue; // a^d ≡ 1 ou a^d ≡ n-1

            bool isComposite = true;

            for (unsigned j = 0; j < r - 1; ++j)
            {
                x = powm(x, 2, n); // x = x² mod n
                if (x == nMinus1)
                {
                    isComposite = false; // a^(2^j * d) ≡ n-1
                    break;
                }
            }
            if (isComposite)
                return false; // n é composto
        }
        return true; // n é provavelmente primo
    }
};