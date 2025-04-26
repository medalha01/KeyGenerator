// key_generator.h
#pragma once
#include "prng.h"
#include "primality_test/primality_test.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <memory>
#include <future>
#include <atomic>
#include <cstdint> // Incluído para uint_fast32_t

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
    PrimalityTest* primalityTester_;                   // Ponteiro externo (não possui posse)
    unsigned keyBits_;                                 // Tamanho da chave em bits

public:
    // Construtor principal
    KeyGenerator(std::unique_ptr<PRNG> prng,
                 PrimalityTest* tester,
                 unsigned keyBits = 2048,
                 int primalityIter = 64);

    // Permite trocar o PRNG (transferência de posse)
    void setGenerator(std::unique_ptr<PRNG> newPrng);
    // Permite trocar o algoritmo de teste de primalidade
    void setTester(PrimalityTest* newTester);

    /* ---------- API de geração ---------- */
    // Gera chave sequencialmente (thread única)
    [[nodiscard]] BigInt generateKey(uint_fast32_t seed);
    // Gera chave usando múltiplas threads
    [[nodiscard]] BigInt generateKeyConcurrent(uint_fast32_t seed);

private:
    // Método interno para gerar um candidato a primo (ímpar, MSB set)
    // Agora recebe o PRNG a ser usado como argumento.
    [[nodiscard]] BigInt generateCandidate(PRNG& prng);

    // Sobrecarga mantida para compatibilidade interna ou testes simples,
    // mas a versão principal agora é a que recebe PRNG&.
    // Esta versão usará o prng_ membro após semear.
    [[nodiscard]] BigInt generateCandidate(uint_fast32_t seed);
};
