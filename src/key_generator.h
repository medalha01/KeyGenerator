#pragma once
#include "prng.h"
#include "primality_test/primality_test.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <memory>
#include <future>
#include <atomic>

using BigInt = boost::multiprecision::cpp_int;

/* =========================================================================
   Gera chaves RSA (ou similares) encontrando números primos com N bits.
   Suporta geração concorrente usando múltiplas threads.
   ========================================================================= */
class KeyGenerator
{
private:
    const int primalityIterations_;                    // Iterações do teste
    std::unique_ptr<PRNG> prng_;                       // PRNG “mestre”
    PrimalityTest* primalityTester_;                   // Ponteiro externo
    unsigned keyBits_;                                 // Tamanho da chave

public:
    KeyGenerator(std::unique_ptr<PRNG> prng,
                 PrimalityTest* tester,
                 unsigned keyBits = 2048,
                 int primalityIter = 64);

    void setGenerator(std::unique_ptr<PRNG> newPrng);
    void setTester(PrimalityTest* newTester);

    /* ---------- API de geração ---------- */
    [[nodiscard]] BigInt generateKey(uint_fast32_t seed);
    [[nodiscard]] BigInt generateKeyConcurrent(uint_fast32_t seed);

private:
    [[nodiscard]] BigInt generateCandidate();
    [[nodiscard]] BigInt generateCandidate(uint_fast32_t seed);
};

