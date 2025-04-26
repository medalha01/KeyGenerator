#include "key_generator.h"
#include <algorithm>
#include <thread>
#include <iostream>
#include <stdexcept>

KeyGenerator::KeyGenerator(std::unique_ptr<PRNG> prng,
                           PrimalityTest* tester,
                           unsigned keyBits,
                           int primalityIter)
    : primalityIterations_(primalityIter),
      prng_(std::move(prng)),
      primalityTester_(tester),
      keyBits_(keyBits)
{
    if (!prng_)             throw std::invalid_argument("PRNG nulo.");
    if (!primalityTester_)  throw std::invalid_argument("Tester nulo.");
    if (keyBits_ == 0)      throw std::invalid_argument("Tamanho da chave zero.");
}

/* -------------------------------------------------------------------------
   Troca o PRNG (transferência de posse via unique_ptr)
   ------------------------------------------------------------------------- */
void KeyGenerator::setGenerator(std::unique_ptr<PRNG> newPrng)
{
    if (!newPrng) throw std::invalid_argument("PRNG nulo.");
    prng_ = std::move(newPrng);
}

void KeyGenerator::setTester(PrimalityTest* newTester)
{
    if (!newTester) throw std::invalid_argument("Tester nulo.");
    primalityTester_ = newTester;
}

/* -------------------------------------------------------------------------
   Constrói candidato de N bits usando o PRNG mestre.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateCandidate()
{
    const unsigned bitsPerCall = 32;
    BigInt candidate {0};
    unsigned bitsGerados = 0;

    while (bitsGerados < keyBits_) {
        const uint_fast32_t rnd = prng_->generate();
        const unsigned bitsRestantes = keyBits_ - bitsGerados;
        const unsigned bitsParaPegar = std::min(bitsPerCall, bitsRestantes);

        const uint_fast32_t mask =
            bitsParaPegar == 32 ? 0xFFFFFFFFu
                                : ((1u << bitsParaPegar) - 1u);

        candidate |= BigInt(rnd & mask) << bitsGerados;
        bitsGerados += bitsParaPegar;
    }

    // Garante que é ímpar e com bit mais significativo ligado
    boost::multiprecision::bit_set(candidate, 0);
    boost::multiprecision::bit_set(candidate, keyBits_ - 1);

    return candidate;
}

/* -------------------------------------------------------------------------
   Sobrecarga: re-semente antes.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateCandidate(uint_fast32_t seed)
{
    prng_->setSeed(seed);
    return generateCandidate();
}

/* -------------------------------------------------------------------------
   Geração sequencial (apenas 1 thread).
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateKey(uint_fast32_t seed)
{
    prng_->setSeed(seed);

    while (true) {
        BigInt candidate = generateCandidate();
        if (primalityTester_->isPrime(candidate, primalityIterations_, *prng_))
            return candidate;                           
    }
}

/* -------------------------------------------------------------------------
   Geração concorrente usando metade das CPUs disponíveis.
   Cada thread clona o PRNG mestre para ter estado independente.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateKeyConcurrent(uint_fast32_t seed)
{
    const unsigned threadCount =
        std::max(1u, std::thread::hardware_concurrency() / 2);

    std::cout << "Gerando chave " << keyBits_
              << "-bits usando " << threadCount << " threads...\n";

    std::promise<BigInt> primePromise;
    std::future<BigInt> primeFuture = primePromise.get_future();
    std::atomic<bool> primeFound {false};

    auto worker = [&](uint_fast32_t threadSeed)
    {
        auto localPrng = prng_->clone();
        localPrng->setSeed(threadSeed);

        try {
            while (!primeFound.load(std::memory_order_acquire)) {
                BigInt candidate {0};
                unsigned bitsGerados = 0;
                const unsigned bitsPerCall = 32;

                // ----- Constrói candidato -----
                while (bitsGerados < keyBits_) {
                    const uint_fast32_t rnd = localPrng->generate();
                    const unsigned restante = keyBits_ - bitsGerados;
                    const unsigned pegar   = std::min(bitsPerCall, restante);
                    const uint_fast32_t mask =
                        (pegar == 32) ? 0xFFFFFFFFu : ((1u << pegar) - 1u);
                    candidate |= BigInt(rnd & mask) << bitsGerados;
                    bitsGerados += pegar;
                }
                boost::multiprecision::bit_set(candidate, 0);
                boost::multiprecision::bit_set(candidate, keyBits_ - 1);
                // ------------------------------

                if (primalityTester_->isPrime(candidate,
                                             primalityIterations_,
                                             *localPrng))
                {
                    if (!primeFound.exchange(true))     // Signal winner
                        primePromise.set_value(candidate);
                    break;                              // Sai da thread
                }
            }
        }
        catch (...) {
            if (!primeFound.exchange(true))
                primePromise.set_exception(std::current_exception());
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned i = 0; i < threadCount; ++i)
        threads.emplace_back(worker, seed + i);

    // Aguarda vencedor
    BigInt result = primeFuture.get();
    for (auto& t : threads) if (t.joinable()) t.join();

    return result;
}

