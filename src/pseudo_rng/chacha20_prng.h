#pragma once
#include "prng.h"
#include <array>
#include <cstdint>

/* =========================================================================
   ChaCha20-PRNG
   -------------------------------------------------------------------------
   Implementa o gerador pseudo-aleatório baseado no stream-cipher ChaCha20.
   A cada chamada a generate() devolve 32 bits do keystream.
   ========================================================================= */
class ChaCha20PRNG final : public PRNG
{
private:
    /* ---- Constantes de ChaCha20 (“expand 32-byte k”) ---- */
    static constexpr std::array<uint32_t,4> CONSTANT_WORDS_ {
        0x6170'7865u, 0x3320'646eu, 0x7962'2d32u, 0x6b20'6574u
    };

    /* ---- Chave de 256 bits e nonce de 96 bits (RFC 8439) ---- */
    std::array<uint32_t,8> keyWords_  {};   // k0…k7
    std::array<uint32_t,3> nonceWords_{};   // n0…n2

    /* ---- Contador de 64 bits (split em 2 palavras) ---- */
    uint32_t counterLow_  {0};
    uint32_t counterHigh_ {0};

    /* ---- Buffer com o bloco de 512 bits (16×32 bits) ---- */
    std::array<uint32_t,16> keystreamBlock_ {};
    unsigned nextWordIndex_ {16};           // 16 ⇒ força gerar bloco na 1ª chamada

    /* ---------------------------------------------------------------------
       Gera um bloco ChaCha20 (20 rounds) e preenche keystreamBlock_
       --------------------------------------------------------------------- */
    void generateBlock();

    /* ---------------------------------------------------------------------
       Inicializa a chave/nonce a partir da semente fornecida.
       Sem segurança criptográfica forte caso a semente seja pequena,
       mas suficiente como PRNG genérico.
       --------------------------------------------------------------------- */
    void initializeFromSeed(uint_fast32_t seedValue);

    /* ------------------------ Utilitário round --------------------------- */
    static inline void quarterRound(
        uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) noexcept;

public:
    explicit ChaCha20PRNG(uint_fast32_t seed = 0);

    [[nodiscard]] uint_fast32_t generate() override;
    void setSeed(uint_fast32_t newSeed) override;

    [[nodiscard]] std::unique_ptr<PRNG> clone() const override {
        return std::make_unique<ChaCha20PRNG>(*this);
    }
};

