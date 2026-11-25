// Coverage tests for src/unicode/bengali.cpp and src/unicode/bijoy.cpp.
// Each test names the measured gap (uncovered line or branch) it closes.

#include <cstddef>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "suborno/unicode/bengali.hpp"
#include "suborno/unicode/bijoy.hpp"

namespace {

// supported_bengali_inventory() returns a fixed-order literal, so row indexes
// are part of the tested contract of bengali.cpp:94-159.
const std::vector<suborno::unicode::BengaliCharacterInfo>& inventory() {
    static const std::vector<suborno::unicode::BengaliCharacterInfo> rows =
        suborno::unicode::supported_bengali_inventory();
    return rows;
}

// Verifies one inventory row: character, Unicode name, general category,
// romanization and the self-normalized form.
void expect_row(std::size_t index,
                const std::string& character,
                const std::string& name,
                const std::string& category,
                const std::string& romanization) {
    const suborno::unicode::BengaliCharacterInfo& row = inventory().at(index);
    EXPECT_EQ(row.character, character);
    EXPECT_EQ(row.name, name);
    EXPECT_EQ(row.category, category);
    EXPECT_EQ(row.romanization, romanization);
    EXPECT_EQ(row.normalized_form, character);
}

}  // namespace

// Covers bengali.cpp:59-61 — empty input short-circuits to valid, and every
// well-formed 1/2/3/4-byte sequence shape is accepted.
TEST(UnicodeCoverage, IsValidUtf8AcceptsEmptyAndEveryWellFormedSequenceLength) {
    EXPECT_TRUE(suborno::unicode::is_valid_utf8(""));
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\x7F"));  // U+007F: 1-byte.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xC2\x80"));  // U+0080: 2-byte.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xDF\xBF"));  // U+07FF: 2-byte end.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xE0\xA0\x80"));  // U+0800: 3-byte start.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("ক"));  // U+0995: Bengali.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xED\x9F\xBF"));  // U+D7FF: last pre-surrogate.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xEE\x80\x80"));  // U+E000: private use.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xF0\x90\x80\x80"));  // U+10000: 4-byte start.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xF0\x9F\x98\x80"));  // U+1F600: emoji.
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("\xF4\x8F\xBF\xBD"));  // U+10FFFD: near max.
}

// Covers bengali.cpp:67 — every ICU rejection path (lone continuations,
// overlongs, surrogates, truncations, out-of-range and bad leads) is invalid.
TEST(UnicodeCoverage, IsValidUtf8RejectsOverlongSurrogateTruncatedAndBadLeadSequences) {
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\x80"));  // lone continuation.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xBF"));  // lone continuation.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xC0\x80"));  // overlong U+0000.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xC1\xBF"));  // overlong U+007F.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xC2"));  // truncated 2-byte.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xE0\x80\x80"));  // overlong 3-byte.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xE0\xA6"));  // truncated 3-byte.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xED\xA0\x80"));  // UTF-16 surrogate.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xF0\x80\x80\x80"));  // overlong 4-byte.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xF0\x8F\xBF\xBD"));  // overlong U+FFFF.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xF4\x90\x80\x80"));  // above U+10FFFF.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xF5\x80\x80\x80"));  // invalid F5 lead.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xF8\x80\x80\x80\x80"));  // invalid F8 lead.
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xFE"));
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xFF"));
}

// Covers bengali.cpp:71-72,78 — invalid input throws and UTF-8 trail bytes are
// never counted as code points.
TEST(UnicodeCoverage, CountsCodePointsNotBytesAndRejectsInvalidUtf8) {
    EXPECT_EQ(suborno::unicode::count_utf8_code_points(""), 0U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("ab"), 2U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("ক"), 1U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("কি"), 2U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("ক্ষ"), 3U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("\xF0\x9F\x98\x80"), 1U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("aকিx"), 4U);
    EXPECT_THROW(suborno::unicode::count_utf8_code_points("\x80"), std::invalid_argument);
    EXPECT_THROW(suborno::unicode::count_utf8_code_points("\xE0\xA6"), std::invalid_argument);
}

// Covers bengali.cpp:26,30,35-36,43-49 — the NFC path: typed two-part matras
// compose (ে+া→ো, ে+ৗ→ৌ), nuqta letters stay decomposed, joiners survive.
TEST(UnicodeCoverage, NfcComposesTwoPartMatrasAndKeepsNuqtaLettersDecomposed) {
    EXPECT_EQ(suborno::unicode::normalize_nfc("ক\u09C7\u09BE"), "কো");
    EXPECT_EQ(suborno::unicode::normalize_nfc("ক\u09C7\u09D7"), "কৌ");
    EXPECT_EQ(suborno::unicode::normalize_nfc("কো"), "কো");
    EXPECT_EQ(suborno::unicode::normalize_nfc("ক\u200Dষ"), "ক\u200Dষ");
    EXPECT_EQ(suborno::unicode::normalize_nfc("ক্\u200Cত"), "ক্\u200Cত");
    EXPECT_EQ(suborno::unicode::normalize_nfc("আমার"), "আমার");
    EXPECT_EQ(suborno::unicode::normalize_nfc(""), "");
}

// Covers bengali.cpp:36 — the NFKC path: compatibility forms fold while
// Bengali letters and digits (no compatibility decompositions) stay intact.
TEST(UnicodeCoverage, NfkcFoldsCompatibilityFormsAndKeepsBengaliUnchanged) {
    EXPECT_EQ(suborno::unicode::normalize_nfkc("\uFF21\uFF42\uFF11"), "Ab1");
    EXPECT_EQ(suborno::unicode::normalize_nfkc("\uFB01"), "fi");
    EXPECT_EQ(suborno::unicode::normalize_nfkc("ক"), "ক");
    EXPECT_EQ(suborno::unicode::normalize_nfkc("০১২"), "০১২");
    EXPECT_EQ(suborno::unicode::normalize_nfkc(""), "");
}

// Covers bengali.cpp:18-19 — the shared invalid-UTF-8 guard throws
// std::invalid_argument from every string-taking entry point.
TEST(UnicodeCoverage, NormalizationAndGraphemeSplittingRejectInvalidUtf8) {
    EXPECT_THROW(suborno::unicode::normalize_nfc("\xF8\x80\x80\x80\x80"), std::invalid_argument);
    EXPECT_THROW(suborno::unicode::normalize_nfkc("\x80"), std::invalid_argument);
    EXPECT_THROW(suborno::unicode::normalize_nfkc("\xED\xA0\x80"), std::invalid_argument);
    EXPECT_THROW(suborno::unicode::split_graphemes("\xC0\xAF"), std::invalid_argument);
    EXPECT_THROW(suborno::unicode::split_graphemes("\xE0\xA6"), std::invalid_argument);
}

// Covers bengali.cpp:94-129 — the inventory rows for independent vowels and
// consonants, including the duplicated এ row (romanizations "e" and "ei").
TEST(UnicodeCoverage, InventoryEnumeratesIndependentVowelsAndConsonants) {
    EXPECT_EQ(inventory().size(), 66U);
    expect_row(0U, "অ", "Bengali Letter A", "Lo", "a");
    expect_row(1U, "আ", "Bengali Letter AA", "Lo", "aa");
    expect_row(2U, "ই", "Bengali Letter I", "Lo", "i");
    expect_row(3U, "ঈ", "Bengali Letter II", "Lo", "ii");
    expect_row(4U, "উ", "Bengali Letter U", "Lo", "u");
    expect_row(5U, "ঊ", "Bengali Letter UU", "Lo", "uu");
    expect_row(6U, "ঋ", "Bengali Letter Vocalic R", "Lo", "ri");
    expect_row(7U, "এ", "Bengali Letter E", "Lo", "e");
    expect_row(8U, "এ", "Bengali Letter E", "Lo", "ei");
    expect_row(9U, "ও", "Bengali Letter O", "Lo", "o");
    expect_row(10U, "ক", "Bengali Letter Ka", "Lo", "k");
    expect_row(11U, "খ", "Bengali Letter Kha", "Lo", "kh");
    expect_row(12U, "গ", "Bengali Letter Ga", "Lo", "g");
    expect_row(13U, "ঘ", "Bengali Letter Gha", "Lo", "gh");
    expect_row(14U, "চ", "Bengali Letter Ca", "Lo", "c");
    expect_row(15U, "ছ", "Bengali Letter Cha", "Lo", "ch");
    expect_row(16U, "জ", "Bengali Letter Ja", "Lo", "j");
    expect_row(17U, "ঝ", "Bengali Letter Jha", "Lo", "jh");
    expect_row(18U, "ট", "Bengali Letter Tta", "Lo", "tt");
    expect_row(19U, "ঠ", "Bengali Letter Ttha", "Lo", "tth");
    expect_row(20U, "ড", "Bengali Letter Dda", "Lo", "dd");
    expect_row(21U, "ঢ", "Bengali Letter Ddha", "Lo", "ddh");
    expect_row(22U, "ণ", "Bengali Letter Nna", "Lo", "nn");
    expect_row(23U, "ত", "Bengali Letter Ta", "Lo", "t");
    expect_row(24U, "থ", "Bengali Letter Tha", "Lo", "th");
    expect_row(25U, "দ", "Bengali Letter Da", "Lo", "d");
    expect_row(26U, "ধ", "Bengali Letter Dha", "Lo", "dh");
    expect_row(27U, "ন", "Bengali Letter Na", "Lo", "n");
    expect_row(28U, "প", "Bengali Letter Pa", "Lo", "p");
    expect_row(29U, "ফ", "Bengali Letter Pha", "Lo", "ph");
    expect_row(30U, "ব", "Bengali Letter Ba", "Lo", "b");
    expect_row(31U, "ভ", "Bengali Letter Bha", "Lo", "bh");
    expect_row(32U, "ম", "Bengali Letter Ma", "Lo", "m");
    expect_row(33U, "য", "Bengali Letter Ya", "Lo", "y");
    expect_row(34U, "র", "Bengali Letter Ra", "Lo", "r");
    expect_row(35U, "ল", "Bengali Letter La", "Lo", "l");
    expect_row(36U, "শ", "Bengali Letter Sha", "Lo", "sh");
    expect_row(37U, "ষ", "Bengali Letter Ssa", "Lo", "shh");
    expect_row(38U, "স", "Bengali Letter Sa", "Lo", "s");
    expect_row(39U, "হ", "Bengali Letter Ha", "Lo", "h");
    expect_row(40U, "ঙ", "Bengali Letter Nga", "Lo", "ng");
    expect_row(41U, "ঞ", "Bengali Letter Nya", "Lo", "ny");
}

// Covers bengali.cpp:136-159 — the inventory rows for signs, matras, currency
// and the Bengali digits (category Nd, romanized to their ASCII values).
TEST(UnicodeCoverage, InventoryEnumeratesSignsMatrasCurrencyAndDigits) {
    expect_row(42U, "ং", "Bengali Sign Anusvara", "Mn", "ng");
    expect_row(43U, "ঃ", "Bengali Sign Visarga", "Mc", "h");
    expect_row(44U, "্", "Bengali Sign Virama", "Mn", "");
    expect_row(45U, "া", "Bengali Vowel Sign Aa", "Mn", "a");
    expect_row(46U, "ি", "Bengali Vowel Sign I", "Mn", "i");
    expect_row(47U, "ী", "Bengali Vowel Sign II", "Mn", "ii");
    expect_row(48U, "ু", "Bengali Vowel Sign U", "Mn", "u");
    expect_row(49U, "ূ", "Bengali Vowel Sign UU", "Mn", "uu");
    expect_row(50U, "ৃ", "Bengali Vowel Sign Vocalic R", "Mn", "ri");
    expect_row(51U, "ে", "Bengali Vowel Sign E", "Mn", "e");
    expect_row(52U, "ৈ", "Bengali Vowel Sign AI", "Mn", "oi");
    expect_row(53U, "ো", "Bengali Vowel Sign O", "Mn", "o");
    expect_row(54U, "ৌ", "Bengali Vowel Sign AU", "Mn", "ou");
    expect_row(55U, "\u09F3", "Bengali Rupee Sign", "Sc", "rs");
    expect_row(56U, "০", "Bengali Digit Zero", "Nd", "0");
    expect_row(57U, "১", "Bengali Digit One", "Nd", "1");
    expect_row(58U, "২", "Bengali Digit Two", "Nd", "2");
    expect_row(59U, "৩", "Bengali Digit Three", "Nd", "3");
    expect_row(60U, "৪", "Bengali Digit Four", "Nd", "4");
    expect_row(61U, "৫", "Bengali Digit Five", "Nd", "5");
    expect_row(62U, "৬", "Bengali Digit Six", "Nd", "6");
    expect_row(63U, "৭", "Bengali Digit Seven", "Nd", "7");
    expect_row(64U, "৮", "Bengali Digit Eight", "Nd", "8");
    expect_row(65U, "৯", "Bengali Digit Nine", "Nd", "9");
}

// Covers bengali.cpp:162-165 — the block range checks at both boundaries. The
// API tests range membership, not assignedness: unassigned code points inside
// 0980..09FF answer true, and the danda signs 0964/0965 (Bengali punctuation
// living in the Devanagari block) answer false.
TEST(UnicodeCoverage, IsBengaliCodepointSpansExactlyTheBengaliBlock) {
    // Below the block.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x0U));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint('a'));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x20U));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint('0'));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x7FU));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x3B1U));  // α Greek.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x963U));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x964U));  // । danda.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x965U));  // ॥ double danda.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x966U));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x97FU));  // one below the block.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x200CU));  // ZWNJ.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x200DU));  // ZWJ.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x3042U));  // あ CJK.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x1F600U));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x10FFFFU));
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0xFFFFFFFFU));
    // Inside the block, start to end.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x980U));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x981U));  // candrabindu.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x984U));  // unassigned, in range.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x985U));  // অ.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x995U));  // ক.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9BEU));  // া.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9BCU));  // nuqta.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9CDU));  // virama.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9CEU));  // khanda ta.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9DCU));  // ড়.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9DDU));  // ঢ়.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9DFU));  // য়.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9E6U));  // ০.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9EFU));  // ৯.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9F2U));  // taka sign.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9F4U));  // currency numerators.
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9F9U));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9FAU));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9FBU));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9FCU));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9FDU));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9FEU));
    EXPECT_TRUE(suborno::unicode::is_bengali_codepoint(0x9FFU));  // block end.
    // Above the block.
    EXPECT_FALSE(suborno::unicode::is_bengali_codepoint(0x0A00U));
}

// Covers bengali.cpp:183-188 — the grapheme loop: matras and joiners stay
// attached to their base cluster per UAX #29 (virama splits conjuncts).
TEST(UnicodeCoverage, SplitGraphemesGroupsMatrasAndJoinersWithTheirBase) {
    const auto aa = suborno::unicode::split_graphemes("কা");
    ASSERT_EQ(aa.size(), 1U);
    EXPECT_EQ(aa.front(), "কা");

    const auto short_i = suborno::unicode::split_graphemes("কি");
    ASSERT_EQ(short_i.size(), 1U);
    EXPECT_EQ(short_i.front(), "কি");

    const auto two_part = suborno::unicode::split_graphemes("কো");
    ASSERT_EQ(two_part.size(), 1U);
    EXPECT_EQ(two_part.front(), "কো");

    const auto vocalic = suborno::unicode::split_graphemes("কৃ");
    ASSERT_EQ(vocalic.size(), 1U);
    EXPECT_EQ(vocalic.front(), "কৃ");

    const auto word = suborno::unicode::split_graphemes("আমার");
    ASSERT_EQ(word.size(), 3U);
    EXPECT_EQ(word[0], "আ");
    EXPECT_EQ(word[1], "মা");
    EXPECT_EQ(word[2], "র");

    // ZWJ attaches forward, so ক ZWJ ষ breaks after the joiner, not before it.
    const auto zwj = suborno::unicode::split_graphemes("ক\u200Dষ");
    ASSERT_EQ(zwj.size(), 2U);
    EXPECT_EQ(zwj[0], "ক\u200D");
    EXPECT_EQ(zwj[1], "ষ");

    // ZWNJ after a virama stays inside the cluster; the half-form consonant splits.
    const auto zwnj = suborno::unicode::split_graphemes("ক্\u200Cত");
    ASSERT_EQ(zwnj.size(), 2U);
    EXPECT_EQ(zwnj[0], "ক্\u200C");
    EXPECT_EQ(zwnj[1], "ত");

    // A dangling matra is its own cluster.
    const auto dangling = suborno::unicode::split_graphemes("া");
    ASSERT_EQ(dangling.size(), 1U);
    EXPECT_EQ(dangling.front(), "া");
}

// Covers bengali.cpp:170-171 — empty input returns no clusters, and
// independent letters, digits and other scripts split per code point.
TEST(UnicodeCoverage, SplitGraphemesSplitsIndependentLettersDigitsAndScripts) {
    EXPECT_TRUE(suborno::unicode::split_graphemes("").empty());

    const auto ka = suborno::unicode::split_graphemes("ক");
    ASSERT_EQ(ka.size(), 1U);
    EXPECT_EQ(ka.front(), "ক");

    const auto digits = suborno::unicode::split_graphemes("০১২");
    ASSERT_EQ(digits.size(), 3U);
    EXPECT_EQ(digits[0], "০");
    EXPECT_EQ(digits[1], "১");
    EXPECT_EQ(digits[2], "২");

    const auto ascii = suborno::unicode::split_graphemes("a b");
    ASSERT_EQ(ascii.size(), 3U);
    EXPECT_EQ(ascii[0], "a");
    EXPECT_EQ(ascii[1], " ");
    EXPECT_EQ(ascii[2], "b");

    const auto mixed_scripts = suborno::unicode::split_graphemes("aあ");
    ASSERT_EQ(mixed_scripts.size(), 2U);
    EXPECT_EQ(mixed_scripts[0], "a");
    EXPECT_EQ(mixed_scripts[1], "あ");
}

// Covers bijoy.cpp:219-230 — every outcome of the decomposed-nuqta matcher:
// string end, non-nuqta follower, matching base, and a base with no mapping.
TEST(UnicodeCoverage, BijoyEncodesDecomposedNuqtaLettersAndDropsBareNuqta) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BC"), "R");  // ড় decomposed.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A2\u09BC"), "Z");  // ঢ় decomposed.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09AF\u09BC"), "Y");  // য় decomposed.
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("\u09A1\u09BC"));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("\u09A2\u09BC"));

    // A nuqta letter with no base match: ক + ় falls through to the plain
    // glyph path and the bare nuqta itself is dropped as unmapped.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u0995\u09BC"), "K");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u0995\u09BC"));

    // Matcher bails before the loop at end of text and on a non-nuqta follower.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ড"), "g");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BE"), "gv");

    // A mapped nuqta letter followed by another consonant keeps both bytes.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BCখ"), "RX");
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("\u09A1\u09BCখ"));
}

// Covers bijoy.cpp:257-286,332,360,380 — the emit_consonant_unit matra cases
// reached through the conjunct, reph and nuqta paths (not just plain glyphs).
TEST(UnicodeCoverage, BijoyAttachesLeftAndTwoPartMatrasToWholeClusterUnits) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষে"), "eK+S");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষৈ"), "qK+S");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষ\u09CB"), "eK+Sv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষ\u09CC"), "eK+So");

    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BC\u09C7"), "eR");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BC\u09C8"), "qR");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BC\u09CB"), "eRv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BC\u09CC"), "eRo");

    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ক\u09CB"), "eK^v");
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ক\u09C8"), "qK^");
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("ক্ষ\u09CB"));
}

// Covers bijoy.cpp:354-363 — reph is emitted only when র্ precedes a mapped
// consonant; other followers fall back to plain letters plus hasanta bytes.
TEST(UnicodeCoverage, BijoyEmitsRephOnlyWhenAConsonantFollowsTheVirama) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্"), "r+");  // virama at end of text.
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্া"), "r+v");  // matra, not a consonant.
    EXPECT_EQ(suborno::unicode::encode_bijoy("কর্"), "Kr+");  // trailing reph carrier.
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ৎ"), "-^");  // khanda ta counts as consonant.
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ষ"), "S^");  // ordinary consonant base.
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("র্ৎ"));
}

// Covers bijoy.cpp:356 — a consonant-range code point with no glyph mapping
// (unassigned U+09A9 inside ক..হ) skips the reph fast path and is dropped.
TEST(UnicodeCoverage, BijoyFallsBackToPlainLettersWhenTheRephBaseHasNoGlyph) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্\u09A9"), "r+");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("র্\u09A9"));
}

// Covers bijoy.cpp:367-376 — standalone two-part matras emit both halves
// wherever they appear: after vowels, after ASCII and after dropped glyphs.
TEST(UnicodeCoverage, BijoyEncodesStandaloneTwoPartMatrasWhereverTheyAppear) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("অ\u09CB"), "aev");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঔ\u09CC"), "Peo");
    EXPECT_EQ(suborno::unicode::encode_bijoy("k\u09CB"), "kev");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u098D\u09CB"), "ev");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u098D\u09CB"));
}

// Covers bijoy.cpp:293-318 — the decoder walks 1/2/3/4-byte UTF-8 sequences
// and their continuation bytes before the mapping tables see the code points.
TEST(UnicodeCoverage, BijoyDecodesOneToFourByteUtf8Sequences) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("a"), "a");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক"), "K");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xC3\xA9"), "");  // é decodes, then drops.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xF0\x9F\x98\x80"), "");
    EXPECT_EQ(suborno::unicode::encode_bijoy("a\xC3\xA9ক\xF0\x9F\x98\x80"), "aK");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("a\xC3\xA9ক\xF0\x9F\x98\x80"));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("aক"));
}

// Covers bijoy.cpp:396-397 — unmapped code points are dropped from the output
// while the rest of the line still encodes, and the text reports unencodable.
TEST(UnicodeCoverage, BijoyDropsUnmappedCodePointsAndFlagsTheTextUnencodable) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("café \u09F3"), "caf $");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("café \u09F3"));
    EXPECT_EQ(suborno::unicode::encode_bijoy("১\u09FA২"), "12");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক日খ"), "KX");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u200Bক"), "K");
}

// Covers bijoy.cpp:201-210 — passthrough boundaries: 0x20..0x7E and the
// 0x09..0x0D whitespace run pass through; 0x08 and 0x7F are dropped.
TEST(UnicodeCoverage, BijoyPassesThroughAsciiBoundariesAndWhitespaceBytes) {
    EXPECT_EQ(suborno::unicode::encode_bijoy(" "), " ");
    EXPECT_EQ(suborno::unicode::encode_bijoy("~"), "~");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\t\n\v\f\r"), "\t\n\v\f\r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("aZ0 9!"), "aZ0 9!");
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("\t\n\v\f\r"));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable(" "));

    std::string del_input = "ক";
    del_input += '\x7F';
    del_input += "খ";
    EXPECT_EQ(suborno::unicode::encode_bijoy(del_input), "KX");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable(del_input));

    std::string backspace_input = "ক";
    backspace_input += '\x08';
    backspace_input += "খ";
    EXPECT_EQ(suborno::unicode::encode_bijoy(backspace_input), "KX");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable(backspace_input));

    // Covers bijoy.cpp:326-329 — empty input encodes to empty and is encodable.
    EXPECT_EQ(suborno::unicode::encode_bijoy(""), "");
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable(""));
}

// Covers bijoy.cpp:413-417,432-434 — invalid UTF-8 input is returned
// byte-for-byte by encode_bijoy and reported unencodable, for lead shapes the
// existing suite does not cover.
TEST(UnicodeCoverage, BijoyReturnsInvalidUtf8ByteForByteAndReportsItUnencodable) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xF5\x80\x80\x80"), "\xF5\x80\x80\x80");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xC2"), "\xC2");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xF8\x80\x80\x80\x80"), "\xF8\x80\x80\x80\x80");
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xF5\x80\x80\x80"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xC2"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xF8\x80\x80\x80\x80"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xC0\x80"));
}

// Covers bijoy.cpp:326-399 — the full walker on one realistic line mixing
// clusters, reph-free matras, digits and currency in print order.
TEST(UnicodeCoverage, BijoyEncodesARenderedLineWithClustersDigitsAndCurrency) {
    const std::string line = "মাতৃভাষা বাংলা ১২৩\u09F3";
    EXPECT_EQ(suborno::unicode::encode_bijoy(line), "mvtfCvSv cv'lv 123$");
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable(line));
    EXPECT_EQ(suborno::unicode::encode_bijoy(suborno::unicode::encode_bijoy(line)),
              suborno::unicode::encode_bijoy(line));
}
