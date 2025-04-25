//pseudo_rng/prng.h
#pragma once
#include <cstdint>

class PRNG
{
protected:
    uint_fast32_t seed;

public:
    PRNG() = default;

    explicit PRNG(uint_fast32_t seed) : seed(seed) {}
    virtual ~PRNG() = default;

    virtual uint_fast32_t generate() = 0;

    void setSeed(uint_fast32_t newSeed)
    {
        seed = newSeed;
    }
};
