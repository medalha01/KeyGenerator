// pseudo_rng/mersenne_twister.h
#pragma once
#include "prng.h"
#include <array>
#include <cstdint> // Para usar uint32_t explicitamente

/* =========================================================================
   Implementação do gerador MT19937 (32 bits) — Mersenne Twister.
   ========================================================================= */
class MersenneTwister final : public PRNG
{
private:
    // Constantes do algoritmo MT19937 (32-bit)
    static constexpr unsigned WORD_SIZE          = 32;
    static constexpr unsigned STATE_SIZE         = 624;
    static constexpr unsigned TWIST_OFFSET       = 397;
    static constexpr uint32_t MATRIX_A           = 0x9908B0DFu; // Matriz A de torção
    static constexpr uint32_t UPPER_MASK         = 0x80000000u; // Bit mais significativo
    static constexpr uint32_t LOWER_MASK         = 0x7FFFFFFFu; // Bits menos significativos

    // Constantes de "tempering" (melhora distribuição)
    static constexpr unsigned TEMPERING_SHIFT_U  = 11;
    static constexpr uint32_t TEMPERING_MASK_D   = 0xFFFFFFFFu; 
    static constexpr unsigned TEMPERING_SHIFT_S  = 7;
    static constexpr uint32_t TEMPERING_MASK_B   = 0x9D2C5680u;
    static constexpr unsigned TEMPERING_SHIFT_T  = 15;
    static constexpr uint32_t TEMPERING_MASK_C   = 0xEFC60000u;
    static constexpr unsigned TEMPERING_SHIFT_L  = 18;

    // Constante para semeadura
    static constexpr uint32_t SEED_MULTIPLIER    = 1812433253u;

    // Estado interno do gerador
    std::array<uint32_t, STATE_SIZE> stateVector_ {}; // Usa uint32_t explícito
    unsigned index_ {STATE_SIZE};                      // Índice atual no vetor de estado, força twist() inicial

    // Função interna para gerar novos números no estado
    void twist();

public:
    // Construtor default usa a semente padrão do artigo original do MT
    explicit MersenneTwister(uint_fast32_t seed = 5489u);

    // Gera o próximo número pseudo-aleatório de 32 bits
    [[nodiscard]] uint_fast32_t generate() override;

    // Define uma nova semente e reinicializa o estado
    void setSeed(uint_fast32_t newSeed) override;

    // Cria uma cópia do gerador (necessário para concorrência)
    [[nodiscard]] std::unique_ptr<PRNG> clone() const override {
        // Cria uma cópia exata, incluindo o estado atual e índice
        return std::make_unique<MersenneTwister>(*this);
    }
};
