#pragma once
#include "prng.h"
#include <array>

/* =========================================================================
   Implementação do gerador MT19937 (32 bits) — Mersenne Twister.
   ========================================================================= */
class MersenneTwister final : public PRNG
{
private:

    static constexpr unsigned WORD_SIZE          = 32;
    static constexpr unsigned STATE_SIZE         = 624;
    static constexpr unsigned TWIST_OFFSET       = 397;
    static constexpr uint32_t MATRIX_A           = 0x9908B0DFu;
    static constexpr uint32_t UPPER_MASK         = 0x80000000u;
    static constexpr uint32_t LOWER_MASK         = 0x7FFFFFFFu;


    static constexpr unsigned TEMPERING_SHIFT_U  = 11;
    static constexpr uint32_t TEMPERING_MASK_D   = 0xFFFFFFFFu;
    static constexpr unsigned TEMPERING_SHIFT_S  = 7;
    static constexpr uint32_t TEMPERING_MASK_B   = 0x9D2C5680u;
    static constexpr unsigned TEMPERING_SHIFT_T  = 15;
    static constexpr uint32_t TEMPERING_MASK_C   = 0xEFC60000u;
    static constexpr unsigned TEMPERING_SHIFT_L  = 18;

    static constexpr uint32_t SEED_MULTIPLIER    = 1812433253u;


    std::array<uint_fast32_t, STATE_SIZE> stateVector_ {};
    unsigned index_ {STATE_SIZE};                      // Força primeira twist()


    void twist();                                    

public:
    explicit MersenneTwister(uint_fast32_t seed = 5489u);     // 5489 é o default do MT
    [[nodiscard]] uint_fast32_t generate() override;
    void setSeed(uint_fast32_t newSeed) override;

    [[nodiscard]] std::unique_ptr<PRNG> clone() const override {
        return std::make_unique<MersenneTwister>(*this);
    }
};

