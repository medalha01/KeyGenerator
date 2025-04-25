//pseudo_rng/blum_blum_shub.h

#pragma once

#include "prng.h"
#include <random>
#include <stdexcept>
#include <cstdint>

class BlumBlumShub : public PRNG
{
private:
    // Precomputed primes p and q
    static constexpr uint64_t p = 100127;  // p ≡ 3 mod 4
    static constexpr uint64_t q = 100183;  // q ≡ 3 mod 4
    static constexpr uint64_t n = p * q;   // Modulus n = p * q

    uint_fast64_t state;  

    static uint_fast64_t gcd(uint_fast64_t a, uint_fast64_t b)
    {
        while (b != 0) {
            uint_fast64_t temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

    void initialize_state()
    {
        if (seed == 0 || seed >= n || gcd(seed, n) != 1)
        {
            state = 7641693654; // Default seed
            //throw std::invalid_argument("Seed must be a non-zero integer less than n and coprime to n.");
        }
        else
        {
            state = seed;
        }
    }

public:
    explicit BlumBlumShub(uint_fast32_t initialSeed) : PRNG(initialSeed)
    {
        initialize_state();
    }

    uint_fast32_t generate() override
    {
        uint_fast32_t output = 0;
        for (int i = 0; i < 32; ++i)
        {
            // Generate the next bit
            state = (state * state) % n;
            // Extract the least significant bit
            output = (output << 1) | (state & 1u);
        }
        return output;
    }

    void setSeed(uint_fast32_t newSeed)
    {
        PRNG::setSeed(newSeed);
        initialize_state();
    }
};