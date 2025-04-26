#include "pseudo_rng/chacha20_prng.h"
#include <cstring>      // std::memcpy
#include <stdexcept>

/* ========================================================================
   Função utilitária: rotação esquerda de 32 bits
   ======================================================================== */
static constexpr uint32_t rotl32(uint32_t value, int shift) noexcept
{
    return (value << shift) | (value >> (32 - shift));
}

/* -------------------------------------------------------------------------
   Quarter-Round original de ChaCha20
   ------------------------------------------------------------------------- */
inline void ChaCha20PRNG::quarterRound(
    uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) noexcept
{
    a += b;  d ^= a;  d = rotl32(d,16);
    c += d;  b ^= c;  b = rotl32(b,12);
    a += b;  d ^= a;  d = rotl32(d, 8);
    c += d;  b ^= c;  b = rotl32(b, 7);
}

/* -------------------------------------------------------------------------
   Construtor: cria instância já pronta para uso
   ------------------------------------------------------------------------- */
ChaCha20PRNG::ChaCha20PRNG(uint_fast32_t seed)
{
    initializeFromSeed(seed);
}

/* -------------------------------------------------------------------------
   (Re)semente o gerador – reinicia contador e buffer
   ------------------------------------------------------------------------- */
void ChaCha20PRNG::setSeed(uint_fast32_t newSeed)
{
    initializeFromSeed(newSeed);
}

/* -------------------------------------------------------------------------
   Inicializa chave (256 bits) e nonce (96 bits) a partir da semente.
   Estratégia simples: expande o seed usando um xorshift64.
   ------------------------------------------------------------------------- */
void ChaCha20PRNG::initializeFromSeed(uint_fast32_t seedValue)
{
    /* --- gera 64 bits de estado para xorshift --- */
    uint64_t xorshiftState = (static_cast<uint64_t>(seedValue) << 32)
                           | (seedValue ^ 0xDEADBEEFu);

    auto xorshift64 = [&]() -> uint32_t {
        xorshiftState ^= xorshiftState << 13;
        xorshiftState ^= xorshiftState >> 7;
        xorshiftState ^= xorshiftState << 17;
        return static_cast<uint32_t>(xorshiftState & 0xFFFFFFFFu);
    };

    for (auto& word : keyWords_)   word = xorshift64();
    for (auto& word : nonceWords_) word = xorshift64();

    counterLow_ = 0;
    counterHigh_ = 0;
    nextWordIndex_ = 16;           // invalida buffer atual
    seed_ = seedValue;
}

/* -------------------------------------------------------------------------
   Gera um bloco completo (16 words) – chamado quando buffer esgota
   ------------------------------------------------------------------------- */
void ChaCha20PRNG::generateBlock()
{
    /* --- Estado inicial (16 words) --------------------------------------- */
    uint32_t state[16] {
        CONSTANT_WORDS_[0], CONSTANT_WORDS_[1],
        CONSTANT_WORDS_[2], CONSTANT_WORDS_[3],

        keyWords_[0], keyWords_[1], keyWords_[2], keyWords_[3],
        keyWords_[4], keyWords_[5], keyWords_[6], keyWords_[7],

        counterLow_, counterHigh_,                 // contador de 64 bits
        nonceWords_[0], nonceWords_[1], nonceWords_[2]
    };

    /* --- Cópia para trabalhar --- */
    uint32_t workingState[16];
    std::memcpy(workingState, state, sizeof(state));

    /* --- 20 rounds = 10 pares de (colunas + diagonais) ------------------- */
    for (int i = 0; i < 10; ++i) {
        /* Colunas */
        quarterRound(workingState[0],  workingState[4],
                     workingState[8],  workingState[12]);
        quarterRound(workingState[1],  workingState[5],
                     workingState[9],  workingState[13]);
        quarterRound(workingState[2],  workingState[6],
                     workingState[10], workingState[14]);
        quarterRound(workingState[3],  workingState[7],
                     workingState[11], workingState[15]);

        /* Diagonais */
        quarterRound(workingState[0],  workingState[5],
                     workingState[10], workingState[15]);
        quarterRound(workingState[1],  workingState[6],
                     workingState[11], workingState[12]);
        quarterRound(workingState[2],  workingState[7],
                     workingState[8],  workingState[13]);
        quarterRound(workingState[3],  workingState[4],
                     workingState[9],  workingState[14]);
    }

    /* --- Soma original + output e grava no buffer ------------------------ */
    for (int i = 0; i < 16; ++i)
        keystreamBlock_[i] = workingState[i] + state[i];

    /* --- Avança contador (64 bits) -------------------------------------- */
    if (++counterLow_ == 0) ++counterHigh_;

    nextWordIndex_ = 0;
}

/* -------------------------------------------------------------------------
   Devolve 32 bits pseudo-aleatórios
   ------------------------------------------------------------------------- */
uint_fast32_t ChaCha20PRNG::generate()
{
    if (nextWordIndex_ >= 16) generateBlock();
    return keystreamBlock_[nextWordIndex_++];
}

