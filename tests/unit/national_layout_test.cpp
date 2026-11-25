#include "suborno/layouts/national/national_layout.hpp"

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

suborno::layout::KeyEvent altgr_event(std::uint16_t scan_code, bool shift = false) {
    suborno::layout::KeyEvent value{};
    value.modifiers.control = true;
    value.modifiers.alt = true;
    value.modifiers.shift = shift;
    value.scan_code = scan_code;
    value.key_down = true;
    return value;
}

// Types a raw key sequence through a real National session and commits.

std::string type_keys(const suborno::layouts::NationalLayout& layout,
                      const std::vector<suborno::layout::KeyEvent>& keys) {
    auto session = layout.create_session(nullptr, {});
    for (const auto& key : keys) {
        EXPECT_EQ(session->append_key(key), suborno::layout::InputStatus::updated);
    }
    return session->commit().text_utf8;
}

// NFC keeps ড়/ঢ়/য় decomposed, so expectations use explicit code points.

constexpr auto rra = "\u09A1\u09BC";  // ড়

constexpr auto rha = "\u09A2\u09BC";  // ঢ়

constexpr auto yya = "\u09AF\u09BC";  // য়

constexpr auto virama = "\u09CD";
constexpr auto reph = "\u09B0\u09CD";  // র্

constexpr auto zwnj = "\u200C";
constexpr auto zwj = "\u200D";

}  // namespace

TEST(NationalLayoutTest, IsARealFixedPhysicalKeyboardLayout) {
    const suborno::layouts::NationalLayout layout;
    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::fixed);
}

// The four layers follow the National Keyboard of Bangladesh diagram (Figure

// 4 of Unicode document L2/19-079): base = bottom-right, Shift = top-right,

// Link/AltGr = bottom-left, Link+Shift = top-left.

TEST(NationalLayoutTest, BaseLayerConsonantRows) {
    const suborno::layouts::NationalLayout layout;

    // Q-P row: ঙ য ড প ট চ জ হ গ ড়

    EXPECT_EQ(layout.transliterate("q"), "ঙ");
    EXPECT_EQ(layout.transliterate("w"), "য");
    EXPECT_EQ(layout.transliterate("e"), "ড");
    EXPECT_EQ(layout.transliterate("r"), "প");
    EXPECT_EQ(layout.transliterate("t"), "ট");
    EXPECT_EQ(layout.transliterate("y"), "চ");
    EXPECT_EQ(layout.transliterate("u"), "জ");
    EXPECT_EQ(layout.transliterate("i"), "হ");
    EXPECT_EQ(layout.transliterate("o"), "গ");
    EXPECT_EQ(layout.transliterate("p"), rra);

    // A-L row letters: ব ক ত দ (vowel-sign keys are covered separately)

    EXPECT_EQ(layout.transliterate("h"), "ব");
    EXPECT_EQ(layout.transliterate("j"), "ক");
    EXPECT_EQ(layout.transliterate("k"), "ত");
    EXPECT_EQ(layout.transliterate("l"), "দ");

    // Z-M row: ্র ও ে র ন স ম

    EXPECT_EQ(layout.transliterate("z"), std::string(virama) + "র");
    EXPECT_EQ(layout.transliterate("x"), "ও");
    EXPECT_EQ(layout.transliterate("c"), "ে");
    EXPECT_EQ(layout.transliterate("v"), "র");
    EXPECT_EQ(layout.transliterate("b"), "ন");
    EXPECT_EQ(layout.transliterate("n"), "স");
    EXPECT_EQ(layout.transliterate("m"), "ম");

    // Hasanta key

    EXPECT_EQ(layout.transliterate("g"), virama);
}

TEST(NationalLayoutTest, ShiftLayerConsonantRows) {
    const suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("Q"), "ং");
    EXPECT_EQ(layout.transliterate("W"), yya);
    EXPECT_EQ(layout.transliterate("E"), "ঢ");
    EXPECT_EQ(layout.transliterate("R"), "ফ");
    EXPECT_EQ(layout.transliterate("T"), "ঠ");
    EXPECT_EQ(layout.transliterate("Y"), "ছ");
    EXPECT_EQ(layout.transliterate("U"), "ঝ");
    EXPECT_EQ(layout.transliterate("I"), "ঞ");
    EXPECT_EQ(layout.transliterate("O"), "ঘ");
    EXPECT_EQ(layout.transliterate("P"), rha);

    EXPECT_EQ(layout.transliterate("H"), "ভ");
    EXPECT_EQ(layout.transliterate("J"), "খ");
    EXPECT_EQ(layout.transliterate("K"), "থ");
    EXPECT_EQ(layout.transliterate("L"), "ধ");

    EXPECT_EQ(layout.transliterate("Z"), std::string(virama) + "য");
    EXPECT_EQ(layout.transliterate("X"), "ৗ");
    EXPECT_EQ(layout.transliterate("C"), "ৈ");
    EXPECT_EQ(layout.transliterate("V"), "ল");
    EXPECT_EQ(layout.transliterate("B"), "ণ");
    EXPECT_EQ(layout.transliterate("N"), "ষ");
    EXPECT_EQ(layout.transliterate("M"), "শ");

    EXPECT_EQ(layout.transliterate("G"), "।");
}

TEST(NationalLayoutTest, VowelSignKeys) {
    const suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "ৃ");
    EXPECT_EQ(layout.transliterate("s"), "ু");
    EXPECT_EQ(layout.transliterate("S"), "ূ");
    EXPECT_EQ(layout.transliterate("d"), "ি");
    EXPECT_EQ(layout.transliterate("D"), "ী");
    EXPECT_EQ(layout.transliterate("f"), "া");
    // Shift+F gives the letter অ (the diagram's Shift position of the া key).

    EXPECT_EQ(layout.transliterate("F"), "অ");
}

TEST(NationalLayoutTest, NationalHallmarkShiftPositions) {
    const suborno::layouts::NationalLayout layout;

    // Shift+4 = ৳ (U+09F3) and Shift+7 = ঁ (U+0981): the signature National

    // positions, verified against the official diagram and the reference.

    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate("&"), "ঁ");
    // Shift+backslash = ঃ; base backslash = ৎ.

    EXPECT_EQ(layout.transliterate("|"), "ঃ");
    EXPECT_EQ(layout.transliterate("\\"), "ৎ");
    // Shift+grave emits the left double quotation mark; Shift+apostrophe

    // emits the grave accent (reference mapping facts).

    EXPECT_EQ(layout.transliterate("~"), "“");
    EXPECT_EQ(layout.transliterate("\""), "`");
}

TEST(NationalLayoutTest, BengaliDigitRow) {
    const suborno::layouts::NationalLayout layout;
    const std::string out = layout.transliterate("0123456789");
    EXPECT_EQ(out, "০১২৩৪৫৬৭৮৯");
    ASSERT_EQ(out.size(), 30U);  // 10 x 3-byte code points

    for (unsigned int i = 0; i < 10; ++i) {
        EXPECT_TRUE(suborno::unicode::is_valid_utf8(out.substr(i * 3, 3))) << i;
    }
}

TEST(NationalLayoutTest, ShiftDigitsKeepEnglishPunctuation) {
    const suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("!"), "!");
    EXPECT_EQ(layout.transliterate("@"), "@");
    EXPECT_EQ(layout.transliterate("#"), "#");
    EXPECT_EQ(layout.transliterate("%"), "%");
    EXPECT_EQ(layout.transliterate("^"), "^");
    EXPECT_EQ(layout.transliterate("*"), "*");
    EXPECT_EQ(layout.transliterate("("), "(");
    EXPECT_EQ(layout.transliterate(")"), ")");
    EXPECT_EQ(layout.transliterate("-"), "-");
    EXPECT_EQ(layout.transliterate("_"), "_");
    EXPECT_EQ(layout.transliterate("="), "=");
    EXPECT_EQ(layout.transliterate("+"), "+");
}

TEST(NationalLayoutTest, EnglishOnlyKeysPassThrough) {
    const suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("[]"), "[]");
    EXPECT_EQ(layout.transliterate("{}"), "{}");
    EXPECT_EQ(layout.transliterate(";:"), ";:");
    EXPECT_EQ(layout.transliterate(",<"), ",<");
    EXPECT_EQ(layout.transliterate(".>"), ".>");
    EXPECT_EQ(layout.transliterate("/?"), "/?");
    EXPECT_EQ(layout.transliterate("`"), "`");
}

TEST(NationalLayoutTest, AltGrLinkLayerDigitsAndSigns) {
    const suborno::layouts::NationalLayout layout;

    // Link layer digit row: fraction/currency numerators per the diagram.

    EXPECT_EQ(type_keys(layout, {altgr_event(0x02)}), "৴");  // U+09F4

    EXPECT_EQ(type_keys(layout, {altgr_event(0x03)}), "৵");  // U+09F5

    EXPECT_EQ(type_keys(layout, {altgr_event(0x04)}), "৶");  // U+09F6

    EXPECT_EQ(type_keys(layout, {altgr_event(0x05)}), "৷");  // U+09F7

    EXPECT_EQ(type_keys(layout, {altgr_event(0x06)}), "৸");  // U+09F8

    EXPECT_EQ(type_keys(layout, {altgr_event(0x07)}), "ৠ");  // U+09E0

    EXPECT_EQ(type_keys(layout, {altgr_event(0x08)}), "৺");  // U+09FA

    EXPECT_EQ(type_keys(layout, {altgr_event(0x09)}), "ৡ");  // U+09E1

    EXPECT_EQ(type_keys(layout, {altgr_event(0x0A)}), "ঌ");  // U+098C

    EXPECT_EQ(type_keys(layout, {altgr_event(0x0B)}), "৹");  // U+09F9

    // Link+Shift digit row (top-left diagram positions).

    EXPECT_EQ(type_keys(layout, {altgr_event(0x02, true)}), "\u0980");  // ঀ

    EXPECT_EQ(type_keys(layout, {altgr_event(0x04, true)}), "ঽ");  // U+09BD

    EXPECT_EQ(type_keys(layout, {altgr_event(0x05, true)}), "৲");  // U+09F2

    EXPECT_EQ(type_keys(layout, {altgr_event(0x06, true)}), "ৄ");  // U+09C4

    EXPECT_EQ(type_keys(layout, {altgr_event(0x07, true)}), "ৠ");  // U+09E0

    EXPECT_EQ(type_keys(layout, {altgr_event(0x08, true)}), "\u09FC");  // ৾ U+09FC

    EXPECT_EQ(type_keys(layout, {altgr_event(0x09, true)}), "ৣ");  // U+09E3

    EXPECT_EQ(type_keys(layout, {altgr_event(0x0A, true)}), "ৢ");  // U+09E2

    EXPECT_EQ(type_keys(layout, {altgr_event(0x0B, true)}), "৽");  // U+09FD

    // Link+2 has no reference mapping (empty) and must not emit anything.

    auto session = layout.create_session(nullptr, {});
    EXPECT_EQ(session->append_key(altgr_event(0x03, true)), suborno::layout::InputStatus::empty);
    EXPECT_EQ(session->commit().text_utf8, "");
}

TEST(NationalLayoutTest, AltGrLinkLayerIndependentVowels) {
    const suborno::layouts::NationalLayout layout;

    // Link + vowel-sign keys emit the independent vowels (g-prefix facts in

    // the reference become real AltGr layer entries here).

    EXPECT_EQ(type_keys(layout, {altgr_event(0x21)}), "আ");  // Link+f

    EXPECT_EQ(type_keys(layout, {altgr_event(0x20)}), "ই");  // Link+d

    EXPECT_EQ(type_keys(layout, {altgr_event(0x20, true)}), "ঈ");
    EXPECT_EQ(type_keys(layout, {altgr_event(0x1F)}), "উ");  // Link+s

    EXPECT_EQ(type_keys(layout, {altgr_event(0x1F, true)}), "ঊ");
    EXPECT_EQ(type_keys(layout, {altgr_event(0x1E)}), "ঋ");  // Link+a

    EXPECT_EQ(type_keys(layout, {altgr_event(0x2E)}), "এ");  // Link+c

    EXPECT_EQ(type_keys(layout, {altgr_event(0x2E, true)}), "ঐ");
    EXPECT_EQ(type_keys(layout, {altgr_event(0x2D)}), "ো");  // Link+x

    EXPECT_EQ(type_keys(layout, {altgr_event(0x2D, true)}), "ঔ");

    // Link+Shift+G = ॥; Link+- = ×; Link+Shift+- = ÷.

    EXPECT_EQ(type_keys(layout, {altgr_event(0x22, true)}), "॥");
    EXPECT_EQ(type_keys(layout, {altgr_event(0x0C)}), "×");
    EXPECT_EQ(type_keys(layout, {altgr_event(0x0C, true)}), "÷");
}

TEST(NationalLayoutTest, AssameseLinkCharactersAreExcluded) {
    const suborno::layouts::NationalLayout layout;

    // The reference maps Link+comma to ৰ (U+09F0) and Link+period to ৱ

    // (U+09F1). SubornoIME deliberately excludes the Assamese letters from

    // its Bengali inventory, so those Link positions stay empty.

    auto session = layout.create_session(nullptr, {});
    EXPECT_EQ(session->append_key(altgr_event(0x33)), suborno::layout::InputStatus::empty);
    session->reset();
    EXPECT_EQ(session->append_key(altgr_event(0x34)), suborno::layout::InputStatus::empty);
    EXPECT_EQ(session->commit().text_utf8, "");

    // The exotic Link+Shift+7 Vedic anusvara (U+09FC) is part of the

    // reference mapping and stays available.

    EXPECT_EQ(type_keys(layout, {altgr_event(0x08, true)}), "\u09FC");
}

TEST(NationalLayoutTest, RephShiftAPrecedesTrailingConsonant) {
    const suborno::layouts::NationalLayout layout;

    // Shift+A after a consonant moves র্ before it (reference fact: type the

    // consonant, then Shift+A).

    EXPECT_EQ(layout.transliterate("jA"), std::string(reph) + "ক");
    // Reph applies to the trailing cluster (ক্ক).

    EXPECT_EQ(layout.transliterate("jgjA"), std::string(reph) + "ক" + virama + "ক");
    // Standalone Shift+A emits র্ raw.

    EXPECT_EQ(layout.transliterate("A"), reph);
    // Reph after the decomposed ড় unit (consonant + nukta).

    EXPECT_EQ(layout.transliterate("pA"), std::string(reph) + rra);
    // After a vowel sign the reph key cannot attach: র্ lands raw.

    EXPECT_EQ(layout.transliterate("jfA"), std::string("ক") + "া" + reph);
}

TEST(NationalLayoutTest, BrokenConjunctViaDoubleHasanta) {
    const suborno::layouts::NationalLayout layout;

    // Second hasanta press emits virama + ZWNJ: ক্‌ক.

    EXPECT_EQ(layout.transliterate("jggj"), std::string("ক") + virama + zwnj + "ক");
}

TEST(NationalLayoutTest, CandrabinduAaKarCanonicalOrder) {
    const suborno::layouts::NationalLayout layout;

    // Shift+7 then f: ঁ + া reorders to the canonical াঁ.

    EXPECT_EQ(layout.transliterate("&f"), "াঁ");
    EXPECT_EQ(layout.transliterate("h&f"), std::string("ব") + "াঁ");
}

TEST(NationalLayoutTest, RaKarAndYaPhalaReorder) {
    const suborno::layouts::NationalLayout layout;

    // Vowel sign typed before ্র reorders: ক + ু + ্র → ক্রু.

    EXPECT_EQ(layout.transliterate("jsz"), std::string("ক") + virama + "র" + "ু");
    // ...and before ্য: ক + ু + ্য → ক্যু.

    EXPECT_EQ(layout.transliterate("jsZ"), std::string("ক") + virama + "য" + "ু");
    // Modern order needs no reorder: প + ্র + ু stays প্রু (প is the r key).

    EXPECT_EQ(layout.transliterate("rzs"), std::string("প") + virama + "র" + "ু");
    // Shift+Z after র yields the র‍্য ya-phala sequence (ZWJ keeps it

    // explicit).

    EXPECT_EQ(layout.transliterate("vZ"), std::string("র") + zwj + virama + "য");
}

TEST(NationalLayoutTest, EkarAaKarComposeThroughNfc) {
    const suborno::layouts::NationalLayout layout;

    // ক + ে + া normalizes to কো; ক + ে + ৗ normalizes to কৌ.

    EXPECT_EQ(layout.transliterate("jcf"), "কো");
    EXPECT_EQ(layout.transliterate("jcX"), "কৌ");
    // The reorder swaps only the sign directly before ্র, matching the

    // reference's single-sign rule: ক + ে + া + ্র → ক ে ্র া.

    EXPECT_EQ(layout.transliterate("jcfz"), std::string("ক") + "ে" + virama + "র" + "া");
}

TEST(NationalLayoutTest, FfSequenceGivesAa) {
    const suborno::layouts::NationalLayout layout;

    // Shift+F (অ) then f (া-kar) is invalid as অ+া; the layout produces আ.

    EXPECT_EQ(layout.transliterate("Ff"), "আ");
}

TEST(NationalLayoutTest, Words) {
    const suborno::layouts::NationalLayout layout;

    // বাংলা = h f Q V f

    EXPECT_EQ(layout.transliterate("hfQVf"), std::string("ব") + "াং" + "ল" + "া");
    // নমস্কার = b m n g j f v (ন ম স ্ ক া র)

    EXPECT_EQ(layout.transliterate("bmngjfv"), "নমস্কার");
    // আমি needs Link+f for আ; ম is scan 0x32; ি is scan 0x20.

    EXPECT_EQ(type_keys(layout, {altgr_event(0x21), event(0x32), event(0x20)}), "আমি");
}

TEST(NationalLayoutTest, CapsLockDoesNotModifyBanglaLayers) {
    const suborno::layouts::NationalLayout layout;

    EXPECT_EQ(type_keys(layout, {event(0x10, false, true)}), "ঙ");
    EXPECT_EQ(type_keys(layout, {event(0x10, true, true)}), "ং");
    EXPECT_EQ(type_keys(layout, {event(0x1E, false, true)}), "ৃ");
}

TEST(NationalLayoutTest, SessionBackspaceRemovesGraphemes) {
    const suborno::layouts::NationalLayout layout;
    auto session = layout.create_session(nullptr, {});

    // কা is a single extended grapheme cluster (ক + া-kar); each backspace

    // removes exactly one cluster. Removing the final cluster reports empty

    // (same convention as the Probhat session).

    ASSERT_EQ(session->append_utf8("jfjf"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কাকা");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কা");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);
    EXPECT_EQ(session->snapshot().preedit_utf8, "");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::empty);
}

TEST(NationalLayoutTest, SessionCommitNormalizesNfc) {
    const suborno::layouts::NationalLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_EQ(session->append_utf8("p"), suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, rra);
    EXPECT_TRUE(suborno::unicode::is_valid_utf8(committed));
    EXPECT_EQ(session->commit().status, suborno::layout::InputStatus::empty);
}

TEST(NationalLayoutTest, CtrlOrAltAloneNeverInjectsText) {
    const suborno::layouts::NationalLayout layout;
    auto session = layout.create_session(nullptr, {});

    suborno::layout::KeyEvent ctrl = event(0x10);
    ctrl.modifiers.control = true;
    EXPECT_EQ(session->append_key(ctrl), suborno::layout::InputStatus::empty);

    suborno::layout::KeyEvent alt = event(0x10);
    alt.modifiers.alt = true;
    EXPECT_EQ(session->append_key(alt), suborno::layout::InputStatus::empty);

    suborno::layout::KeyEvent win = event(0x10);
    win.modifiers.windows = true;
    EXPECT_EQ(session->append_key(win), suborno::layout::InputStatus::empty);
    EXPECT_EQ(session->commit().text_utf8, "");
}

TEST(NationalLayoutTest, Candidates) {
    const suborno::layouts::NationalLayout layout;

    const auto candidates = layout.generate_candidates("hfQVf");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "বাংলা");
    EXPECT_EQ(candidates.front().source, suborno::layout::CandidateSource::layout);
}

TEST(NationalLayoutTest, EmptyInputYieldsNoCandidates) {
    const suborno::layouts::NationalLayout layout;

    EXPECT_TRUE(layout.transliterate("").empty());
    EXPECT_TRUE(layout.generate_candidates("").empty());
}

TEST(NationalLayoutTest, InvalidUtf8ReturnsEmpty) {
    const suborno::layouts::NationalLayout layout;

    const std::string malformed = "\xFF\xFE";
    EXPECT_TRUE(layout.transliterate(malformed).empty());
    auto session = layout.create_session(nullptr, {});
    EXPECT_EQ(session->append_utf8(malformed), suborno::layout::InputStatus::invalid_utf8);
}
