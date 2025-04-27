/*──────────────────────────────────────────────────────────────
 *  KeyGenerator  –  encontra número primo de  keyBits_  bits.
 *──────────────────────────────────────────────────────────────*/
#include "key_generator.h"
#include <atomic>
#include <future>
#include <thread>
#include <iostream>

KeyGenerator::KeyGenerator(std::unique_ptr<PRNG> masterPRNG,
                           PrimalityTest*       primalityTester,
                           unsigned             keySizeBits,
                           int                  primalityIterations)
    : primalityIterations_(primalityIterations),
      prng_(std::move(masterPRNG)),
      primalityTester_(primalityTester),
      keyBits_(keySizeBits)
{
    if (!prng_ || !primalityTester_)
        throw std::invalid_argument("Null pointer");
    if (keyBits_ < 2)
        throw std::invalid_argument("keySizeBits must be ≥ 2");
    if (primalityIterations_ <= 0)
        throw std::invalid_argument("Iterations must be positive");
}

BigInt KeyGenerator::generateCandidate(PRNG& localPRNG)
{
    const unsigned bitsPerCall = 32;
    BigInt candidate{0};
    unsigned accumulatedBits = 0;

    while (accumulatedBits < keyBits_)
    {
        uint32_t chunk = localPRNG.generate();
        unsigned take =
            std::min(bitsPerCall, keyBits_ - accumulatedBits);
        uint32_t mask =
            (take == 32) ? 0xFFFFFFFFu : ((1u << take) - 1u);

        candidate |= BigInt(chunk & mask) << accumulatedBits;
        accumulatedBits += take;
    }

    boost::multiprecision::bit_set(candidate, 0);               // ímpar
    boost::multiprecision::bit_set(candidate, keyBits_-1);  // bit alto
    return candidate;
}

BigInt KeyGenerator::generateKey(uint_fast32_t seed)
{
    prng_->setSeed(seed);
    while (true)
    {
        BigInt potentialPrime = generateCandidate(*prng_);
        if (primalityTester_->isPrime(potentialPrime,
                                      primalityIterations_,
                                      *prng_))
            return potentialPrime;
    }
}

BigInt KeyGenerator::generateKeyConcurrent(uint_fast32_t seed)
{
    unsigned threadCount =
        std::max(1u, std::thread::hardware_concurrency() / 2);

    std::promise<BigInt> firstPrimePromise;
    std::future<BigInt>  firstPrimeFuture = firstPrimePromise.get_future();
    std::atomic<bool>    primeFound{false};

    auto worker = [&](uint_fast32_t threadSeed)
    {
        auto localPRNG = prng_->clone();
        localPRNG->setSeed(threadSeed);

        while (!primeFound.load(std::memory_order_acquire))
        {
            BigInt candidate = generateCandidate(*localPRNG);
            if (primalityTester_->isPrime(candidate,
                                          primalityIterations_,
                                          *localPRNG))
            {
                if (!primeFound.exchange(true))
                    firstPrimePromise.set_value(candidate);
                break;
            }
        }
    };

    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threadCount; ++t)
        pool.emplace_back(worker, seed + t);

    BigInt primeResult = firstPrimeFuture.get();
    for (auto& th : pool) if (th.joinable()) th.join();
    return primeResult;
}
