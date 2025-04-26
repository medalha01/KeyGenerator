// pseudo_rng/mersenne_twister.cpp
#include "mersenne_twister.h"
#include <limits> // Para numeric_limits

/* -------------------------------------------------------------------------
   Construtor: inicializa estado com a semente informada.
   ------------------------------------------------------------------------- */
MersenneTwister::MersenneTwister(uint_fast32_t seed)
{
    setSeed(seed);
}

/* -------------------------------------------------------------------------
   Re-semente o gerador (algoritmo original de Matsumoto & Nishimura).
   Usa uint32_t para o estado interno.
   ------------------------------------------------------------------------- */
void MersenneTwister::setSeed(uint_fast32_t newSeed)
{
    seed_ = newSeed;
    // Garante que a semente não é zero, se for, usa um valor padrão.
    stateVector_[0] = (newSeed != 0) ? static_cast<uint32_t>(newSeed) : 5489u;

    for (unsigned i = 1; i < STATE_SIZE; ++i) {
        const uint32_t prev = stateVector_[i - 1]; // Usa uint32_t
        // Cálculo de inicialização padrão do MT
        stateVector_[i] = (SEED_MULTIPLIER * (prev ^ (prev >> (WORD_SIZE - 2))) + i);
        // Os resultados são implicitamente truncados para 32 bits se uint_fast32_t for maior
        // mas ao usar uint32_t para o stateVector_, a atribuição garante isso.
    }
    index_ = STATE_SIZE; // Força twist() na próxima chamada a generate()
}

/* -------------------------------------------------------------------------
   Gera novo bloco de 624 números (fase “twist” do MT).
   Usa uint32_t para operações internas.
   ------------------------------------------------------------------------- */
void MersenneTwister::twist()
{
    for (unsigned i = 0; i < STATE_SIZE; ++i) {
        // Combina bits superiores de state[i] com inferiores de state[(i+1) % N]
        const uint32_t higherBits = stateVector_[i] & UPPER_MASK;
        const uint32_t lowerBits  = stateVector_[(i + 1) % STATE_SIZE] & LOWER_MASK;
        const uint32_t mergedBits = higherBits | lowerBits;

        // Aplica a transformação linear (equivalente a multiplicar por A)
        uint32_t temperedBits = mergedBits >> 1;
        if (mergedBits & 1u) { // Se o bit menos significativo for 1
            temperedBits ^= MATRIX_A; // XOR com a constante mágica A
        }

        // Atualiza o estado usando o valor torcido e um elemento futuro
        stateVector_[i] = stateVector_[(i + TWIST_OFFSET) % STATE_SIZE] ^ temperedBits;
    }
    index_ = 0; // Reseta o índice para o início do novo bloco
}

/* -------------------------------------------------------------------------
   Devolve o próximo valor pseudo-aleatório (32 bits).
   A interface ainda retorna uint_fast32_t, mas a geração interna é uint32_t.
   ------------------------------------------------------------------------- */
uint_fast32_t MersenneTwister::generate()
{
    if (index_ >= STATE_SIZE) {
        // Se todos os números do bloco foram usados, gera um novo bloco
        twist();
    }

    // Pega o próximo número do estado
    uint32_t output = stateVector_[index_++]; // Usa uint32_t

    // Aplica o "tempering" para melhorar a distribuição dos bits
    output ^= (output >> TEMPERING_SHIFT_U); 
    output ^= (output << TEMPERING_SHIFT_S) & TEMPERING_MASK_B;
    output ^= (output << TEMPERING_SHIFT_T) & TEMPERING_MASK_C;
    output ^= (output >> TEMPERING_SHIFT_L);

    return static_cast<uint_fast32_t>(output);
}
