// Focused coverage tests for the unicode inventory, the Bijoy edge paths and
// the English passthrough layout's empty-input branch. Unicode/NFC correctness
// itself is covered by unicode_test.cpp, bijoy_test.cpp and the linguistic
// regression suite; this file closes the remaining line/branch gaps.

#include <gtest/gtest.h>
#include <string>

#include "suborno/layouts/english/english_layout.hpp"
#include "suborno/unicode/bengali.hpp"
#include "suborno/unicode/bijoy.hpp"

namespace {

using suborno::layouts::EnglishLayout;
using suborno::unicode::encode_bijoy;
using suborno::unicode::is_bijoy_encodable;
using suborno::unicode::supported_bengali_inventory;

}  // namespace

TEST(CoverageUnicodeTest, SupportedBengaliInventoryMatchesCatalog) {
    const std::vector<suborno::unicode::BengaliCharacterInfo> inventory =
        supported_bengali_inventory();
    // The catalog is a product contract: vowels, consonants, signs, the
    // rupee sign and the ten Bengali digits, in list order.
    ASSERT_EQ(inventory.size(), 66U);

    EXPECT_EQ(inventory.front().character, "অ");
    EXPECT_EQ(inventory.front().name, "Bengali Letter A");
    EXPECT_EQ(inventory.front().category, "Lo");
    EXPECT_EQ(inventory.front().romanization, "a");

    EXPECT_EQ(inventory[10].character, "ক");
    EXPECT_EQ(inventory[10].romanization, "k");

    // The vowel sign block keeps combining semantics (Mn).
    EXPECT_EQ(inventory[45].character, "া");
    EXPECT_EQ(inventory[45].category, "Mn");

    // Currency + the digit run ০..৯ close the list.
    EXPECT_EQ(inventory[55].character, "৳");
    EXPECT_EQ(inventory[55].category, "Sc");
    EXPECT_EQ(inventory[56].character, "০");
    EXPECT_EQ(inventory.back().character, "৯");
    EXPECT_EQ(inventory.back().name, "Bengali Digit Nine");

    for (const auto& entry : inventory) {
        EXPECT_FALSE(entry.character.empty());
        EXPECT_FALSE(entry.name.empty());
        EXPECT_FALSE(entry.category.empty());
    }
}

TEST(CoverageUnicodeTest, EnglishLayoutGenerateCandidatesRejectsEmptyInput) {
    EnglishLayout layout;
    EXPECT_TRUE(layout.generate_candidates("").empty());

    // Non-empty Latin input echoes back as the single identity candidate.
    const std::vector<suborno::layout::Candidate> candidates = layout.generate_candidates("hello");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates[0].text, "hello");
    EXPECT_EQ(candidates[0].normalized_text, "hello");
}

TEST(CoverageUnicodeTest, BijoyEncodesAndFlagsDecomposedNuqtaOnOrdinaryConsonant) {
    // ক + U+09BC: the nuqta mark is present but ক is not a nuqta letter
    // (ড/ঢ/য are), so the decomposed-nuqta matcher must decline and the
    // plain mapping is used; the unmatched mark itself has no Bijoy glyph.
    const std::string plain = encode_bijoy("ক");
    const std::string with_nuqta = encode_bijoy("ক\u09BC");
    EXPECT_EQ(with_nuqta, plain);
    EXPECT_EQ(with_nuqta, "K");

    // The unmatched nuqta mark is not part of the Bijoy inventory, so the
    // encoding is lossy and is_bijoy_encodable must say so.
    EXPECT_FALSE(is_bijoy_encodable("ক\u09BC"));
}

TEST(CoverageUnicodeTest, BijoyEncodabilityChecksInvalidUtf8AndValidBangla) {
    // Truncated multibyte sequence is rejected before any table lookup.
    EXPECT_FALSE(is_bijoy_encodable("ক\xC3"));

    // Ordinary Bangla words, ASCII passthrough and the zero-width joiners
    // (removed during encoding) are all loss-free.
    EXPECT_TRUE(is_bijoy_encodable("কলম"));
    EXPECT_TRUE(is_bijoy_encodable("ami bhai"));
    EXPECT_TRUE(is_bijoy_encodable("ক\u200Cলম"));

    // Invalid UTF-8 is also returned unchanged by the encoder (documented
    // commit-path passthrough).
    EXPECT_EQ(encode_bijoy("ক\xC3"), "ক\xC3");
}
