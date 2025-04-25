// key.generator.h

#include <bitset>
#include <iostream>

#include "prng.h"

class KeyGenerator
{
private:
    PRNG *prng;

    static const unsigned int KEY_SIZE = 2048;

public:
    KeyGenerator(PRNG *prng) : prng(prng)
    {
    }

    ~KeyGenerator()
    {
        delete prng;
    }

    void set_generator(PRNG *new_prng)
    {
        delete prng;
        prng = new_prng;
    }

    std::bitset<KEY_SIZE> generate_key_candidate(uint_fast32_t seed)
    {
        prng->setSeed(seed);
        return generate_key_candidate();
    }

    std::bitset<KEY_SIZE> generate_key_candidate()
    {
        std::bitset<KEY_SIZE> key;
        unsigned pos = 0;
        while (pos < KEY_SIZE)
        {
            uint_fast32_t w = prng->generate();
            for (unsigned i = 0; i < 32 && pos < KEY_SIZE; ++i, ++pos)
            {
                key[pos] = (w >> i) & 1u;
            }
        }
        key[0] = 1;
        key[KEY_SIZE - 1] = 1;
        return key;
    }
};