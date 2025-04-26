#include "pseudo_rng/mersenne_twister.h"

/* -------------------------------------------------------------------------
   Construtor: inicializa estado com a semente informada.
   ------------------------------------------------------------------------- */
MersenneTwister::MersenneTwister(uint_fast32_t seed)
{
    setSeed(seed);
}

/* -------------------------------------------------------------------------
   Re-semente o gerador (algoritmo original de Matsumoto & Nishimura).
   ------------------------------------------------------------------------- */
void MersenneTwister::setSeed(uint_fast32_t newSeed)
{
    seed_          = newSeed;
    stateVector_[0] = newSeed;

    for (unsigned i = 1; i < STATE_SIZE; ++i) {
        const uint_fast32_t prev = stateVector_[i - 1];
        stateVector_[i] = SEED_MULTIPLIER * (prev ^ (prev >> (WORD_SIZE - 2))) + i;
    }
    index_ = STATE_SIZE;                               // Força twist() na próxima chamada
}

/* -------------------------------------------------------------------------
   Gera novo bloco de 624 números (fase “twist” do MT).
   ------------------------------------------------------------------------- */
void MersenneTwister::twist()
{
    for (unsigned i = 0; i < STATE_SIZE; ++i) {
        const uint_fast32_t higherBits = stateVector_[i] & UPPER_MASK;
        const uint_fast32_t lowerBits  = stateVector_[(i + 1) % STATE_SIZE] & LOWER_MASK;
        const uint_fast32_t mergedBits = higherBits | lowerBits;

        uint_fast32_t temperedBits = mergedBits >> 1;
        if (mergedBits & 1u) temperedBits ^= MATRIX_A;

        stateVector_[i] = stateVector_[(i + TWIST_OFFSET) % STATE_SIZE] ^ temperedBits;
    }
    index_ = 0;
}

/* -------------------------------------------------------------------------
   Devolve o próximo valor pseudo-aleatório (32 bits).
   ------------------------------------------------------------------------- */
uint_fast32_t MersenneTwister::generate()
{
    if (index_ >= STATE_SIZE) twist();

    uint_fast32_t output = stateVector_[index_++];

    // Tempering: melhora a distribuição de bits de saída
    output ^= (output >> TEMPERING_SHIFT_U) & TEMPERING_MASK_D;
    output ^= (output << TEMPERING_SHIFT_S) & TEMPERING_MASK_B;
    output ^= (output << TEMPERING_SHIFT_T) & TEMPERING_MASK_C;
    output ^=  output >> TEMPERING_SHIFT_L;

    return output;
}
