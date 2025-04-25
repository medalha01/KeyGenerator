// pseudo_rng/mersenne_twister.cpp
#include "mersenne_twister.h"
#include <array>
#include <cstdint>
#include <random>
#include <ctime>
#include <bitset>
#include <iostream>

MersenneTwister::MersenneTwister(uint_fast32_t seed)
{
    this->seed = seed;
    setSeed(seed);
}

void MersenneTwister::setSeed(uint_fast32_t newSeed)
{
    // Set the first element of the internal state array to the seed
    // and initialize the rest of the state array
    state_mt[0] = seed;

    for (unsigned int i = 1; i < STATE_LENGTH; ++i)
    {
        // Take the previous element in the state array
        uint_fast32_t previous_state = state_mt[i - 1];

        // Formula:
        // state[i] = (SEED_MULTI * (previous_state XOR (previous_state >> (WORD_SIZE - 2))) + i)
        //
        // - previous_state >> (WORD_SIZE - 2) : right shift by 30 bits if WORD_SIZE = 32
        // - XOR with previous_state to introduce more entropy
        // - Multiply by a large constant SEED_MULTI (typically 1812433253)
        // - Add the current index to further decorrelate values
        state_mt[i] = SEED_MULTI * (previous_state ^ (previous_state >> (WORD_SIZE - 2))) + i;
    }

    index = STATE_LENGTH;
}

// refresh the states
void MersenneTwister::twist()
{
    // Generate the next STATE_LENGTH values in the sequence
    for (unsigned i = 0; i < STATE_LENGTH; ++i)
    {

        // Combine the upper bits of state[i] and the lower bits of state[i+1]
        // UPPER_MASK and LOWER_MASK are masks that split a 32-bit word
        // UPPER_MASK typically masks the highest bit(s), LOWER_MASK the lowest bits
        uint_fast32_t x = (state_mt[i] & UPPER_MASK) | (state_mt[(i + 1) % STATE_LENGTH] & LOWER_MASK);

        // Right shift x by 1 bit
        uint_fast32_t xA = x >> 1;

        if (x & 1)
        {
            // MATRIX_A introduces non-linearity into the generator
            xA ^= MATRIX_A;
        }

        // Mix xA with another part of the state array (offset by TWIST_OFFSET)

        state_mt[i] = state_mt[(i + TWIST_OFFSET) % STATE_LENGTH] ^ xA;
    }
    index = 0;
}

uint_fast32_t MersenneTwister::generate()
{
    if (index >= STATE_LENGTH)
    {
        // If the index is at the end of the state array, generate a new batch of values
        twist();
    }

    uint_fast32_t y = state_mt[index++];

    // Apply the tempering transformation to the value at the current index
    y ^= (y >> TEMPERING_SHIFT_U) & TEMPERING_MASK_D;
    y ^= (y << TEMPERING_SHIFT_S) & TEMPERING_MASK_B;
    y ^= (y << TEMPERING_SHIFT_T) & TEMPERING_MASK_C;
    y ^= (y >> TEMPERING_SHIFT_L);

    return y;
}
