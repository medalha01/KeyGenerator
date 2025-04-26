#include "fermat_test.h"

using BigInt = boost::multiprecision::cpp_int;

BigInt FermatTest::generate_witness(const BigInt &n, PRNG &rng)
{
    if (n <= 3)
    {
        throw std::invalid_argument("n must be greater than 3.");
    }

    BigInt witness_number = 0;                              // Initialize witness number
    int number_of_bits = boost::multiprecision::msb(n) + 1; // Get the number of bits in n

    while (witness_number < 2)
    {
        unsigned int current_bits = 0;
        const unsigned int bits_per_prng_output = 32;

        while (current_bits < number_of_bits)
        {
            uint_fast32_t random_word = rng.generate();
            unsigned int bits_to_take = std::min(bits_per_prng_output, (unsigned int)(number_of_bits - current_bits));

            // Create a mask for the lower 'bits_to_take' bits
            // Handle the case where bits_to_take is 32 carefully
            uint_fast32_t mask = (bits_to_take >= 32) ? 0xFFFFFFFF : (static_cast<uint_fast32_t>(1) << bits_to_take) - 1;
            BigInt chunk = random_word & mask;

            witness_number |= (chunk << current_bits);
            current_bits += bits_to_take;
        }
        witness_number %= n;
    }
    return witness_number;
}

bool FermatTest::isPrime(const BigInt &n, int iterations, PRNG &rng)
{
    if (n <= 1)
    {
        return false;
    }
    if (n <= 3)
    {
        return true;
    }
    if (n % 2 == 0)
    {
        return false;
    }

    // --- Fermat Test Loop ---

    // Pre-calculate n-1 as it's used repeatedly

    BigInt n_minus_1 = n - 1;
    for (int i = 0; i < iterations; ++i)
    {
        BigInt witness = generate_witness(n, rng);
        BigInt result = boost::multiprecision::powm(witness, n_minus_1, n);
        if (result != 1)
        {
            return false;
        }
    }
    return true;
}
