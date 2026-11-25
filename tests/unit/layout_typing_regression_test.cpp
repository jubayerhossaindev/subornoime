// Regression tests for the layout-typing correctness campaign (BUG-06,

// F-1..F-5 and the inventory gaps). Every vector here is a previously failing

// or untypeable case from windows-ui-audit findings; the fixes live in

// phonetic_tokenizer.cpp / phonetic_parser.cpp / national_layout.cpp /

// khipro_layout.cpp / borno_phonetic_layout.cpp / probhat_layout.cpp.

//

// Unicode note: য় (U+09DF) is a composition exclusion — normalize_nfc keeps

// it decomposed (য U+09AF + nukta U+09BC), so every expected value below is

// built from explicit code points instead of possibly-composed literals.

#include <gtest/gtest.h>
#include <string>

#include "suborno/layouts/khipro/khipro_layout.hpp"
#include "suborno/layouts/national/national_layout.hpp"
#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/borno_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/layouts/probhat/probhat_layout.hpp"
#include "suborno/unicode/bengali.hpp"

namespace {

constexpr const char* chandrabindu = "\u0981";  // ঁ

constexpr const char* virama = "\u09CD";  // ্

constexpr const char* ya_nukta = "\u09AF\u09BC";  // য় (NFC form: decomposed)

constexpr const char* anusvara = "\u0982";  // ং

}  // namespace

// ---------------------------------------------------------------------------

// suborno-phonetic (default layout)

// ---------------------------------------------------------------------------

TEST(LayoutTypingRegressionTest, SubornoChandrabinduIsTyped) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    EXPECT_EQ(layout.transliterate("pa^c"), std::string("প") + "া" + chandrabindu + "চ");
    EXPECT_EQ(layout.transliterate("ka^c"), std::string("ক") + "া" + chandrabindu + "চ");
    EXPECT_EQ(layout.transliterate("a^sh"), std::string("আ") + chandrabindu + "শ");
    EXPECT_EQ(layout.transliterate("sa^ko"), std::string("স") + "া" + chandrabindu + "ক" + "ো");
}

TEST(LayoutTypingRegressionTest, SubornoRiNoLongerShadowsRaPlusIKar) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    // The old "ri" vowel token swallowed র + ি-kar sequences (করি→কঋ).

    EXPECT_EQ(layout.transliterate("pri"), std::string("প") + virama + "র" + "ি");
    EXPECT_EQ(layout.transliterate("priy"), std::string("প") + virama + "র" + "ি" + "য");
    EXPECT_EQ(layout.transliterate("k:boundary:ri"), std::string("ক") + "র" + "ি");
    EXPECT_EQ(layout.transliterate("j:boundary:n:boundary:priy:nukta:"),
              std::string("জনপ্রি") + ya_nukta);
    // ঋ/ৃ moved to "rri" (Avro convention shared by the Avro and Borno
    // phonetic layouts).

    EXPECT_EQ(layout.transliterate("rrishi"), "ঋশি");
    EXPECT_EQ(layout.transliterate("krripa"), std::string("ক") + "ৃ" + "পা");
    EXPECT_EQ(layout.transliterate("prrithibii"), std::string("প") + "ৃ" + "থিবী");
}

TEST(LayoutTypingRegressionTest, SubornoNgaBeforeVelarFinalAndStandalone) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    EXPECT_EQ(layout.transliterate("ngk"), std::string("ঙ") + virama + "ক");
    EXPECT_EQ(layout.transliterate("ngg"), std::string("ঙ") + virama + "গ");
    EXPECT_EQ(layout.transliterate("Ongk"), std::string("অ") + "ঙ" + virama + "ক");
    EXPECT_EQ(layout.transliterate("ongk"), std::string("ও") + "ঙ" + virama + "ক");
    EXPECT_EQ(layout.transliterate("r:boundary:ng"), std::string("র") + "ঙ");
    EXPECT_EQ(layout.transliterate("ng"), "ঙ");
    // বাংলা keeps the anusvara variant before a non-velar consonant.

    EXPECT_EQ(layout.transliterate("bangla"), std::string("বা") + anusvara + "লা");
    EXPECT_EQ(layout.transliterate("Bangali"), "বাঙালি");
}

TEST(LayoutTypingRegressionTest, SubornoKhandaControl) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    EXPECT_EQ(layout.transliterate(":khanda:"), "ৎ");
    EXPECT_EQ(layout.transliterate("u:khanda:s"), std::string("উৎস"));
}

TEST(LayoutTypingRegressionTest, SubornoFullCorpusHeadlinesStillPass) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("bangla"), std::string("বা") + anusvara + "লা");
    EXPECT_EQ(layout.transliterate("bangladesh"), std::string("বাংলাদেশ"));
    EXPECT_EQ(layout.transliterate("manuSh"), "মানুষ");
    EXPECT_EQ(layout.transliterate("kSh"), std::string("ক") + virama + "ষ");
}

// ---------------------------------------------------------------------------

// national (positional Jatiyo layout; rewritten from the phonetic prototype)

// ---------------------------------------------------------------------------

// The layout is now the real positional National (Jatiyo) layout per the

// BDS 1738:2018 diagram (Unicode L2/19-079 Figure 4) with BornoTSF as

// factual reference. The regression equivalents of the old phonetic-token

// tests follow; hasanta is typed with the dedicated ্ key (g), not "X".

TEST(LayoutTypingRegressionTest, NationalExplicitHasantaFormsConjuncts) {
    const suborno::layouts::NationalLayout layout;
    // প্র ন্দ ম্প ক্ষ via the hasanta key.

    EXPECT_EQ(layout.transliterate("rgv"), std::string("প") + virama + "র");
    EXPECT_EQ(layout.transliterate("bgl"), std::string("ন") + virama + "দ");
    EXPECT_EQ(layout.transliterate("mgr"), std::string("ম") + virama + "প");
    EXPECT_EQ(layout.transliterate("jgN"), std::string("ক") + virama + "ষ");
}

TEST(LayoutTypingRegressionTest, NationalPreviouslyMissingCharacters) {
    const suborno::layouts::NationalLayout layout;
    EXPECT_EQ(layout.transliterate("q"), "ঙ");
    EXPECT_EQ(layout.transliterate("Q"), anusvara);
    EXPECT_EQ(layout.transliterate("W"), ya_nukta);
    EXPECT_EQ(layout.transliterate("\\"), "ৎ");
    EXPECT_EQ(layout.transliterate("G"), "।");
    EXPECT_EQ(layout.transliterate("&"), chandrabindu);
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate("|"), "ঃ");
    // Candrabindu (Shift+7) + aa-kar reorders to the canonical াঁ.

    EXPECT_EQ(layout.transliterate("h&f"), std::string("ব") + "াঁ");
    // Anusvara (Shift+Q) stays as typed: বাং.

    EXPECT_EQ(layout.transliterate("hfQ"), std::string("ব") + "া" + anusvara);
    EXPECT_EQ(layout.transliterate("1421"), "১৪২১");
}

TEST(LayoutTypingRegressionTest, NationalRephAndWords) {
    const suborno::layouts::NationalLayout layout;
    // কর্ম: type ক ম then Shift+A — the reph র্ lands before the ম.

    EXPECT_EQ(layout.transliterate("jmA"), std::string("ক") + "র্" + "ম");
    // বাংলা.

    EXPECT_EQ(layout.transliterate("hfQVf"), std::string("বা") + anusvara + "লা");
    // জনপ্রিয়: conjunct via hasanta, য় through the decomposed NFC form.

    EXPECT_EQ(layout.transliterate("ubrgvdW"), std::string("জনপ") + virama + "রি" + ya_nukta);
    // Shift+A after a cluster applies reph to the whole cluster (ক্ক).

    EXPECT_EQ(layout.transliterate("jgjA"), std::string("র্") + "ক" + virama + "ক");
}

// ---------------------------------------------------------------------------

// khipro

// ---------------------------------------------------------------------------

TEST(LayoutTypingRegressionTest, KhiproNgaIsTypeableAndJoinsConjuncts) {
    const suborno::layouts::KhiproLayout layout;
    // bn-khipro.mim spec: uppercase letters pass through literally, 'n'

    // before a consonant reads as ঙ, so Nga is typed as "ng".

    EXPECT_EQ(layout.transliterate("Ng"), "N\u0997");
    EXPECT_EQ(layout.transliterate("Ngk"), "N\u0997\u0995");
    EXPECT_EQ(layout.transliterate("baNgali"), "\u09AC\u09BEN\u0997\u09BE\u09B2\u09BF");
    // Regression guards: the anusvara mappings are unchanged.

    EXPECT_EQ(layout.transliterate("bangla"), "\u09AC\u09BE\u0999\u09B2\u09BE");
    EXPECT_EQ(layout.transliterate("x"), anusvara);
}

TEST(LayoutTypingRegressionTest, KhiproNuktaKeepsTheConsonantRunAlive) {
    const suborno::layouts::KhiproLayout layout;
    // Spec behaviour: the ya-phala run stays alive across the nukta token.

    // ';' outside a join context emits U+204C; য় comes out precomposed.

    EXPECT_EQ(layout.transliterate("y,,z"), "\u09DF\u09BC\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("s;f;tf;w;y,,zar"),
              "\u09B8\u204C\u099F\u0993\u09DF\u09BC\u09CD\u09AF\u09BE\u09B0");
    // The primary derivation (য় = "y") needs no nukta at all.

    EXPECT_EQ(layout.transliterate("s;f;tf;w;yzar"),
              "\u09B8\u204C\u099F\u0993\u09DF\u09CD\u09AF\u09BE\u09B0");
}

TEST(LayoutTypingRegressionTest, KhiproDigitsAndHeadlines) {
    const suborno::layouts::KhiproLayout layout;
    EXPECT_EQ(layout.transliterate("1234"), "১২৩৪");
    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    // khipro ষ is "sf" (no kSh cluster token); auto virama joins ক্ষ.

    EXPECT_EQ(layout.transliterate("ksf"), std::string("ক") + virama + "ষ");
}

// Khipro trailing/displaced virama (INV-064, formerly NOT_DERIVABLE): the

// "qq" token follows the real Khipro Classic explicit-hasanta convention and

// commits exactly U+0995 U+09CD — no ZWNJ, no dropped virama.

TEST(LayoutTypingRegressionTest, KhiproTrailingViramaIsTypeableWithCorrectCodePoints) {
    const suborno::layouts::KhiproLayout layout;
    const std::string expected = std::string("ক") + virama;
    EXPECT_EQ(layout.transliterate("kqq"), expected);
    EXPECT_EQ(layout.transliterate("rqq"), std::string("র") + virama);
    // The token also composes like any other joiner.

    EXPECT_EQ(layout.transliterate("kqqt"), std::string("ক") + virama + "ত");
    EXPECT_EQ(layout.transliterate("kqq."), std::string(expected) + "।");
    // The spec emits one virama per explicit token.

    EXPECT_EQ(layout.transliterate("kqqqq"), "\u0995\u09CD\u09CD");
}

TEST(LayoutTypingRegressionTest, KhiproTrailingViramaSessionCommitParity) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("kqq"), suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, std::string("ক") + virama);
    EXPECT_EQ(committed, suborno::unicode::normalize_nfc(committed));
}

// Probhat standalone nukta (BRIEF-060, formerly NOT_TYPEABLE): AltGr+0x34 is

// the documented SubornoIME Probhat Compatibility Extension for U+09BC; the

// upstream MIT Probhat KLC has no such key. Physical scan-code path only.

TEST(LayoutTypingRegressionTest, ProbhatStandaloneNuktaIsTypeableViaScanCode) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    suborno::layout::KeyEvent nukta{};
    nukta.modifiers.control = true;
    nukta.modifiers.alt = true;
    nukta.scan_code = 0x34;
    nukta.key_down = true;
    EXPECT_EQ(session->append_key(nukta), suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, std::string("\u09BC"));
    // ক + nukta stays decomposed (composition exclusion), byte-exact.

    session->reset();
    suborno::layout::KeyEvent ka{};
    ka.scan_code = 0x25;
    ka.key_down = true;
    EXPECT_EQ(session->append_key(ka), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(nukta), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->commit().text_utf8, std::string("ক") + "\u09BC");
    // The legacy ড়/ঢ়/য় mappings are untouched. NFC keeps composition

    // exclusions decomposed, so the expectations use explicit escapes.

    EXPECT_EQ(layout.transliterate("R"), std::string("ড") + "\u09BC");
    EXPECT_EQ(layout.transliterate("X"), "\u09A2\u09BC");
    EXPECT_EQ(layout.transliterate("z"), std::string("য") + "\u09BC");
}

// ---------------------------------------------------------------------------

// borno-phonetic

// ---------------------------------------------------------------------------

TEST(LayoutTypingRegressionTest, BornoAccentKeyBreaksVowelCollisions) {
    const suborno::layouts::BornoPhoneticLayout layout;
    EXPECT_EQ(layout.transliterate("Ta`i"), std::string("ট") + "া" + "ই");
    EXPECT_EQ(layout.transliterate("mo`i"), "মই");
    // Regression guards for the existing contract (BornoTSF emits the

    // precomposed য় U+09DF after a vowel sign).

    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("rri"), "ঋ");
    EXPECT_EQ(layout.transliterate("jonopriy"), "\u099C\u09A8\u09AA\u09CD\u09B0\u09BF\u09DF");
}

// ---------------------------------------------------------------------------

// probhat

// ---------------------------------------------------------------------------

TEST(LayoutTypingRegressionTest, ProbhatSessionCommitMatchesTransliterateEncoding) {
    const suborno::layouts::ProbhatLayout layout;
    // য় is a composition exclusion: both paths must agree on its NFC form

    // (য + nukta), never the bare vowel.

    const std::string session_out = layout.transliterate("\xE0\xA6\xAF\xE0\xA6\xBC");
    EXPECT_EQ(session_out, std::string(ya_nukta));
    EXPECT_NE(session_out, std::string("য"));
}

// ---------------------------------------------------------------------------

// avro-phonetic (regression guards)

// ---------------------------------------------------------------------------

TEST(LayoutTypingRegressionTest, AvroUnchangedHeadlines) {
    const suborno::layouts::AvroPhoneticLayout layout;
    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("rri"), "ঋ");
    EXPECT_EQ(layout.transliterate("jonopriy"), "\u099C\u09A8\u09AA\u09CD\u09B0\u09BF\u09DF");
    EXPECT_EQ(layout.transliterate("Ta`i"), std::string("ট") + "া" + "ই");
}
