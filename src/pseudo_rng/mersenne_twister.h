// pseudo_rng/mersenne_twister.h
#pragma once
#include "prng.h"
#include <array>
#include <cstdint>

class MersenneTwister : public PRNG
{
private:
    static constexpr unsigned WORD_SIZE = 32;
    static constexpr unsigned STATE_LENGTH = 624;
    static constexpr unsigned TWIST_OFFSET = 397;
    static constexpr uint32_t MATRIX_A = 0x9908B0DF;
    static constexpr uint32_t UPPER_MASK = 0x80000000; // Most significant bit
    static constexpr uint32_t LOWER_MASK = 0x7FFFFFFF; // Least significant 31 bits

    static constexpr unsigned TEMPERING_SHIFT_U = 11;
    static constexpr uint32_t TEMPERING_MASK_D = 0xFFFFFFFF;
    static constexpr unsigned TEMPERING_SHIFT_S = 7;
    static constexpr uint32_t TEMPERING_MASK_B = 0x9D2C5680;
    static constexpr unsigned TEMPERING_SHIFT_T = 15;
    static constexpr uint32_t TEMPERING_MASK_C = 0xEFC60000;
    static constexpr unsigned TEMPERING_SHIFT_L = 18;

    static const unsigned int SEED_MULTI = 1812433253;

    unsigned int index = 0;
    std::array<uint_fast32_t, STATE_LENGTH> state_mt;

    void twist();

public:
    explicit MersenneTwister(uint_fast32_t seed);
    uint_fast32_t generate() override;
    void setSeed(uint_fast32_t newSeed);
};
