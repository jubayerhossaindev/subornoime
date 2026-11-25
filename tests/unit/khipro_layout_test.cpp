#include "suborno/layouts/khipro/khipro_layout.hpp"

#include <gtest/gtest.h>
#include <string>

TEST(KhiproLayoutTest, BasicConsonantAndVowelComposition) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::compositional);
    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("kaka"), "কাকা");
    // bn-khipro.mim spec: n before a consonant becomes ঙ (BornoTSF Khipro

    // behaviour) — বাঙলা, not বাংলা.

    EXPECT_EQ(layout.transliterate("bangla"), "বাঙলা");
    EXPECT_EQ(layout.transliterate("tfaka"), "টাকা");
    EXPECT_EQ(layout.transliterate("dffaka"), "ঢাকা");
}

TEST(KhiproLayoutTest, AutomaticallyJoinsAdjacentConsonants) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("kt"), "ক্ত");
    EXPECT_EQ(layout.transliterate("k;t"), "কত");
    EXPECT_EQ(layout.transliterate("kor"), "কর");
}

TEST(KhiproLayoutTest, SupportsForcedVowelForms) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("amii"), "আমী");
    EXPECT_EQ(layout.transliterate("amiif"), "আমিই");
    EXPECT_EQ(layout.transliterate("valwi"), "ভালৈ");
    EXPECT_EQ(layout.transliterate("valwif"), "ভালোই");
    EXPECT_EQ(layout.transliterate("nwuka"), "নৌকা");
    EXPECT_EQ(layout.transliterate("nwufka"), "নোউকা");
}

TEST(KhiproLayoutTest, SupportsSlicerAndChandrabindu) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("ta/der"), "তাঁদের");
    EXPECT_EQ(layout.transliterate("bakr/uddh"), "বাকরুদ্ধ");
}

TEST(KhiproLayoutTest, SupportsSpecialCharactersAndLiteralSeparator) {
    const suborno::layouts::KhiproLayout layout;

    // The mim spec keeps an unmapped ';' verbatim (no collapse to one).

    EXPECT_EQ(layout.transliterate(";;"), ";;");
    EXPECT_EQ(layout.transliterate("x"), "ং");
    EXPECT_EQ(layout.transliterate("oo"), "ঃ");
    EXPECT_EQ(layout.transliterate("t/"), "ৎ");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate(".f"), "॥");
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate("$f"), "₹");
}

TEST(KhiproLayoutTest, CreatesCompositionSessionsAndCommitsTheSelectedOutput) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(
        nullptr, {.candidate_limit = 4, .learning_enabled = true, .previous_word = {}});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_utf8("kt"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");
    ASSERT_FALSE(session->snapshot().candidates.empty());
    EXPECT_EQ(session->commit().text_utf8, "ক্ত");
}

// ---------------------------------------------------------------------------

// Explicit trailing virama (mission: KHIPRO TRAILING VIRAMA FIX).

// Token "qq" follows the real Khipro Classic convention (explicit hasanta);

// the committed output is exactly U+0995 U+09CD with no ZWNJ.

// ---------------------------------------------------------------------------

namespace {
constexpr const char* virama_cp = "\u09CD";  // ্

constexpr const char* ka_virama = "\u0995\u09CD";  // ক্

}  // namespace

TEST(KhiproLayoutTest, KhiproTrailingViramaCanBeTyped) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_utf8("kqq"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, ka_virama);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, ka_virama);
    // Screen appearance alone is not enough: the committed text must end in

    // the real virama code point, not in some look-alike or stripped form.

    ASSERT_EQ(committed.size(), 6U);  // two 3-byte code points

    EXPECT_EQ(committed.substr(3), virama_cp);
}

TEST(KhiproLayoutTest, KhiproTrailingViramaProducesCorrectUnicode) {
    const suborno::layouts::KhiproLayout layout;
    struct Case {
        const char* keys;
        const char* consonant;  // 3-byte UTF-8 Bengali consonant
    };
    const Case cases[] = {
        {"kqq", "\xE0\xA6\x95"},  // ক

        {"gqq", "\xE0\xA6\x97"},  // গ

        {"tqq", "\xE0\xA6\xA4"},  // ত

        {"mqq", "\xE0\xA6\xAE"},  // ম

        {"nqq", "\xE0\xA6\xA8"},  // ন

        {"rqq", "\xE0\xA6\xB0"},  // র

    };
    for (const Case& item : cases) {
        const std::string expected = std::string(item.consonant) + "\xE0\xA7\x8D";  // + ্ (U+09CD)

        EXPECT_EQ(layout.transliterate(item.keys), expected) << item.keys;
    }
}

TEST(KhiproLayoutTest, KhiproTrailingViramaCommitsCorrectly) {
    const suborno::layouts::KhiproLayout layout;
    // Word-final, punctuation-adjacent and paragraph-boundary commits all

    // carry the virama through NFC unchanged (ক্ is NFC-stable).

    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("ami kqq"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->commit().text_utf8, std::string("আমি ") + ka_virama);

    session->reset();
    EXPECT_EQ(session->append_utf8("kqq."), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->commit().text_utf8, std::string(ka_virama) + "।");

    session->reset();
    EXPECT_EQ(session->append_utf8("kqq"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->commit().text_utf8, ka_virama);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

TEST(KhiproLayoutTest, KhiproTrailingViramaBackspaceWorks) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    // Backspace edits the raw Latin buffer one keystroke at a time, like

    // every other Khipro editing operation (ksf -> ks -> ...).

    EXPECT_EQ(session->append_utf8("kqq"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, ka_virama);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কৃ");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);

    // Commit first, then backspace on the empty buffer: no crash, no text.

    EXPECT_EQ(session->append_utf8("kqq"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->commit().text_utf8, ka_virama);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);
}

TEST(KhiproLayoutTest, KhiproTrailingViramaDoesNotBreakAutoJoin) {
    const suborno::layouts::KhiproLayout layout;
    // Explicit hasanta followed by a consonant = exactly one join, same

    // result as the automatic path.

    EXPECT_EQ(layout.transliterate("kqqt"), "ক্ত");
    EXPECT_EQ(layout.transliterate("kt"), "ক্ত");
    EXPECT_EQ(layout.transliterate("kqqsf"), "ক্ষ");
    EXPECT_EQ(layout.transliterate("ksf"), "ক্ষ");
    EXPECT_EQ(layout.transliterate("nqqd"), "ন্দ");
    EXPECT_EQ(layout.transliterate("pqqra"), "প্রা");
    // The automatic virama chain stays untouched after an explicit join

    // (bn-khipro.mim spec: ক্ত then a plain দ).

    EXPECT_EQ(layout.transliterate("kqqtd"), "ক্তদ");
}

TEST(KhiproLayoutTest, KhiproExplicitJoinRemainsUnchanged) {
    const suborno::layouts::KhiproLayout layout;
    // The ';' partition key, '/' slicer, chandrabindu and khanda-ta tokens

    // keep their documented behavior alongside the new "qq" token.

    EXPECT_EQ(layout.transliterate("k;t"), "কত");
    EXPECT_EQ(layout.transliterate("t;i"), "তই");
    EXPECT_EQ(layout.transliterate(";;"), ";;");
    EXPECT_EQ(layout.transliterate("ks/"), "কস");
    EXPECT_EQ(layout.transliterate("bakr/uddh"), "বাকরুদ্ধ");
    EXPECT_EQ(layout.transliterate("t/"), "ৎ");
    EXPECT_EQ(layout.transliterate("ka/"), "কাঁ");
    // A slash with nothing before it stays literal in the spec (the

    // chandrabindu reading only fires after a vowel).

    EXPECT_EQ(layout.transliterate("/"), "/");
    // The slicer after an explicit hasanta: the spec leaves ক্ + ঁ.

    EXPECT_EQ(layout.transliterate("kqq/"), "ক্ঁ");
    EXPECT_EQ(layout.transliterate("kqq/t"), "ক্ঁত");
}

TEST(KhiproLayoutTest, KhiproExistingCorpusRemainsUnchanged) {
    const suborno::layouts::KhiproLayout layout;
    // Corpus headlines + inventory rows under the bn-khipro.mim spec.

    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("kaka"), "কাকা");
    // Spec: uppercase letters pass through literally (uppercaseIdentity map)

    // and 'n' before a consonant reads as ঙ.

    EXPECT_EQ(layout.transliterate("bangla"), "বাঙলা");
    EXPECT_EQ(layout.transliterate("baNgali"), "বাNগালি");
    EXPECT_EQ(layout.transliterate("Ngk"), "Nগক");
    // Spec: ';' outside a join context emits U+204C.

    EXPECT_EQ(layout.transliterate("s;f;tf;w;yzar"),
              "\u09B8\u204C\u099F\u0993\u09DF\u09CD\u09AF\u09BE\u09B0");
    EXPECT_EQ(layout.transliterate("sbadhiin;ta"), "স্বাধীনতা");
    EXPECT_EQ(layout.transliterate("k;mpiutfar"), "কম্পিউটার");
    EXPECT_EQ(layout.transliterate("1234"), "১২৩৪");
    EXPECT_EQ(layout.transliterate("y,,z"), "\u09DF\u09BC\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("kq"), "কৃ");
    EXPECT_EQ(layout.transliterate("q"), "ঋ");
    // Nukta, digits, khanda-ta and visarga inventory rows stay put.

    EXPECT_EQ(layout.transliterate("k,,"), "ক়");
    EXPECT_EQ(layout.transliterate("koo"), "কঃ");
    // Spec: n before g reads as ঙ.

    EXPECT_EQ(layout.transliterate("kng"), "কঙ");
}

TEST(KhiproLayoutTest, KhiproTrailingViramaEdgeCases) {
    const suborno::layouts::KhiproLayout layout;
    // Spec (blind-state): the bare token keeps a lone virama alive.

    EXPECT_EQ(layout.transliterate("qq"), "্");
    EXPECT_EQ(layout.transliterate("aqq"), "আ্");
    EXPECT_EQ(layout.transliterate(";qq"), ";্");
    // Repeated token: the spec emits two viramas in a row.

    EXPECT_EQ(layout.transliterate("kqqqq"), "ক্্");
    // Explicit hasanta after a realised pair: the pair keeps its join and the

    // second consonant ends up dead (ক্স্ = ক স ্ + ্).

    EXPECT_EQ(layout.transliterate("ksqq"), std::string("ক") + virama_cp + "স" + virama_cp);
    // Uppercase stays literal passthrough (case-sensitive layout).

    EXPECT_EQ(layout.transliterate("kQQ"), std::string("ক") + "QQ");
    // Unmapped punctuation passthrough is untouched by the token.

    EXPECT_EQ(layout.transliterate("qq(Q)!"), "্(Q)!");
}
