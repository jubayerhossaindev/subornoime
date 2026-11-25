#include "suborno/layouts/probhat/probhat_layout.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

#include "suborno/unicode/bengali.hpp"

namespace {

suborno::layout::KeyEvent event(std::uint16_t scan_code,
                                bool shift = false,
                                bool caps_lock = false) {
    return {
        .modifiers = {.shift = shift, .caps_lock = caps_lock},
        .scan_code = scan_code,
        .key_down = true,
    };
}

}  // namespace

TEST(ProbhatLayoutTest, IsARealFixedPhysicalKeyboardLayout) {
    const suborno::layouts::ProbhatLayout layout;
    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::fixed);
}

TEST(ProbhatLayoutTest, AltGrLayerFollowsTheUpstreamKlcColumns) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    // AltGr (Ctrl+Alt) layers come straight from the upstream KLC columns

    // 7-8; keys without an AltGr value there must stay empty.

    suborno::layout::KeyEvent altgr{};
    altgr.modifiers.control = true;
    altgr.modifiers.alt = true;
    altgr.key_down = true;

    altgr.modifiers.shift = false;
    altgr.scan_code = 0x02;
    session->append_key(altgr);
    EXPECT_EQ(session->snapshot().preedit_utf8, "¡");

    session->reset();
    altgr.modifiers.shift = true;
    altgr.scan_code = 0x12;
    session->append_key(altgr);
    EXPECT_EQ(session->snapshot().preedit_utf8, "É");

    session->reset();
    altgr.modifiers.shift = false;
    altgr.scan_code = 0x33;
    session->append_key(altgr);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ç");

    session->reset();
    altgr.scan_code = 0x25;
    session->append_key(altgr);
    EXPECT_EQ(session->snapshot().preedit_utf8, "");
}

TEST(ProbhatLayoutTest, CtrlAloneOrAltAloneInjectsNoPreedit) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    suborno::layout::KeyEvent ctrl{};
    ctrl.modifiers.control = true;
    ctrl.scan_code = 0x25;
    ctrl.key_down = true;
    session->append_key(ctrl);
    EXPECT_EQ(session->snapshot().preedit_utf8, "");

    suborno::layout::KeyEvent alt{};
    alt.modifiers.alt = true;
    alt.scan_code = 0x25;
    alt.key_down = true;
    session->append_key(alt);
    EXPECT_EQ(session->snapshot().preedit_utf8, "");
}

TEST(ProbhatLayoutTest, CapsLockAppliesToSgCapsKeysMarkedInTheKlc) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    // 0x10 is CAPLOK|SGCAPS upstream; 0x33 is SGCAPS-only; 0x25 is CAPLOK.

    session->append_key(event(0x10, false, true));
    EXPECT_EQ(session->snapshot().preedit_utf8, "ধ");

    session->reset();
    session->append_key(event(0x33, false, true));
    EXPECT_EQ(session->snapshot().preedit_utf8, "ৃ");

    session->reset();
    session->append_key(event(0x25, false, true));
    EXPECT_EQ(session->snapshot().preedit_utf8, "খ");
}

TEST(ProbhatLayoutTest, MapsPhysicalKeyPositionsAndShiftLayer) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("k"), "ক");
    EXPECT_EQ(layout.transliterate("K"), "খ");
    EXPECT_EQ(layout.transliterate("f"), "ত");
    EXPECT_EQ(layout.transliterate("F"), "থ");
    EXPECT_EQ(layout.transliterate("a"), "া");
    EXPECT_EQ(layout.transliterate("A"), "অ");
    EXPECT_EQ(layout.transliterate("q"), "দ");
    EXPECT_EQ(layout.transliterate("Q"), "ধ");
    EXPECT_EQ(layout.transliterate("z"), "\u09af\u09bc");
    EXPECT_EQ(layout.transliterate("Z"), "য");
    EXPECT_EQ(layout.transliterate("y"), "এ");
    EXPECT_EQ(layout.transliterate("Y"), "ঐ");
    EXPECT_EQ(layout.transliterate("c"), "চ");
    EXPECT_EQ(layout.transliterate("C"), "ছ");
    EXPECT_EQ(layout.transliterate("w"), "ূ");
    EXPECT_EQ(layout.transliterate("W"), "ঊ");
    EXPECT_EQ(layout.transliterate("u"), "ু");
    EXPECT_EQ(layout.transliterate("U"), "উ");
    EXPECT_EQ(layout.transliterate("e"), "ী");
    EXPECT_EQ(layout.transliterate("E"), "ঈ");
    EXPECT_EQ(layout.transliterate("["), "ে");
    EXPECT_EQ(layout.transliterate("{"), "ৈ");
    EXPECT_EQ(layout.transliterate("]"), "ো");
    EXPECT_EQ(layout.transliterate("}"), "ৌ");
}

TEST(ProbhatLayoutTest, ConcatenatesLiteralFixedKeyOutputsWithoutPhoneticRewriting) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("ka"), "কা");
    EXPECT_EQ(layout.transliterate("ki"), "কি");
    EXPECT_EQ(layout.transliterate("ke"), "কী");
    EXPECT_EQ(layout.transliterate("ko"), "কও");
    EXPECT_EQ(layout.transliterate("k["), "কে");
    EXPECT_EQ(layout.transliterate("k]"), "কো");
    EXPECT_EQ(layout.transliterate("wa"), "ূা");
}

TEST(ProbhatLayoutTest, AppliesCapsLockOnlyToKeysMarkedInTheKlc) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x25, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "খ");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x02, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "১");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x33, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "\u09c3");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x0C, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "-");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x33, true, false)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ৃ");
}

TEST(ProbhatLayoutTest, UsesPhysicalScanCodeInsteadOfTheActiveLogicalCharacter) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    const suborno::layout::KeyEvent physical_k{
        .key = suborno::layout::PhysicalKey::letter_a,
        .scan_code = 0x25,
        .logical_text_available = true,
        .logical_text_utf8 = "য",
    };
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(physical_k), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

TEST(ProbhatLayoutTest, AppliesShiftLayerFromPhysicalKeyEvents) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(event(0x25, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "খ");
}

TEST(ProbhatLayoutTest, PassesThroughUnmappedNumpadDigits) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x50)), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

TEST(ProbhatLayoutTest, MapsDigitsSignsAndPunctuation) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("1234567890"), "১২৩৪৫৬৭৮৯০");
    EXPECT_EQ(layout.transliterate("4"), "৪");
    EXPECT_EQ(layout.transliterate("!"), "!");
    EXPECT_EQ(layout.transliterate("@"), "@");
    EXPECT_EQ(layout.transliterate("#"), "#");
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate("%"), "%");
    EXPECT_EQ(layout.transliterate("^"), "^");
    EXPECT_EQ(layout.transliterate("&"), "ঞ");
    EXPECT_EQ(layout.transliterate("*"), "ৎ");
    EXPECT_EQ(layout.transliterate("("), "(");
    EXPECT_EQ(layout.transliterate(")"), ")");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate(">"), "ঁ");
    EXPECT_EQ(layout.transliterate("/"), "্");
    EXPECT_EQ(layout.transliterate(","), ",");
    EXPECT_EQ(layout.transliterate("<"), "ৃ");
    EXPECT_EQ(layout.transliterate("\\"), "\\");
}

TEST(ProbhatLayoutTest, BackspaceRemovesOneBengaliGrapheme) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(event(0x21)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কত");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
    EXPECT_EQ(session->commit().text_utf8, "ক");
}

// ---------------------------------------------------------------------------

// Standalone Bengali nukta (mission: PROBHAT STANDALONE NUKTA FIX).

// SubornoIME Probhat Compatibility Extension: AltGr (Ctrl+Alt) on scan 0x34

// emits U+09BC. The upstream MIT Probhat KLC has no U+09BC anywhere; the

// upstream column for that slot is -1, so no existing key is overwritten.

// ---------------------------------------------------------------------------

namespace {

suborno::layout::KeyEvent altgr_event(std::uint16_t scan_code, bool shift = false) {
    suborno::layout::KeyEvent value{};
    value.modifiers.control = true;
    value.modifiers.alt = true;
    value.modifiers.shift = shift;
    value.scan_code = scan_code;
    value.key_down = true;
    return value;
}

constexpr const char* nukta = "\u09BC";  // ়

}  // namespace

TEST(ProbhatLayoutTest, ProbhatStandaloneNuktaCanBeTyped) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, nukta);
}

TEST(ProbhatLayoutTest, ProbhatNuktaProducesU09BC) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    // Exactly one code point U+09BC (three UTF-8 bytes E0 A6 BC), NFC-stable.

    ASSERT_EQ(committed.size(), 3U);
    EXPECT_EQ(committed, nukta);
    EXPECT_EQ(committed, suborno::unicode::normalize_nfc(committed));
}

TEST(ProbhatLayoutTest, ProbhatNuktaAfterConsonantWorks) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    struct Case {
        std::uint16_t base_scan;
        bool shift;
        const char* base_letter;  // 3-byte UTF-8 Bengali consonant

        const char* expected_tail;  // the code points the cluster must end with
    };
    // ড়/ঢ়/য় are composition exclusions: ড+nukta stays decomposed

    // (U+09A1 U+09BC) and is NOT the precomposed look-alike string.

    const Case cases[] = {
        {0x25, false, "\xE0\xA6\x95", "\xE0\xA6\x95\xE0\xA6\xBC"},  // ক় = U+0995 U+09BC

        {0x20, false, "\xE0\xA6\xA1", "\xE0\xA6\xA1\xE0\xA6\xBC"},  // ড় = U+09A1 U+09BC

        {0x2D, false, "\xE0\xA6\xB6", "\xE0\xA6\xB6\xE0\xA6\xBC"},  // শ় = U+09B6 U+09BC

        {0x2C, true, "\xE0\xA6\xAF", "\xE0\xA6\xAF\xE0\xA6\xBC"},  // য় = U+09AF U+09BC

    };
    for (const Case& item : cases) {
        session->reset();
        EXPECT_EQ(session->append_key(event(item.base_scan, item.shift)),
                  suborno::layout::InputStatus::updated)
            << item.base_letter;
        EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
        EXPECT_EQ(session->snapshot().preedit_utf8, item.expected_tail);
        EXPECT_EQ(session->commit().text_utf8, item.expected_tail);
    }
}

TEST(ProbhatLayoutTest, ProbhatNuktaUsesExpectedScanCode) {
    const suborno::layouts::ProbhatLayout layout;
    // The extension lives on AltGr (Ctrl+Alt) of scan 0x34 — the OEM_PERIOD

    // key whose base layer is danda । and whose Shift layer is chandrabindu ঁ.

    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate(">"), "ঁ");

    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, nukta);
    // Shift+AltGr keeps the upstream empty (-1) column.

    session->reset();
    EXPECT_EQ(session->append_key(altgr_event(0x34, true)), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

TEST(ProbhatLayoutTest, ProbhatNuktaDoesNotBreakExistingMappings) {
    const suborno::layouts::ProbhatLayout layout;
    // Every layer of 0x34 except the new AltGr slot, plus the nukta-carrying

    // letters and the AltGr samples asserted before the extension.

    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate(">"), "ঁ");
    EXPECT_EQ(layout.transliterate("x"), "\xE0\xA6\xB6");  // শ (base 0x2D)

    EXPECT_EQ(layout.transliterate("X"), "\xE0\xA6\xA2\xE0\xA6\xBC");  // ঢ় (Shift 0x2D, decomposed)

    EXPECT_EQ(layout.transliterate("z"), "\xE0\xA6\xAF\xE0\xA6\xBC");  // য় (base 0x2C, decomposed)

    EXPECT_EQ(layout.transliterate("Z"), "\xE0\xA6\xAF");  // য (Shift 0x2C)

    EXPECT_EQ(layout.transliterate("R"), "\xE0\xA6\xA1\xE0\xA6\xBC");  // ড় (Shift 0x13, decomposed)

    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    // AltGr samples that existed before the extension still hold.

    EXPECT_EQ(session->append_key(altgr_event(0x02)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "\xC2\xA1");  // ¡

    session->reset();
    EXPECT_EQ(session->append_key(altgr_event(0x33)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "\xC3\xA7");  // ç

    session->reset();
    // Keys with an upstream-empty AltGr column stay empty.

    EXPECT_EQ(session->append_key(altgr_event(0x25)), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

TEST(ProbhatLayoutTest, ProbhatNuktaCommitMatchesEncoding) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    // ক + nukta: the session commit and the pure transliterate path must

    // agree byte for byte (NFC parity contract).

    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, layout.transliterate("k") + nukta);
    EXPECT_EQ(committed, suborno::unicode::normalize_nfc(committed));
    EXPECT_NE(committed, std::string("ক"));  // the nukta must survive commit
}

TEST(ProbhatLayoutTest, ProbhatNuktaBackspaceWorks) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    // ক + ় is ONE grapheme cluster (Lo + Mn, ccc=7): a single backspace

    // removes the whole cluster, so the composition becomes empty.

    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string("\xE0\xA6\x95") + nukta);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());

    // With a trailing consonant the cluster boundary shows: backspace eats

    // ত first, then the whole ক় cluster.

    session->reset();
    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(event(0x21)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string("\xE0\xA6\x95") + nukta);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);

    // A standalone nukta is likewise a single grapheme.

    session->reset();
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

TEST(ProbhatLayoutTest, ProbhatNuktaCompositionAndEditingEdges) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    // ক + virama + nukta: canonical reordering at commit. The raw preedit

    // keeps the typed order, while the committed NFC text sorts the nukta

    // (ccc=7) before the virama (ccc=9): ক ় ্ = U+0995 U+09BC U+09CD.

    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(event(0x35)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string("\xE0\xA6\x95") + "\u09CD" + nukta);
    EXPECT_EQ(session->commit().text_utf8, std::string("\xE0\xA6\x95") + nukta + "\u09CD");

    // ক + nukta + virama: nukta first, then the dead consonant.

    session->reset();
    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(event(0x35)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string("\xE0\xA6\x95") + nukta + "\u09CD");

    // Repeated presses concatenate literally — the fixed-layout contract.

    session->reset();
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string(nukta) + nukta);

    // repeat_count honours the key-repeat semantics.

    session->reset();
    suborno::layout::KeyEvent repeated = altgr_event(0x34);
    repeated.repeat_count = 2;
    EXPECT_EQ(session->append_key(repeated), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string(nukta) + nukta);

    // Key-up events and Ctrl/Alt-alone never inject text.

    session->reset();
    suborno::layout::KeyEvent key_up = altgr_event(0x34);
    key_up.key_down = false;
    EXPECT_EQ(session->append_key(key_up), suborno::layout::InputStatus::empty);
    suborno::layout::KeyEvent ctrl{};
    ctrl.modifiers.control = true;
    ctrl.scan_code = 0x34;
    ctrl.key_down = true;
    EXPECT_EQ(session->append_key(ctrl), suborno::layout::InputStatus::empty);

    // Nukta then space/enter-style commit boundaries.

    session->reset();
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(event(0x39)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, std::string(nukta) + " ");
    EXPECT_EQ(session->commit().text_utf8, std::string(nukta) + " ");
}
