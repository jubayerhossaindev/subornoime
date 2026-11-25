// BDS mission conformance tests (COMPLETE BDS COMPLIANCE mission, 2026-11-24).

//

// Independent expected-output corpus derived from the official Unicode

// Bengali chart (U+0980.pdf, current revision) and UAX #15 conformance data —

// NOT from SubornoIME's own implementation. These tests verify:

//   1. Bengali digit row (U+09E6..U+09EF) through the national layout

//      pipeline.

//   2. Preservation of the Khipro trailing-virama fix (kqq -> U+0995 U+09CD).

//   3. Preservation of the Probhat standalone-nukta fix (AltGr+0x34 ->

//      U+09BC).

//   4. Engine NFC behaviour against the official NormalizationTest.txt

//      expectations for the Bengali block (composition exclusions

//      U+09DC/U+09DD/U+09DF and virama/nukta sequence stability).

//   5. Full Bengali block UTF-8 round-trip through the engine's unicode

//      module.

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

#include "suborno/layouts/khipro/khipro_layout.hpp"
#include "suborno/layouts/national/national_layout.hpp"
#include "suborno/layouts/probhat/probhat_layout.hpp"
#include "suborno/unicode/bengali.hpp"

namespace {

constexpr const char* ka = "\xE0\xA6\x95";  // ক U+0995

constexpr const char* virama = "\xE0\xA7\x8D";  // ্ U+09CD

constexpr const char* nukta = "\xE0\xA6\xBC";  // ় U+09BC

constexpr const char* rra = "\xE0\xA7\x9C";  // ড় U+09DC

constexpr const char* rha = "\xE0\xA7\x9D";  // ঢ় U+09DD

constexpr const char* yya = "\xE0\xA7\x9F";  // য় U+09DF

constexpr const char* dda = "\xE0\xA6\xA1";  // ড U+09A1

constexpr const char* ddha = "\xE0\xA6\xA2";  // ঢ U+09A2

constexpr const char* ya = "\xE0\xA6\xAF";  // য U+09AF

suborno::layout::KeyEvent make_key(std::uint16_t scan_code, bool shift, bool ctrl, bool alt) {
    suborno::layout::KeyEvent key{};
    key.modifiers.control = ctrl;
    key.modifiers.alt = alt;
    key.modifiers.shift = shift;
    key.key_down = true;
    key.scan_code = scan_code;
    return key;
}

}  // namespace

// ---------------------------------------------------------------------------

// 1. Bengali digits (BDS 1520 repertoire relevance: Bangla digits are part of

//    the interchange repertoire; the engine must emit the exact code points).

// ---------------------------------------------------------------------------

TEST(BdsConformanceTest, NationalLayoutBengaliDigitRowProducesU09E6ToU09EF) {
    const suborno::layouts::NationalLayout layout;
    const std::string out = layout.transliterate("0123456789");
    // শূন্য ০..৯ = U+09E6..U+09EF, each a 3-byte UTF-8 sequence.

    EXPECT_EQ(out, "০১২৩৪৫৬৭৮৯");
    ASSERT_EQ(out.size(), 30U);  // 10 x 3 bytes

    for (unsigned int i = 0; i < 10; ++i) {
        const std::string digit = out.substr(i * 3, 3);
        EXPECT_TRUE(suborno::unicode::is_valid_utf8(digit)) << i;
        EXPECT_EQ(suborno::unicode::count_utf8_code_points(digit), 1U) << i;
    }
}

TEST(BdsConformanceTest, ProbhatDigitRowProducesU09E7ToU09EF) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    // Probhat number row 0x02..0x0C: ১..০ per the upstream KLC table.

    const std::uint16_t scans[] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
    const char* expected[] = {"১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯", "০"};
    for (unsigned int i = 0; i < 10; ++i) {
        session->reset();
        EXPECT_EQ(session->append_key(make_key(scans[i], false, false, false)),
                  suborno::layout::InputStatus::updated);
        EXPECT_EQ(session->snapshot().preedit_utf8, expected[i]) << "scan " << scans[i];
        const std::string committed = session->commit().text_utf8;
        EXPECT_EQ(committed, expected[i]) << "scan " << scans[i];
    }
}

// ---------------------------------------------------------------------------

// 2. Preservation of the Khipro trailing-virama fix (acceptance criterion:

//    "Khipro fix — preserved and verified").

// ---------------------------------------------------------------------------

TEST(BdsConformanceTest, KhiproTrailingViramaFixPreservedU0995U09CD) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("kqq"), suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, std::string(ka) + virama);
    ASSERT_EQ(committed.size(), 6U);
    EXPECT_EQ(committed.substr(0, 3), ka);
    EXPECT_EQ(committed.substr(3, 3), virama);
}

TEST(BdsConformanceTest, KhiproExplicitJoinAfterViramaStillComposes) {
    const suborno::layouts::KhiproLayout layout;
    // kqq then a second consonant must compose the conjunct without a second

    // virama (kk -> ক্ক, one virama total).

    const std::string out = layout.transliterate("kqqk");
    EXPECT_EQ(out, std::string(ka) + virama + ka);
}

// ---------------------------------------------------------------------------

// 3. Preservation of the Probhat standalone-nukta fix.

// ---------------------------------------------------------------------------

TEST(BdsConformanceTest, ProbhatStandaloneNuktaFixPreservedU09BC) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(make_key(0x34, false, true, true)),
              suborno::layout::InputStatus::updated);
    const std::string preedit = session->snapshot().preedit_utf8;
    EXPECT_EQ(preedit, nukta);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, nukta);
    ASSERT_EQ(committed.size(), 3U);  // U+09BC -> E0 A6 BC

    EXPECT_EQ(committed, std::string("\xE0\xA6\xBC"));
}

TEST(BdsConformanceTest, ProbhatNuktaAfterConsonantFormsU09A1U09BCSequence) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);
    // ড (scan 0x20 unshifted, per the upstream KLC column) then AltGr+0x34

    // nukta -> ড + U+09BC, the canonical decomposed form of ড় (U+09DC

    // composition exclusion).

    EXPECT_EQ(session->append_key(make_key(0x20, false, false, false)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(make_key(0x34, false, true, true)),
              suborno::layout::InputStatus::updated);
    const std::string committed = session->commit().text_utf8;
    EXPECT_EQ(committed, std::string(dda) + nukta);
    EXPECT_EQ(suborno::unicode::normalize_nfc(committed), committed);
}

// ---------------------------------------------------------------------------

// 4. Engine NFC behaviour vs the official NormalizationTest.txt Bengali

//    expectations (UAX15-C3 subset; the engine purports NFC only).

// ---------------------------------------------------------------------------

TEST(BdsConformanceTest, EngineNfcMatchesOfficialBengaliNormalizationData) {
    struct Row {
        std::string source;  // raw code points as UTF-8

        std::string nfc_expected;  // official NFC column
    };
    const Row rows[] = {
        {"ো", "ো"},  // U+09CB stays composed

        {"ো", "ো"},  // U+09C7 U+09BE composes to U+09CB

        {"ৌ", "ৌ"},  // U+09CC stays composed

        {"ৌ", "ৌ"},  // U+09C7 U+09D7 composes to U+09CC

        {rra, std::string(dda) + nukta},  // U+09DC -> 09A1 09BC

        {rha, std::string(ddha) + nukta},  // U+09DD -> 09A2 09BC

        {yya, std::string(ya) + nukta},  // U+09DF -> 09AF 09BC

        {std::string(dda) + nukta, std::string(dda) + nukta},  // stays decomposed

        {std::string(ddha) + nukta, std::string(ddha) + nukta},  // stays decomposed

        {std::string(ya) + nukta, std::string(ya) + nukta},  // stays decomposed

    };
    for (const Row& row : rows) {
        EXPECT_EQ(suborno::unicode::normalize_nfc(row.source), row.nfc_expected);
    }
}

TEST(BdsConformanceTest, ViramaAndNuktaSequencesAreNfcStable) {
    EXPECT_EQ(suborno::unicode::normalize_nfc(std::string(ka) + virama), std::string(ka) + virama);
    EXPECT_EQ(suborno::unicode::normalize_nfc(nukta), nukta);
    EXPECT_EQ(suborno::unicode::normalize_nfc(std::string(ka) + nukta), std::string(ka) + nukta);
    EXPECT_EQ(suborno::unicode::normalize_nfc(std::string(ka) + virama + ka),
              std::string(ka) + virama + ka);
}

// ---------------------------------------------------------------------------

// 5. Full Bengali block UTF-8 round-trip through the engine unicode module.

// ---------------------------------------------------------------------------

TEST(BdsConformanceTest, BengaliBlockUtf8RoundTripSweep) {
    auto encode = [](std::uint32_t cp) {
        std::string s;
        if (cp < 0x80) {
            s.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        return s;
    };
    for (std::uint32_t cp = 0x0980; cp <= 0x09FF; ++cp) {
        const std::string text = encode(cp);
        ASSERT_TRUE(suborno::unicode::is_valid_utf8(text)) << "U+" << std::hex << cp;
        ASSERT_EQ(suborno::unicode::count_utf8_code_points(text), 1U) << "U+" << std::hex << cp;
        // NFC must never crash, truncate, or inject private-use/surrogates.

        const std::string nfc = suborno::unicode::normalize_nfc(text);
        ASSERT_FALSE(nfc.empty()) << "U+" << std::hex << cp;
        ASSERT_TRUE(suborno::unicode::is_valid_utf8(nfc)) << "U+" << std::hex << cp;
    }
}

TEST(BdsConformanceTest, ReservedBengaliCodePointsDoNotCorruptEngine) {
    // U+0984, U+0991, U+0992, U+09A4..U+09A8 gap etc. are unassigned in the

    // Bengali block; the engine must still treat their UTF-8 bytes safely.

    const std::uint32_t reserved[] = {0x0984, 0x0991, 0x0992, 0x09C4, 0x09D0, 0x09E4, 0x09E5};
    for (std::uint32_t cp : reserved) {
        const std::string text = {static_cast<char>(0xE0 | (cp >> 12)),
                                  static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
                                  static_cast<char>(0x80 | (cp & 0x3F))};
        EXPECT_TRUE(suborno::unicode::is_valid_utf8(text)) << "U+" << std::hex << cp;
        EXPECT_EQ(suborno::unicode::count_utf8_code_points(text), 1U) << "U+" << std::hex << cp;
        EXPECT_EQ(suborno::unicode::normalize_nfc(text), text) << "U+" << std::hex << cp;
    }
}
