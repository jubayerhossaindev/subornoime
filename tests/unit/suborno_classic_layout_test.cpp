#include "suborno/layouts/phonetic/suborno_classic_layout.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "compat_settings.hpp"

// Suborno Classic expectations: the layout is driven by Suborno's clean-room
// engine (src/layouts/compat), whose behaviour contract follows the
// published "Suborno Classic" reference — including the differences from
// Suborno's earlier simplified token table ("ee" is NOT ঈ; the backslash
// has count-based literal escaping; the raw-keys candidate joins the
// candidate list).
TEST(SubornoClassicLayoutTest, ConvertsIndependentAndDependentVowels) {
    const suborno::layouts::SubornoClassicLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "\u0986");
    EXPECT_EQ(layout.transliterate("ka"), "\u0995\u09BE");
    EXPECT_EQ(layout.transliterate("o"), "\u0985");
    EXPECT_EQ(layout.transliterate("ko"), "\u0995");
    EXPECT_EQ(layout.transliterate("O"), "\u0993");
    EXPECT_EQ(layout.transliterate("kO"), "\u0995\u09CB");
    // "ee" is e+e (এ এ), not ঈ; the vowel sign is taken from the vowel-sign
    // map and the trailing vowel becomes independent.
    EXPECT_EQ(layout.transliterate("ee"), "\u098F\u098F");
    EXPECT_EQ(layout.transliterate("kee"), "\u0995\u09C7\u098F");
    EXPECT_EQ(layout.transliterate("rri"), "\u098B");
    EXPECT_EQ(layout.transliterate("krri"), "\u0995\u09C3");
}

TEST(SubornoClassicLayoutTest, PreservesCaseSensitiveConsonantTokens) {
    const suborno::layouts::SubornoClassicLayout layout;

    EXPECT_EQ(layout.transliterate("t"), "\u09A4");
    EXPECT_EQ(layout.transliterate("T"), "\u099F");
    EXPECT_EQ(layout.transliterate("th"), "\u09A5");
    EXPECT_EQ(layout.transliterate("Th"), "\u09A0");
    EXPECT_EQ(layout.transliterate("d"), "\u09A6");
    EXPECT_EQ(layout.transliterate("D"), "\u09A1");
    EXPECT_EQ(layout.transliterate("dh"), "\u09A7");
    EXPECT_EQ(layout.transliterate("Dh"), "\u09A2");
    EXPECT_EQ(layout.transliterate("ng"), "\u0982");
    EXPECT_EQ(layout.transliterate("Ng"), "\u0999");
    EXPECT_EQ(layout.transliterate("NG"), "\u099E");
    EXPECT_EQ(layout.transliterate("y"), "\u09DF");
}

TEST(SubornoClassicLayoutTest, ComposesConsonantRunsAndExplicitControls) {
    const suborno::layouts::SubornoClassicLayout layout;

    EXPECT_EQ(layout.transliterate("kt"), "\u0995\u09CD\u09A4");
    EXPECT_EQ(layout.transliterate("k,,t"), "\u0995\u09CD\u09A4");
    EXPECT_EQ(layout.transliterate("k,.t"), "\u0995\u09CD\u200C\u09A4");
    EXPECT_EQ(layout.transliterate("k,..t"), "\u0995\u09CD\u200D\u09A4");
    EXPECT_EQ(layout.transliterate("D./"), "\u09A1\u09BC");
    EXPECT_EQ(layout.transliterate("0123456789"),
              "\u09E6\u09E7\u09E8\u09E9\u09EA\u09EB\u09EC\u09ED\u09EE\u09EF");
    // legacy conjunct-map entries through the compatibility engine.
    EXPECT_EQ(layout.transliterate("kkh"), "\u0995\u09CD\u09B7");
    EXPECT_EQ(layout.transliterate("ngOI"), "\u0999\u09CD\u0997\u09C8");
    EXPECT_EQ(layout.transliterate("bangladesh"),
              "\u09AC\u09BE\u0982\u09B2\u09BE\u09A6\u09C7\u09B6");
}

TEST(SubornoClassicLayoutTest, EscapesLiteralGraphemesAndPreservesUnknownText) {
    const suborno::layouts::SubornoClassicLayout layout;

    // legacy reference IME count-based literal escaping: a single backslash is emitted
    // literally and the remaining text still parses.
    EXPECT_EQ(layout.transliterate(R"(\k)"), "\\\u0995");
    EXPECT_EQ(layout.transliterate("hellO!"), "\u09B9\u09C7\u09B2\u09CD\u09B2\u09CB!");
    EXPECT_THROW(static_cast<void>(layout.transliterate("\xFF")), std::invalid_argument);
}

TEST(SubornoClassicLayoutTest, ExposesIndependentCandidateIdentity) {
    const suborno::layouts::SubornoClassicLayout layout;

    // legacy reference IME _HandleCompositionInputWorker semantics: the phonetic output
    // first, then the raw typed keys as the "typed" fallback candidate.
    const auto candidates = layout.generate_candidates("ami");
    ASSERT_EQ(candidates.size(), 2U);
    EXPECT_EQ(candidates[0].text, "\u0986\u09AE\u09BF");
    EXPECT_EQ(candidates[0].source, suborno::layout::CandidateSource::layout);
    EXPECT_EQ(candidates[0].stable_id, "suborno-classic:\u0986\u09AE\u09BF");
    EXPECT_EQ(candidates[1].text, "ami");
    EXPECT_EQ(candidates[1].stable_id, "suborno-classic:ami");
}

TEST(SubornoClassicLayoutTest, AutocorrectLayerMatchesLegacyEngine) {
    // PhoneticCorrection defaults to enabled, exactly like legacy reference IME's
    // registry default: ac.json hits respell the buffer before parsing and
    // emoticon entries pass through untouched.
    const suborno::layouts::SubornoClassicLayout layout;
    ASSERT_TRUE(compat::get_phonetic_correction());

    // dhaka -> "Dhaka" -> ঢাকা (with correction), ধাকা without.
    EXPECT_EQ(layout.transliterate("dhaka"), "\u09A2\u09BE\u0995\u09BE");
    compat::set_phonetic_correction(false);
    EXPECT_EQ(layout.transliterate("dhaka"), "\u09A7\u09BE\u0995\u09BE");
    compat::set_phonetic_correction(true);

    // facebook -> "feisbuk" -> ফেইসবুক; emoticons pass through verbatim.
    EXPECT_EQ(layout.transliterate("facebook"), "\u09AB\u09C7\u0987\u09B8\u09AC\u09C1\u0995");
    EXPECT_EQ(layout.transliterate(":)"), ":)");
    EXPECT_EQ(layout.transliterate("xD"), "xD");
}
