// key_generator.cpp
#include "key_generator.h"
#include <algorithm>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <vector> // Incluído para std::vector
#include <atomic> // Incluído para std::atomic
#include <future> // Incluído para std::promise, std::future

KeyGenerator::KeyGenerator(std::unique_ptr<PRNG> prng,
                           PrimalityTest* tester,
                           unsigned keyBits,
                           int primalityIter)
    : primalityIterations_(primalityIter),
      prng_(std::move(prng)),         // Assume posse do PRNG
      primalityTester_(tester),       // Armazena ponteiro para o tester
      keyBits_(keyBits)
{
    // Validação dos parâmetros
    if (!prng_) { throw std::invalid_argument("PRNG não pode ser nulo."); }
    if (!primalityTester_) { throw std::invalid_argument("Primality Tester não pode ser nulo."); }
    if (keyBits_ == 0) { throw std::invalid_argument("Tamanho da chave (keyBits) não pode ser zero."); }
    if (keyBits_ < 2) { throw std::invalid_argument("Tamanho da chave (keyBits) deve ser pelo menos 2."); } // Garante MSB e LSB podem ser setados
    if (primalityIterations_ <= 0) { throw std::invalid_argument("Número de iterações de primalidade deve ser positivo."); }
}

/* -------------------------------------------------------------------------
   Troca o PRNG (transferência de posse via unique_ptr)
   ------------------------------------------------------------------------- */
void KeyGenerator::setGenerator(std::unique_ptr<PRNG> newPrng)
{
    if (!newPrng) throw std::invalid_argument("PRNG não pode ser nulo.");
    prng_ = std::move(newPrng);
}

/* -------------------------------------------------------------------------
   Troca o Teste de Primalidade (ponteiro externo)
   ------------------------------------------------------------------------- */
void KeyGenerator::setTester(PrimalityTest* newTester)
{
    if (!newTester) throw std::invalid_argument("Primality Tester não pode ser nulo.");
    primalityTester_ = newTester;
}

/* -------------------------------------------------------------------------
   Constrói candidato de 'keyBits_' bits usando o PRNG fornecido.
   Garante que o número seja ímpar e tenha o bit mais significativo ligado.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateCandidate(PRNG& prng) // Recebe PRNG por referência
{
    const unsigned bitsPerCall = 32; // Assumindo que PRNG gera 32 bits por chamada
    BigInt candidate {0};
    unsigned bitsGerados = 0;

    // Gera bits aleatórios suficientes para preencher keyBits_
    while (bitsGerados < keyBits_) {
        const uint_fast32_t rnd = prng.generate();
        const unsigned bitsRestantes = keyBits_ - bitsGerados;
        // Pega o mínimo entre os bits disponíveis no 'rnd' e os bits que faltam
        const unsigned bitsParaPegar = std::min(bitsPerCall, bitsRestantes);

        // Cria máscara para pegar apenas os 'bitsParaPegar' bits inferiores de 'rnd'
        const uint_fast32_t mask = (bitsParaPegar >= 32) ?
            0xFFFFFFFFu : // Caso especial para evitar shift de 32 bits
            ((1u << bitsParaPegar) - 1u);

        // Incorpora os bits aleatórios no candidato na posição correta
        candidate |= BigInt(rnd & mask) << bitsGerados;
        bitsGerados += bitsParaPegar;
    }

    // Garante que o candidato seja ímpar (bit 0 ligado)
    boost::multiprecision::bit_set(candidate, 0);
    // Garante que o bit mais significativo esteja ligado (para ter exatamente keyBits_ bits)
    boost::multiprecision::bit_set(candidate, keyBits_ - 1);

    return candidate;
}

/* -------------------------------------------------------------------------
   Sobrecarga: re-semente o PRNG mestre antes de gerar o candidato.
   Usado principalmente para testes ou geração única com semente específica.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateCandidate(uint_fast32_t seed)
{
    if (!prng_) throw std::runtime_error("PRNG mestre não inicializado.");
    prng_->setSeed(seed);
    // Chama a versão que recebe o PRNG por referência
    return generateCandidate(*prng_);
}

/* -------------------------------------------------------------------------
   Geração sequencial (apenas 1 thread).
   Usa o PRNG membro 'prng_'.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateKey(uint_fast32_t seed)
{
    if (!prng_) throw std::runtime_error("PRNG mestre não inicializado.");
    prng_->setSeed(seed);

    while (true) {
        // Gera um candidato usando o PRNG membro
        BigInt candidate = generateCandidate(*prng_);

        // Testa a primalidade do candidato
        // Passa o PRNG membro para o teste (pode ser usado para gerar testemunhas)
        if (primalityTester_->isPrime(candidate, primalityIterations_, *prng_))
            return candidate; // Encontrou um primo, retorna
        // Se não for primo, o loop continua para gerar e testar o próximo candidato
    }
}

/* -------------------------------------------------------------------------
   Geração concorrente usando múltiplas threads.
   Cada thread clona o PRNG mestre para ter estado independente.
   ------------------------------------------------------------------------- */
BigInt KeyGenerator::generateKeyConcurrent(uint_fast32_t seed)
{
    if (!prng_) throw std::runtime_error("PRNG mestre não inicializado.");

    // Determina o número de threads a usar (metade dos núcleos, no mínimo 1)
    const unsigned threadCount =
        std::max(1u, std::thread::hardware_concurrency() / 2);

    std::cout << "Gerando chave " << keyBits_
              << "-bits usando " << threadCount << " threads...\n";

    // Mecanismos de sincronização para obter o primeiro primo encontrado
    std::promise<BigInt> primePromise;
    std::future<BigInt> primeFuture = primePromise.get_future();
    std::atomic<bool> primeFound {false}; // Flag para sinalizar às threads que parem

    // Função executada por cada thread worker
    auto worker = [&](uint_fast32_t threadSeed)
    {
        // Clona o PRNG mestre para ter um estado independente por thread
        auto localPrng = prng_->clone();
        if (!localPrng) {
             // Tenta setar a exceção na promise se o clone falhar
            try {
                if (!primeFound.exchange(true)) { // Evita múltiplas exceções
                   primePromise.set_exception(std::make_exception_ptr(
                       std::runtime_error("Falha ao clonar PRNG na thread.")
                   ));
                }
            } catch(...) { /* Ignora exceção de set_exception se já tiver valor/exceção */ }
            return; // Termina a thread
        }
        localPrng->setSeed(threadSeed); // Semente única para cada thread

        // Usa o mesmo tester compartilhado (assume-se que isPrime é thread-safe
        // em relação ao estado do tester, mas não do PRNG passado)
        PrimalityTest* localTester = primalityTester_;

        try {
            // Continua buscando primos até que um seja encontrado por alguma thread
            while (!primeFound.load(std::memory_order_acquire)) {

                // ----- Gera candidato usando o PRNG local -----
                BigInt candidate = generateCandidate(*localPrng); // Chama a função refatorada
                // --------------------------------------------

                // Testa a primalidade usando o PRNG local para testemunhas
                if (localTester->isPrime(candidate,
                                         primalityIterations_,
                                         *localPrng))
                {
                    // Tenta definir o valor na promise se for a primeira thread a encontrar
                    // exchange retorna o valor anterior; se era false, esta thread venceu
                    if (!primeFound.exchange(true, std::memory_order_release)) {
                        primePromise.set_value(candidate); // Define o primo encontrado
                    }
                    // Mesmo se não for a primeira, sai do loop pois um primo foi encontrado
                    break;
                }
                // Se o candidato não for primo, continua o loop
            }
        }
        catch (...) { // Captura qualquer exceção durante a geração ou teste
            // Tenta reportar a exceção se nenhuma outra thread o fez
            if (!primeFound.exchange(true, std::memory_order_release)) {
                primePromise.set_exception(std::current_exception());
            }
            // A thread termina após a exceção ou após encontrar um primo
        }
    }; // Fim da lambda worker

    // Cria e inicia as threads
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned i = 0; i < threadCount; ++i) {
        // Passa uma semente diferente para cada thread (base + índice)
        threads.emplace_back(worker, seed + i);
    }

    // Aguarda o resultado (o primeiro primo encontrado ou uma exceção)
    BigInt result = primeFuture.get(); // Bloqueia até que a promise seja satisfeita

    // Junta todas as threads (espera que terminem)
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Retorna o primo encontrado
    return result;
}
