// primality_test/primality_test.h
#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <stdexcept> // Para std::invalid_argument
#include <algorithm> // Para std::min
#include <limits>    // Para std::numeric_limits
#include "prng.h"    // Inclui a definição de PRNG

class PRNG; // Declaração avançada (forward declaration)
using BigInt = boost::multiprecision::cpp_int;

/**
 * @brief Classe base abstrata para testes de primalidade probabilísticos.
 */
class PrimalityTest
{

protected:
    /* -------------------------------------------------------------------------
       Gera testemunha 'a' uniformemente aleatória no intervalo [2, n-2].
       Usa o PRNG fornecido para obter bits aleatórios.
       Garante distribuição mais uniforme que simplesmente `rand() % n`.
       ------------------------------------------------------------------------- */
    [[nodiscard]] BigInt generateWitness(const BigInt &n, PRNG &prng)
    {
        if (n <= 3)
        {
            // Testes de primalidade geralmente não são chamados para n <= 3,
            // mas se forem, não há testemunhas válidas em [2, n-2].
            // Poderia retornar um valor fixo ou lançar exceção.
            // Lançar exceção parece mais seguro para indicar uso incorreto.
            throw std::invalid_argument("n deve ser maior que 3 para gerar testemunha em [2, n-2].");
        }

        // O intervalo desejado é [2, n-2]. O tamanho do intervalo é (n-2) - 2 + 1 = n-3.
        const BigInt range_size = n - 3; // n >= 4 aqui. range_size >= 1.
        const unsigned target_bits = boost::multiprecision::msb(range_size) + 1;

        // Para obter uma distribuição uniforme usando PRNG de 32 bits,
        // podemos gerar um número com um pouco mais de bits que o necessário
        // e usar módulo. Adicionar alguns bits extras (e.g., 64) reduz o viés.
        const unsigned bits_to_generate = target_bits + 64;
        const unsigned bits_per_call = 32;
        BigInt random_val{0};
        unsigned bits_gerados = 0;

        while (bits_gerados < bits_to_generate)
        {
            const uint_fast32_t rnd = prng.generate();
            const unsigned bits_restantes = bits_to_generate - bits_gerados;
            const unsigned bits_para_pegar = std::min(bits_per_call, bits_restantes);
            const uint_fast32_t mask = (bits_para_pegar >= 32) ? 0xFFFFFFFFu : ((1u << bits_para_pegar) - 1u);

            random_val |= BigInt(rnd & mask) << bits_gerados;
            bits_gerados += bits_para_pegar;
        }

        // Mapeia o valor aleatório grande para o intervalo [0, range_size-1]
        // e depois desloca para [2, n-2].
        BigInt witness = 2 + (random_val % range_size);

        // Verificação de segurança (embora improvável com n > 3)
        if (witness < 2 || witness >= n - 1)
        {
            // Se algo der muito errado, retorna um valor padrão seguro.
            // Com n > 3, n-2 >= 2. Se n=4, range=1, witness = 2 + (rand % 1) = 2. OK.
            // Se n=5, range=2, witness = 2 + (rand % 2) = 2 ou 3. OK.
            // A lógica parece robusta, mas uma fallback pode ser útil.
            return 2; // Retorna 2 como fallback seguro se o cálculo falhar.
        }

        return witness;
    }

    /* -------------------------------------------------------------------------
       Decompõe n-1 em d * 2^r, onde d é ímpar.
       Usado no teste de Miller-Rabin.
       ------------------------------------------------------------------------- */
    void decompose(const BigInt &nMinusOne,
                   unsigned &r,        // Saída: expoente da potência de 2
                   BigInt &d) noexcept // Saída: fator ímpar
    {
        r = 0;
        d = nMinusOne; // Começa com d = n-1

        // Enquanto d for par e maior que zero
        while (d > 0 && (d & 1) == 0)
        {
            d >>= 1; // Divide d por 2 (deslocamento de bits para a direita)
            ++r;     // Incrementa o contador do expoente r
        }
        // Ao final, d será ímpar e n-1 = d * 2^r
    }

public:
    // Destrutor virtual padrão
    virtual ~PrimalityTest() = default;

    /**
     * @brief Executa o teste de primalidade probabilístico em n.
     * @param n O número a ser testado.
     * @param iterations O número de iterações/testemunhas a usar.
     * @param rng O gerador de números pseudo-aleatórios para escolher testemunhas.
     * @return true se n for provavelmente primo, false se for composto.
     */
    [[nodiscard]] virtual bool isPrime(const BigInt &n,
                                       int iterations,
                                       PRNG &rng) = 0;
};
