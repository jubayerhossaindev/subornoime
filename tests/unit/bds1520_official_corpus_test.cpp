// BDS 1520 obtained-edition official corpus tests (BDS FINALIZATION PHASE 2,

// 2026-11-24).

//

// The corpus is transcribed from the OFFICIAL document — "SPECIFICATION FOR

// BANGLA CODED CHARACTER SET FOR INFORMATION INTERCHANGE (Second Revision)"

// (headers "BDS 1520:2011"), Table 2 "Graphic Character Allocations" —

// obtained from the Bangladesh Computer Council official file store and

// verified against unicodedata + the 579-row engine harness by

// scripts/layout_verify/bds1520_official_corpus.py. The current BDS

// 1520:2018 (Third Revision) full text is still store-only (BLOCKED), so the

// clause binding of these tests is the OBTAINED edition, not the 2018 edition.

//

// These tests verify the engine against the official allocation facts:

//   1. Offset rule: 8-bit code NN <-> Unicode scalar U+09NN, with the two

//      documented Daari exceptions (E4 -> U+0964, E5 -> U+0965).

//   2. Every official character round-trips through the engine unicode

//      module: valid UTF-8, exactly one code point, NFC-idempotent, NFC

//      never alters the bytes.

//   3. UTF-16 representation: every official character is a BMP scalar and

//      therefore exactly one UTF-16 code unit equal to the scalar.

//   4. Engine block classification: block characters are recognised by the

//      engine's Bengali classifier; danda characters are valid UTF-8 but are

//      intentionally outside the U+0980..U+09FF block.

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

#include "bds1520_corpus_data.inc"
#include "suborno/unicode/bengali.hpp"

namespace {

// The three precomposed letters in the official repertoire are canonical

// composition exclusions in Unicode: NFC maps each to its canonical

// decomposition and never back. This is the documented engine behaviour

// (matrix row CV-10) and must hold for exactly these three rows.

struct CompositionExclusion {
    unsigned int bds_hex;
    unsigned int scalar;
    const char* decomposed_utf8;
};

constexpr CompositionExclusion bds1520_composition_exclusions[] = {
    {0xDC, 0x09DC, "\xE0\xA6\xA1\xE0\xA6\xBC"},  // ড় -> ড U+09A1 + ় U+09BC

    {0xDD, 0x09DD, "\xE0\xA6\xA2\xE0\xA6\xBC"},  // ঢ় -> ঢ U+09A2 + ় U+09BC

    {0xDF, 0x09DF, "\xE0\xA6\xAF\xE0\xA6\xBC"},  // য় -> য U+09AF + ় U+09BC

};

bool is_composition_exclusion(unsigned int bds_hex) {
    for (const auto& e : bds1520_composition_exclusions) {
        if (e.bds_hex == bds_hex)
            return true;
    }
    return false;
}

struct RowCheck {
    unsigned int bds_hex;
    unsigned int scalar;
};

// Clause 6.1 + Table 2: the printed 8-bit code position identifies the same

// character as the Unicode scalar 0x0900 + code — except the two Daari rows,

// which share the danda characters used by Bengali (U+0964/U+0965).

unsigned int expected_scalar(unsigned int bds_hex) {
    if (bds_hex == 0xE4)
        return 0x0964;
    if (bds_hex == 0xE5)
        return 0x0965;
    return 0x0900 + bds_hex;
}

unsigned int decode_first_scalar(const std::string& utf8) {
    // All corpus characters are 3-byte UTF-8 BMP sequences.

    EXPECT_EQ(utf8.size(), 3U);
    const auto b0 = static_cast<unsigned char>(utf8[0]);
    const auto b1 = static_cast<unsigned char>(utf8[1]);
    const auto b2 = static_cast<unsigned char>(utf8[2]);
    EXPECT_EQ(b0 & 0xF0, 0xE0);
    EXPECT_EQ(b1 & 0xC0, 0x80);
    EXPECT_EQ(b2 & 0xC0, 0x80);
    return (static_cast<unsigned int>(b0 & 0x0F) << 12)
           | (static_cast<unsigned int>(b1 & 0x3F) << 6) | static_cast<unsigned int>(b2 & 0x3F);
}

}  // namespace

TEST(Bds1520OfficialCorpusTest, CorpusRowCountMatchesOfficialTable) {
    // Table 2 transcribes 94 allocation rows: 92 Bengali-block characters +

    // the two Daari rows sharing U+0964/U+0965. Clause 5's own count

    // statement ("90 graphic characters") does not match the printed Table 2

    // (94 rows) — recorded as an internal inconsistency of the obtained

    // edition, NOT silently reconciled here.

    EXPECT_EQ(bds1520_row_count, 94U);
    unsigned int block = 0;
    unsigned int danda = 0;
    for (const auto& row : bds1520_rows) {
        if (row.scalar == 0x0964 || row.scalar == 0x0965) {
            ++danda;
        } else {
            ++block;
        }
    }
    EXPECT_EQ(block, 92U);
    EXPECT_EQ(danda, 2U);
}

TEST(Bds1520OfficialCorpusTest, OffsetRuleMapsEightBitCodeToUnicodeScalar) {
    for (const auto& row : bds1520_rows) {
        EXPECT_EQ(row.scalar, expected_scalar(row.bds_hex))
            << "8-bit code 0x" << std::hex << row.bds_hex;
        EXPECT_EQ(decode_first_scalar(row.utf8), row.scalar)
            << "UTF-8 bytes of row 0x" << std::hex << row.bds_hex;
    }
}

TEST(Bds1520OfficialCorpusTest, EveryOfficialCharacterRoundTripsThroughEngine) {
    for (const auto& row : bds1520_rows) {
        const std::string text(row.utf8);
        ASSERT_TRUE(suborno::unicode::is_valid_utf8(text)) << "row 0x" << std::hex << row.bds_hex;
        ASSERT_EQ(suborno::unicode::count_utf8_code_points(text), 1U)
            << "row 0x" << std::hex << row.bds_hex;
        if (is_composition_exclusion(row.bds_hex)) {
            // NFC decomposes the precomposed letter to its canonical

            // decomposition and the result is NFC-stable (composition

            // exclusion — never recomposed).

            const std::string nfc = suborno::unicode::normalize_nfc(text);
            const std::string decomp(bds1520_composition_exclusions[(row.bds_hex == 0xDC)   ? 0
                                                                    : (row.bds_hex == 0xDD) ? 1
                                                                                            : 2]
                                         .decomposed_utf8);
            EXPECT_EQ(nfc, decomp) << "row 0x" << std::hex << row.bds_hex;
            EXPECT_EQ(suborno::unicode::normalize_nfc(nfc), nfc)
                << "row 0x" << std::hex << row.bds_hex;
            // The decomposed form itself stays decomposed under NFC.

            EXPECT_EQ(suborno::unicode::normalize_nfc(decomp), decomp)
                << "row 0x" << std::hex << row.bds_hex;
        } else {
            // NFC idempotence: normalization never changes an already-NFC

            // repertoire character and never swallows it.

            const std::string nfc = suborno::unicode::normalize_nfc(text);
            ASSERT_EQ(nfc, text) << "row 0x" << std::hex << row.bds_hex;
            const std::string nfc2 = suborno::unicode::normalize_nfc(nfc);
            ASSERT_EQ(nfc2, nfc) << "row 0x" << std::hex << row.bds_hex;
        }
    }
}

TEST(Bds1520OfficialCorpusTest, Utf16SingleUnitForEveryBmpAllocation) {
    for (const auto& row : bds1520_rows) {
        const unsigned int scalar = decode_first_scalar(row.utf8);
        ASSERT_LT(scalar, 0x10000U) << "row 0x" << std::hex << row.bds_hex << " must be BMP";
        // One UTF-16 code unit, value equal to the scalar; no surrogate use

        // anywhere in the official repertoire.

        const std::u16string u16{static_cast<char16_t>(scalar)};
        ASSERT_EQ(u16.size(), 1U);
        ASSERT_EQ(static_cast<unsigned int>(u16[0]), scalar);
        ASSERT_FALSE(scalar >= 0xD800 && scalar <= 0xDFFF) << "surrogate range must never occur";
    }
}

TEST(Bds1520OfficialCorpusTest, EngineClassificationOfOfficialRepertoire) {
    for (const auto& row : bds1520_rows) {
        if (row.scalar >= 0x0980 && row.scalar <= 0x09FF) {
            EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(row.scalar))
                << "row 0x" << std::hex << row.bds_hex << " (U+" << row.scalar
                << ") should classify as Bengali";
        } else {
            // The Daari rows live at U+0964/U+0965 by design: valid text for

            // the engine, but outside the Bengali block classification.

            EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(row.scalar)) << "U+" << row.scalar;
        }
    }
}
