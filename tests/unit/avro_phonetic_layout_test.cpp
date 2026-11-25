#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "borno_compat_settings.hpp"

// Avro Phonetic expectations: the layout is driven by Suborno's clean-room
// cppAvroPhonetic-behaviour engine (src/layouts/borno_compat), so the
// expected outputs below follow that behaviour contract exactly — including
// the differences from Suborno's earlier simplified token table:
// word-initial "y" is ইয়, "chh" is ছ+হ, "Z" is ্য, "Y" is the precomposed
// য় (U+09DF), and there is no backslash escape in Avro mode.
TEST(AvroPhoneticLayoutTest, ConvertsIndependentAndDependentVowels) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "\u0986");
    EXPECT_EQ(layout.transliterate("ka"), "\u0995\u09BE");
    EXPECT_EQ(layout.transliterate("o"), "\u0985");
    EXPECT_EQ(layout.transliterate("ko"), "\u0995");
    EXPECT_EQ(layout.transliterate("O"), "\u0993");
    EXPECT_EQ(layout.transliterate("kO"), "\u0995\u09CB");
    EXPECT_EQ(layout.transliterate("ee"), "\u0988");
    EXPECT_EQ(layout.transliterate("kee"), "\u0995\u09C0");
    EXPECT_EQ(layout.transliterate("oo"), "\u0989");
    EXPECT_EQ(layout.transliterate("rri"), "\u098B");
    EXPECT_EQ(layout.transliterate("krri"), "\u0995\u09C3");
    EXPECT_EQ(layout.transliterate("OI"), "\u0990");
    EXPECT_EQ(layout.transliterate("OU"), "\u0994");
}

TEST(AvroPhoneticLayoutTest, PreservesCaseSensitiveConsonantTokens) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("c"), "\u099A");
    EXPECT_EQ(layout.transliterate("ch"), "\u099B");
    // cppAvroPhonetic has no "chh" cluster token: ছ + হ.
    EXPECT_EQ(layout.transliterate("chh"), "\u099B\u09B9");
    EXPECT_EQ(layout.transliterate("j"), "\u099C");
    EXPECT_EQ(layout.transliterate("J"), "\u099C");
    // Word-initial "y" is ই + য়; "Y" is the precomposed য় (U+09DF).
    EXPECT_EQ(layout.transliterate("y"), "\u0987\u09DF");
    EXPECT_EQ(layout.transliterate("Y"), "\u09DF");
    // Word-initial "Z" is ্ + য.
    EXPECT_EQ(layout.transliterate("Z"), "\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("kY"), "\u0995\u09DF");
    EXPECT_EQ(layout.transliterate("kZ"), "\u0995\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("t"), "\u09A4");
    EXPECT_EQ(layout.transliterate("T"), "\u099F");
    EXPECT_EQ(layout.transliterate("th"), "\u09A5");
    EXPECT_EQ(layout.transliterate("Th"), "\u09A0");
    EXPECT_EQ(layout.transliterate("Ng"), "\u0999");
    EXPECT_EQ(layout.transliterate("NG"), "\u099E");
    EXPECT_EQ(layout.transliterate("ng"), "\u0982");
}

TEST(AvroPhoneticLayoutTest, HandlesAccentKeyOverrides) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("eki"), "\u098F\u0995\u09BF");
    EXPECT_EQ(layout.transliterate(std::string("ek") + '\x60' + "i"), "\u098F\u0995\u0987");
    EXPECT_EQ(layout.transliterate(std::string("a") + '\x60'), "\u09BE");
    EXPECT_EQ(layout.transliterate(std::string("k") + '\x60' + "i"), "\u0995\u0987");
    EXPECT_EQ(layout.transliterate(std::string("k") + '\x60' + "ta"), "\u0995\u09A4\u09BE");
    EXPECT_EQ(layout.transliterate(std::string(".") + '\x60'), ".");
    EXPECT_EQ(layout.transliterate(std::string(":") + '\x60'), ":");
}

TEST(AvroPhoneticLayoutTest, SupportsRephPhalaAndSpecialSigns) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("orrko"), "\u0985\u09B0\u09CD\u0995");
    EXPECT_EQ(layout.transliterate("by"), "\u09AC\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("kr"), "\u0995\u09CD\u09B0");
    // With phonetic correction enabled the ac.json entry for "km"
    // respells the buffer, so the bare conjunct is asserted with the
    // BornoTSF switch off (same temporary bypass BornoTSF itself uses).
    borno_compat::set_phonetic_correction(false);
    EXPECT_EQ(layout.transliterate("km"), "\u0995\u09CD\u09AE");
    borno_compat::set_phonetic_correction(true);
    EXPECT_EQ(layout.transliterate("rri"), "\u098B");
    EXPECT_EQ(layout.transliterate("ng"), "\u0982");
    EXPECT_EQ(layout.transliterate(":"), "\u0983");
    EXPECT_EQ(layout.transliterate("^"), "\u0981");
    EXPECT_EQ(layout.transliterate("."), "\u0964");
    EXPECT_EQ(layout.transliterate("$"), "\u09F3");
    EXPECT_EQ(layout.transliterate(std::string("t") + '\x60' + '\x60'), "\u09CE");
}

TEST(AvroPhoneticLayoutTest, PreservesLiteralAndUnknownUnicodeText) {
    const suborno::layouts::AvroPhoneticLayout layout;

    // cppAvroPhonetic has no backslash escape branch: a backslash is an
    // unknown character and is emitted verbatim before the parsed rest.
    EXPECT_EQ(layout.transliterate(R"(\k)"), "\\\u0995");
    EXPECT_EQ(layout.transliterate("hellO!"), "\u09B9\u09C7\u09B2\u09CD\u09B2\u09CB!");
    EXPECT_THROW(static_cast<void>(layout.transliterate("\xFF")), std::invalid_argument);
}

TEST(AvroPhoneticLayoutTest, AutocorrectLayerMatchesBornoTSF) {
    // PhoneticCorrection defaults to enabled, exactly like BornoTSF: the
    // same ac.json respelling layer feeds both phonetic layouts.
    const suborno::layouts::AvroPhoneticLayout layout;
    ASSERT_TRUE(borno_compat::get_phonetic_correction());

    EXPECT_EQ(layout.transliterate("dhaka"), "\u09A2\u09BE\u0995\u09BE");
    borno_compat::set_phonetic_correction(false);
    EXPECT_EQ(layout.transliterate("dhaka"), "\u09A7\u09BE\u0995\u09BE");
    borno_compat::set_phonetic_correction(true);

    EXPECT_EQ(layout.transliterate("chatgpt"),
              "\u099A\u09CD\u09AF\u09BE\u099F\u099C\u09BF\u09AA\u09BF\u099F\u09BF");
    EXPECT_EQ(layout.transliterate(":)"), ":)");
}
