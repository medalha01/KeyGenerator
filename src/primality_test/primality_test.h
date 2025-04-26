// ──────────────────────────────────────────────
// primality_test/primality_test.h
// ──────────────────────────────────────────────
#pragma once

#include <boost/multiprecision/cpp_int.hpp>

class PRNG; // fwd‑decl
using BigInt = boost::multiprecision::cpp_int;

/**
 * @brief Abstract probabilistic primality test.
 */
class PrimalityTest {
public:
    virtual ~PrimalityTest() = default;
    [[nodiscard]] virtual bool isPrime(const BigInt& n,
                                       int iterations,
                                       PRNG& rng) = 0;
};

