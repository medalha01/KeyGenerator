// ──────────────────────────────────────────────
// pseudo_rng/prng.h
// ──────────────────────────────────────────────
#pragma once

#include <cstdint>
#include <memory>

/**
 * @brief Classe‑base para geradores pseudo‑aleatórios que devolvem
 *        um inteiro de 32 bits a cada chamada de generate().
 */
class PRNG {
protected:
    uint_fast32_t seed_ {0};   // Semente corrente

public:
    PRNG() = default;
    explicit PRNG(uint_fast32_t seed) : seed_{seed} {}
    virtual ~PRNG() = default;

    /// Gera o próximo valor aleatório de 32 bits.
    [[nodiscard]] virtual uint_fast32_t generate() = 0;

    /// Define nova semente; implementações devem reinicializar estado interno.
    virtual void setSeed(uint_fast32_t newSeed) { seed_ = newSeed; }

    /// Construtor polimórfico (clone).
    [[nodiscard]] virtual std::unique_ptr<PRNG> clone() const = 0;
};
