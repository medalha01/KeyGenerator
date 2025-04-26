#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <stdexcept>  // For catching exceptions
#include <functional> // For std::function
#include <iomanip>    // For std::fixed, std::setprecision

// Include BigInt *before* KeyGenerator if KeyGenerator uses it in the header
#include <boost/multiprecision/cpp_int.hpp> // Include Boost BigInt

#include "key_generator.h" // Now uses BigInt
#include "pseudo_rng/mersenne_twister.h"
#include "pseudo_rng/blum_blum_shub.h"
// #include "primality_tests.h" // Include if you use primality tests

// Define BigInt type alias if not already done globally
using BigInt = boost::multiprecision::cpp_int;

// Function to run benchmark for a specific PRNG type
void benchmark_prng(
    const std::string &prng_name,
    std::function<PRNG *()> create_prng_func,
    int num_runs,
    unsigned int key_size_bits, // Pass key size
    uint_fast32_t key_gen_seed) // Seed for KeyGenerator::generate_key_candidate
{
    std::cout << "\n--- Benchmarking " << prng_name << " (" << key_size_bits << " bits) ---" << std::endl;
    auto global_start_time = std::chrono::high_resolution_clock::now();
    std::cout << "Start time: " << std::chrono::system_clock::to_time_t(global_start_time) << std::endl; // More informative time

    std::vector<BigInt> generated_keys; // Store keys if needed (optional)
    generated_keys.reserve(num_runs);
    double total_duration_sec = 0.0;

    try
    {
        // Create the PRNG instance *outside* the loop if its creation is expensive
        // and if KeyGenerator doesn't take ownership in the loop.
        // However, the original code created a new PRNG per run, let's stick to that
        // for now, assuming KeyGenerator manages ownership correctly.

        for (int i = 0; i < num_runs; ++i)
        {
            auto run_start_time = std::chrono::high_resolution_clock::now();

            // Create the specific PRNG instance for this run
            // Use unique_ptr for safer memory management
            std::unique_ptr<PRNG> prng_instance(create_prng_func());
            if (!prng_instance)
            {
                throw std::runtime_error("Failed to create PRNG instance.");
            }

            PrimalityTest *primality_tester = nullptr; // Replace with actual tester if needed
            // KeyGenerator uses the PRNG pointer but doesn't own it
            KeyGenerator key_gen(prng_instance.get(), primality_tester, key_size_bits); // Pass key size

            // Generate a key candidate using the specific seed for this run
            // Note: This re-seeds the PRNG *every time* inside generate_key_candidate
            BigInt key = key_gen.generate_key_candidate(key_gen_seed + i); // Vary seed slightly per run?

            // Optional: Store or use the key
            // generated_keys.push_back(key);
            // std::cout << "Generated key " << i << ": " << std::hex << key << std::dec << std::endl; // Example: print key (can be very long!)

            auto run_end_time = std::chrono::high_resolution_clock::now();
            total_duration_sec += std::chrono::duration<double>(run_end_time - run_start_time).count();

            // prng_instance goes out of scope and is deleted by unique_ptr
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Error during benchmark for " << prng_name << ": " << e.what() << std::endl;
        return; // Exit benchmark for this PRNG
    }
    catch (const std::exception &e)
    {
        std::cerr << "An unexpected error occurred during benchmark for " << prng_name << ": " << e.what() << std::endl;
        return;
    }

    auto global_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = global_end_time - global_start_time; // Use overall time for total

    double total_time_ms = elapsed_seconds.count() * 1000.0;
    // double total_time_ms = total_duration_sec * 1000.0; // Alternative: sum of run durations
    double avg_time_ms_per_key = total_time_ms / num_runs;

    std::cout << "Finished time: " << std::chrono::system_clock::to_time_t(global_end_time) << std::endl;
    std::cout << "Number of runs: " << num_runs << std::endl;
    std::cout << std::fixed << std::setprecision(3); // Format output
    std::cout << "Total time: " << total_time_ms << " ms" << std::endl;
    std::cout << "Average time per key generation: " << avg_time_ms_per_key << " ms" << std::endl;
    std::cout << "--- End Benchmark ---" << std::endl;
}

int main()
{
    const int num_runs = 100; // Reduce runs for potentially slower BigInt ops
    const unsigned int key_size_2048 = 2048;
    const unsigned int key_size_4096 = 4096;
    const uint_fast32_t mt_key_seed = 12345;  // Base seed for MT key generation
    const uint_fast32_t bbs_key_seed = 67891; // Base seed for BBS key generation (check validity constraints)

    auto global_start_time = std::chrono::high_resolution_clock::now();
    std::cout << "Starting RNG Benchmarks..." << std::endl;
    std::cout << "Global Start Time: " << std::chrono::system_clock::to_time_t(global_start_time) << std::endl; // Use system_clock for human-readable time
    std::cout << "Number of runs per PRNG/size: " << num_runs << std::endl;

    // --- Mersenne Twister Benchmarks ---
    benchmark_prng(
        "Mersenne Twister",
        []()
        { return new MersenneTwister(0); }, // Initial PRNG seed (overwritten by key_gen_seed)
        num_runs,
        key_size_2048, // Specify key size
        mt_key_seed);

    benchmark_prng(
        "Mersenne Twister",
        []()
        { return new MersenneTwister(0); },
        num_runs,
        key_size_4096, // Specify key size
        mt_key_seed);

    std::cout << "\n"; // Add a newline for separation

    // --- Blum Blum Shub Benchmarks ---
    // Ensure bbs_key_seed is valid for the specific p, q in BlumBlumShub.h
    // n = p * q = 100127 * 100183 = 10031173941. seed must be < n and gcd(seed, n) == 1.
    // 67891 is prime and not p or q, so gcd(67891, n) = 1. It's valid.
    benchmark_prng(
        "Blum Blum Shub",
        []()
        { return new BlumBlumShub(0); }, // Initial PRNG seed (overwritten by key_gen_seed)
        num_runs,
        key_size_2048, // Specify key size
        bbs_key_seed   // Must use a valid seed here
    );

    benchmark_prng(
        "Blum Blum Shub",
        []()
        { return new BlumBlumShub(0); },
        num_runs,
        key_size_4096, // Specify key size
        bbs_key_seed);

    auto global_end_time = std::chrono::high_resolution_clock::now();
    std::cout << "\nGlobal End Time: " << std::chrono::system_clock::to_time_t(global_end_time) << std::endl;
    std::cout << "RNG Benchmarks Finished." << std::endl;

    return 0;
}
