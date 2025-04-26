// src//primality_test/primality_test.h
#pragma once

#include <cstdint> // For int types
#include <boost/multiprecision/cpp_int.hpp> // For BigInt

// Forward declare PRNG if needed for random number generation within tests
class PRNG;

// Define BigInt type alias if not already globally available
using BigInt = boost::multiprecision::cpp_int;

// Interface for primality tests using BigInt
class PrimalityTest {
public:
    virtual ~PrimalityTest() = default; // Essential virtual destructor

    /**
     * @brief Checks if a number is likely prime.
     *
     * @param n The number to test (must be >= 2).
     * @param iterations The number of iterations for probabilistic tests (e.g., Miller-Rabin).
     * @param rng A reference to a PRNG for generating random witnesses if needed by the test.
     * @return true if n is likely prime, false otherwise.
     */
    virtual bool isPrime(const BigInt& n, int iterations, PRNG& rng) = 0;
};
