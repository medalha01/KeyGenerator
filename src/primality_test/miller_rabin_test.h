#include "primality_test.h"
#include <boost/multiprecision/miller_rabin.hpp>
#include "../fast_divisibility.h"

using BigInt = boost::multiprecision::cpp_int;

class MillerRabinTest : public PrimalityTest
{
public:
     [[nodiscard]] bool isPrime(
        const BigInt &n,
        int iterations,
        PRNG &prng) override;
};
