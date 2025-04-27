#pragma once
/*──────────────────────────────────────────────────────────────
 *  Naor–Reingold PRF usada como PRNG determinístico.
 *  Parâmetros P, Q, G e chave a₀…aₙ são fixos (DiceForge demo).
 *  A seed controla apenas o valor de  x  (inputVectorX_).
 *──────────────────────────────────────────────────────────────*/
#include "prng.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <vector>

using BigInt = boost::multiprecision::cpp_int;

class NaorReingoldPRF final : public PRNG
{
    static constexpr unsigned INPUT_DIMENSION = 32;   // bits de x

    /* Parâmetros do grupo */
    const BigInt modulusP_;
    const BigInt subgroupOrderQ_;
    const BigInt generatorG_;

    /* Estado === entrada x da PRF */
    BigInt inputVectorX_;

public:
    explicit NaorReingoldPRF(uint_fast32_t initialSeed = 0);

    uint_fast32_t generate() override;            // 32 bits pseudo-aleatórios
    void setSeed(uint_fast32_t newSeed) override;
    std::unique_ptr<PRNG> clone() const override;
};
