#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <stdexcept> // For catching exceptions
#include <functional> // For std::function

#include "key_generator.h"
#include "pseudo_rng/mersenne_twister.h"
#include "pseudo_rng/blum_blum_shub.h"

// Function to run benchmark for a specific PRNG type
void benchmark_prng(
    const std::string& prng_name,
    std::function<PRNG*()> create_prng_func,
    int num_runs,
    uint_fast32_t key_seed) // Seed for KeyGenerator::generate_key_candidate
{
    std::cout << "\n--- Benchmarking " << prng_name << " ---" << std::endl;
    std::cout << "Starting time: " << __TIME__ << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        for (int i = 0; i < num_runs; ++i)
        {
            // Create the specific PRNG instance
            PRNG* prng_instance = create_prng_func();

            // KeyGenerator takes ownership of the PRNG pointer
            KeyGenerator key_gen(prng_instance);

            // Generate a key candidate using the specific seed
            // This also sets the PRNG's internal seed via setSeed()
            std::bitset<2048> key = key_gen.generate_key_candidate(key_seed);

            // The KeyGenerator destructor will delete prng_instance
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error during benchmark for " << prng_name << ": " << e.what() << std::endl;
        return; // Exit benchmark for this PRNG
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred during benchmark for " << prng_name << ": " << e.what() << std::endl;
        return;
    }


    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    double total_time_ms = elapsed_seconds.count() * 1000.0;
    double avg_time_ms_per_key = total_time_ms / num_runs;

    std::cout << "Finished time: " << __TIME__ << std::endl;
    std::cout << "Number of runs: " << num_runs << std::endl;
    std::cout << "Total time: " << total_time_ms << " ms" << std::endl;
    std::cout << "Average time per key: " << avg_time_ms_per_key << " ms" << std::endl;
    std::cout << "--- End Benchmark ---" << std::endl;
}

int main()
{
    const int num_runs = 1000; // Number of key generations for each PRNG
    const uint_fast32_t mt_key_seed = 12345; // Seed for MT key generation
    // For BBS, the seed must be non-zero, < n, and coprime to n.
    // n = 100127 * 100183 = 10031173941.
    // A prime number different from 100127 and 100183 is a safe choice. 67891 is prime.
    const uint_fast32_t bbs_key_seed = 67891; // Seed for BBS key generation

    std::cout << "Starting RNG Benchmarks..." << std::endl;
    std::cout << "Global Start Time: " << __TIME__ << std::endl;
    std::cout << "Number of runs per PRNG: " << num_runs << std::endl;
    std::cout << "Key Size: " << 2048 << " bits" << std::endl;

    // Benchmark Mersenne Twister
    benchmark_prng(
        "Mersenne Twister",
        []() { return new MersenneTwister(0); }, // Initial PRNG seed (can be anything for MT)
        num_runs,
        mt_key_seed
    );

    std::cout << "\n"; // Add a newline for separation

    // Benchmark Blum Blum Shub
    benchmark_prng(
        "Blum Blum Shub",
        []() { return new BlumBlumShub(0); }, // Initial PRNG seed (will be overwritten)
        num_runs,
        bbs_key_seed // Must use a valid seed here
    );

    std::cout << "\nGlobal End Time: " << __TIME__ << std::endl;
    std::cout << "RNG Benchmarks Finished." << std::endl;

    return 0;
}
