#pragma once
#include "prng.h"
#include <cstdint>
#include <stdexcept>

/* =========================================================================
   Gerador Blum-Blum-Shub: segurança baseada na dificuldade do resíduo
   quadrático. Usa módulo n = primeP * primeQ, com primeP ≡ primeQ ≡ 3 (mod 4)
   ========================================================================= */
class BlumBlumShub final : public PRNG
{
private:
    /* --- Primos pré-computados (mantidos pequenos para exemplo) --- */
    static constexpr uint64_t primeP_ = 100127u;        // 3 mod 4
    static constexpr uint64_t primeQ_ = 100183u;        // 3 mod 4
    static constexpr uint64_t modulusN_ = primeP_ * primeQ_;

    uint_fast64_t state_ {0};

    static uint_fast64_t gcd(uint_fast64_t a, uint_fast64_t b)
    {
        while (b != 0) { uint_fast64_t tmp = b; b = a % b; a = tmp; }
        return a;
    }

    /* ---------------------------------------------------------------------
       Inicializa estado garantindo mdc(seed, n) = 1
       --------------------------------------------------------------------- */
    void initializeState()
    {
        if (seed_ == 0 || seed_ >= modulusN_ || gcd(seed_, modulusN_) != 1)
            state_ = 7641693654ull;                    // Semente default segura
        else
            state_ = seed_;
    }

public:
    explicit BlumBlumShub(uint_fast32_t seed = 7641693654ull) : PRNG(seed) {
        initializeState();
    }

    /* ---------------------------------------------------------------------
       Gera 32 bits (um a um, pois BBS produz 1 bit por iteração).
       --------------------------------------------------------------------- */
    [[nodiscard]] uint_fast32_t generate() override
    {
        uint_fast32_t output {0};
        for (int i = 0; i < 32; ++i) {
            state_ = (state_ * state_) % modulusN_;    // x_{i+1} = x_i² mod n
            output = (output << 1) | (state_ & 1u);    // LSB vira próximo bit
        }
        return output;
    }

    void setSeed(uint_fast32_t newSeed) override {
        PRNG::setSeed(newSeed);
        initializeState();
    }

    [[nodiscard]] std::unique_ptr<PRNG> clone() const override {
        return std::make_unique<BlumBlumShub>(*this);
    }
};

