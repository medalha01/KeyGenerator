/*──────────────────────────────────────────────────────────────
 *  Implementação da PRF de Naor–Reingold como gerador.
 *
 *  Fórmula original:
 *      f(x) = (g^{a₀})^{ Π_{i | xᵢ = 1} aᵢ }   (mod P)
 *
 *  • modulusP_         == P     (primo)
 *  • subgroupOrderQ_   == Q     (ordem do sub-grupo)
 *  • generatorG_       == g
 *  • fixedKeysA[i]     == aᵢ
 *
 *──────────────────────────────────────────────────────────────*/
#include "naor_reingold_prf.h"
#include <vector>
#include <stdexcept>

namespace {
/* Parâmetros DiceForge */
const BigInt DEMO_MODULUS_P("4279969613");
const BigInt DEMO_SUBGROUP_Q("9999929");
const BigInt DEMO_GENERATOR_G("9999918");

/* Chave fixa a₀ … a₃₂  (tamanho = INPUT_DIMENSION + 1) */
const std::vector<BigInt> fixedKeysA = {
    BigInt("650051"), BigInt("3948705"), BigInt("3142325"), BigInt("4036110"),
    BigInt("1141941"), BigInt("5739231"), BigInt("5725758"), BigInt("8299330"),
    BigInt("1776388"), BigInt("1423550"), BigInt("9260804"), BigInt("156410"),
    BigInt("1190436"), BigInt("61218"),  BigInt("2382500"), BigInt("1738876"),
    BigInt("7978879"), BigInt("6010478"), BigInt("310917"),  BigInt("4280253"),
    BigInt("24724"),   BigInt("7087659"), BigInt("796099"),  BigInt("8383655"),
    BigInt("7638286"), BigInt("1390415"), BigInt("7899225"), BigInt("5628976"),
    BigInt("1472292"), BigInt("4284966"), BigInt("9708041"), BigInt("4179835"),
    BigInt("3635954")
};
} // namespace

NaorReingoldPRF::NaorReingoldPRF(uint_fast32_t initialSeed)
    : PRNG(initialSeed),
      modulusP_(DEMO_MODULUS_P),
      subgroupOrderQ_(DEMO_SUBGROUP_Q),
      generatorG_(DEMO_GENERATOR_G),
      inputVectorX_(initialSeed)
{
    /* Verifica se os parâmetros são válidos */

    if ((modulusP_ - 1) % subgroupOrderQ_ != 0 ||
        fixedKeysA.size() != INPUT_DIMENSION + 1)
        throw std::runtime_error("Invalid Naor-Reingold parameters");
}

void NaorReingoldPRF::setSeed(uint_fast32_t newSeed)
{
    PRNG::setSeed(newSeed);
    inputVectorX_ = newSeed;
}

uint_fast32_t NaorReingoldPRF::generate()
{
    /* Produto do expoente  Π_{i | xᵢ=1} aᵢ  (mod Q) */
    BigInt exponentProduct = 1;
    /* Percorre os bits de x  (x = x₀, x₁, x₂, ...) */
    for (unsigned bitIndex = 0; bitIndex < INPUT_DIMENSION; ++bitIndex)
        if (boost::multiprecision::bit_test(inputVectorX_, bitIndex))
            exponentProduct = (exponentProduct * fixedKeysA[bitIndex + 1])
                              % subgroupOrderQ_; 

    /* Base g^{a₀} (mod P) */
    BigInt preComputedBase =
        boost::multiprecision::powm(generatorG_,
                                    fixedKeysA[0],
                                    modulusP_);

    /* Resultado final  (preBase)^{exponentProduct} (mod P) */
    BigInt prfValue =
        boost::multiprecision::powm(preComputedBase,
                                    exponentProduct,
                                    modulusP_);

    /* Atualiza o vetor de entrada x  (x ← x+1) */
    ++inputVectorX_;   
    //                                            // x ← x+1
    return static_cast<uint_fast32_t>(prfValue & 0xFFFFFFFFu);   // 32 bits
}

std::unique_ptr<PRNG> NaorReingoldPRF::clone() const
{
    return std::make_unique<NaorReingoldPRF>(*this);
}
