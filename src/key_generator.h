#pragma once

#include <iostream>
#include <cstdint>                      
#include <boost/multiprecision/cpp_int.hpp>
#include "primalty_test/primality_test.h"
#include "prng.h"




using BigInt = boost::multiprecision::cpp_int;

class KeyGenerator
{
private:
    const int PRIMALITY_TEST_ITERATIONS = 64; 
    PRNG *prng;
    PrimalityTest *primality_tester;

    int key_size_bits;

public:
    KeyGenerator(PRNG *prng_ptr, PrimalityTest *primality_tester, unsigned int key_bits = 2048) : prng(prng_ptr), primality_tester(primality_tester), key_size_bits(key_bits)
    {
        if (key_bits == 0)
        {
            throw std::invalid_argument("Key size cannot be zero.");
        }
        // PRNG pointer ownership is managed outside or via smart pointers preferably
        // The original destructor `delete prng;` is problematic if the PRNG
        // object is managed elsewhere. We remove it here. Consider using
        // std::unique_ptr or std::shared_ptr for better memory management.
    }

    ~KeyGenerator() = default;

    void set_generator(PRNG *new_prng)
    {

        prng = new_prng;
    }

    void set_tester(PrimalityTest *new_tester)
    {
        primality_tester = new_tester;
    }
    // Generates a key candidate of the configured size using the given seed
    BigInt generate_key_candidate(uint_fast32_t seed)
    {
        if (!prng)
        {
            throw std::runtime_error("PRNG not set in KeyGenerator.");
        }
        prng->setSeed(seed);
        return generate_key_candidate();
    }

    // Generates a key candidate of the configured size using the current PRNG state
    BigInt generate_key_candidate()
    {
        {
        if (!prng)
            throw std::runtime_error("PRNG not set in KeyGenerator.");
        }

        BigInt key = 0;
        unsigned int current_bits = 0;
        const unsigned int bits_per_prng_output = 32; // Assuming PRNG generates uint32_t

        while (current_bits < key_size_bits)
        {
            uint_fast32_t random_word = prng->generate();
            unsigned int bits_to_take = std::min(bits_per_prng_output, key_size_bits - current_bits);

            // Create a mask for the lower 'bits_to_take' bits
            uint_fast32_t mask = (bits_to_take == 32) ? 0xFFFFFFFF : (1u << bits_to_take) - 1;
            BigInt chunk = random_word & mask;

            // Shift the chunk into the correct position and add to the key
            key |= (chunk << current_bits);

            current_bits += bits_to_take;
        }

        boost::multiprecision::bit_set(key, 0);

        if (key_size_bits > 0)
        {
            boost::multiprecision::bit_set(key, key_size_bits - 1);
        }

        return key;
    }

    unsigned int get_key_size_bits() const
    {
        return key_size_bits;
    }
};
