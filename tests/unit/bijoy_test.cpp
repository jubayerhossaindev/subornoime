#include "suborno/unicode/bijoy.hpp"

#include <gtest/gtest.h>
#include <string>

// Golden bytes for the everyday sentence asserted below. Bijoy print order
// differs from Unicode: the ে matra prints before its consonant ("Amvr..."
// keeps matras after consonants), ো/ৌ split around the cluster ("esvnvr" =
// ে + স + া | ন | া | র), and reph prints after its base ("Dm^" for ধর্ম).
TEST(BijoyEncodingTest, EncodesEverydaySentencesToTheirGoldenBytes) {
    const std::string golden = suborno::unicode::encode_bijoy("আমার সোনার বাংলা");
    EXPECT_EQ(golden, "Amvr esvnvr cv'lv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("বাংলাদেশ"), "cv'lvedz");
    EXPECT_EQ(suborno::unicode::encode_bijoy("বাংলা"), "cv'lv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("আমি"), "Ami");
}

TEST(BijoyEncodingTest, EncodesAllIndependentVowels) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("অ"), "a");
    EXPECT_EQ(suborno::unicode::encode_bijoy("আ"), "A");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ই"), "b");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঈ"), "B");
    EXPECT_EQ(suborno::unicode::encode_bijoy("উ"), "u");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঊ"), "U");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঋ"), "F");
    EXPECT_EQ(suborno::unicode::encode_bijoy("এ"), "E");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঐ"), "Q");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ও"), "O");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঔ"), "P");
}

TEST(BijoyEncodingTest, EncodesConsonantsPerTheAnchoredBijoyAsciiTable) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক"), "K");
    EXPECT_EQ(suborno::unicode::encode_bijoy("খ"), "X");
    EXPECT_EQ(suborno::unicode::encode_bijoy("গ"), "I");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঘ"), "L");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঙ"), ":");
    EXPECT_EQ(suborno::unicode::encode_bijoy("চ"), ";");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ছ"), "x");
    EXPECT_EQ(suborno::unicode::encode_bijoy("জ"), "j");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঝ"), "J");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঞ"), "N");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ট"), "w");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঠ"), "W");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ড"), "g");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঢ"), "G");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ণ"), "M");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ত"), "t");
    EXPECT_EQ(suborno::unicode::encode_bijoy("থ"), "T");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দ"), "d");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ধ"), "D");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন"), "n");
    EXPECT_EQ(suborno::unicode::encode_bijoy("প"), "p");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ফ"), "V");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ব"), "c");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ভ"), "C");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম"), "m");
    EXPECT_EQ(suborno::unicode::encode_bijoy("য"), "y");
    EXPECT_EQ(suborno::unicode::encode_bijoy("র"), "r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ল"), "l");
    EXPECT_EQ(suborno::unicode::encode_bijoy("শ"), "z");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ষ"), "S");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স"), "s");
    EXPECT_EQ(suborno::unicode::encode_bijoy("হ"), "h");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ৎ"), "-");
    // The nuqta letters keep their own bytes.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09DC"), "R");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09DD"), "Z");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09DF"), "Y");
}

TEST(BijoyEncodingTest, PrintsVowelSignsAfterTheConsonantTheyFollow) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("কা"), "Kv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কি"), "Ki");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কী"), "Kk");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কু"), "K[");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কূ"), "K]");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কৃ"), "Kf");
}

TEST(BijoyEncodingTest, MovesTheLeftSideEMatraAndAiMatraBeforeTheConsonant) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("কে"), "eK");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কৈ"), "qK");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দে"), "ed");
    EXPECT_EQ(suborno::unicode::encode_bijoy("মে"), "em");
    // The matra moves in front of the whole conjunct cluster, not just the
    // first component: ত্রে types as e t + r, ন্ত্রে as e n + t + r.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ত্রে"), "et+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন্ত্রে"), "en+t+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্প্রে"), "es+p+r");
}

TEST(BijoyEncodingTest, SplitsOTwoPartKarCompositesAroundTheConsonant) {
    // ো = ে (left) + া (right); ৌ = ে (left) + ৗ (right).
    EXPECT_EQ(suborno::unicode::encode_bijoy("কো"), "eKv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কৌ"), "eKo");
    EXPECT_EQ(suborno::unicode::encode_bijoy("গো"), "eIv");
    // Without a consonant to attach to, both halves still appear in order.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ো"), "ev");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ৌ"), "eo");
}

TEST(BijoyEncodingTest, EmitsRephAfterTheBaseConsonant) {
    // Unicode places র্ before the cluster; Bijoy prints the reph over the
    // base consonant, so the byte follows it.
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ক"), "K^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ল"), "l^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্ম"), "m^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্য"), "y^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ধর্ম"), "Dm^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("সর্ব"), "sc^");
    // A following ে still lands in front of the whole reordered unit.
    EXPECT_EQ(suborno::unicode::encode_bijoy("র্কে"), "eK^");
}

TEST(BijoyEncodingTest, KeepsPlainPostConsonantRaDistinctFromReph) {
    // A plain র after a consonant encodes to its own byte, never the reph
    // byte, so কর ("Kr") and র্ক ("K^") stay distinguishable.
    EXPECT_EQ(suborno::unicode::encode_bijoy("কর"), "Kr");
    EXPECT_EQ(suborno::unicode::encode_bijoy("পর"), "pr");
    EXPECT_EQ(suborno::unicode::encode_bijoy("গর"), "Ir");
}

TEST(BijoyEncodingTest, JoinsRafalaAndYafalaWithHasanta) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্র"), "K+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("প্র"), "p+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্য"), "K+y");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দ্য"), "d+y");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম্য"), "m+y");
}

TEST(BijoyEncodingTest, CollapsesCommonConjunctsToHasantaJoinedComponents) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ক"), "K+K");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ত"), "K+t");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্র"), "K+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঙ্ক"), ":+K");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঙ্গ"), ":+I");
    EXPECT_EQ(suborno::unicode::encode_bijoy("চ্চ"), ";+;");
    EXPECT_EQ(suborno::unicode::encode_bijoy("জ্জ"), "j+j");
    EXPECT_EQ(suborno::unicode::encode_bijoy("জ্ঞ"), "j+N");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ঞ্চ"), "N+;");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ট্ট"), "w+w");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ড্ড"), "g+g");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ত্ত"), "t+t");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ত্র"), "t+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দ্দ"), "d+d");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দ্ধ"), "d+D");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দ্ম"), "d+m");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন্ত"), "n+t");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন্দ"), "n+d");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন্ধ"), "n+D");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন্ন"), "n+n");
    EXPECT_EQ(suborno::unicode::encode_bijoy("প্ত"), "p+t");
    EXPECT_EQ(suborno::unicode::encode_bijoy("প্র"), "p+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ব্দ"), "c+d");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ব্ধ"), "c+D");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ব্ল"), "c+l");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম্প"), "m+p");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম্ব"), "m+c");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম্ভ"), "m+C");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম্ম"), "m+m");
}

TEST(BijoyEncodingTest, CollapsesShaSaAndHaSeriesConjuncts) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("শ্চ"), "z+;");
    EXPECT_EQ(suborno::unicode::encode_bijoy("শ্র"), "z+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ষ্ট"), "S+w");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ষ্ঠ"), "S+W");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্ক"), "s+K");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্ত"), "s+t");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্থ"), "s+T");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্প"), "s+p");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্প্র"), "s+p+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্ল"), "s+l");
    EXPECT_EQ(suborno::unicode::encode_bijoy("হ্ণ"), "h+M");
    EXPECT_EQ(suborno::unicode::encode_bijoy("হ্ম"), "h+m");
    EXPECT_EQ(suborno::unicode::encode_bijoy("হ্ল"), "h+l");
    // Khoyi and its three-component extensions.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষ"), "K+S");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষ্ণ"), "K+S+M");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ত্ত্ব"), "t+t+c");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ন্ত্র"), "n+t+r");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ম্ত্র"), "m+t+r");
}

TEST(BijoyEncodingTest, EncodesNumeralsDandaAndTakaSign) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("০১২৩৪৫৬৭৮৯"), "0123456789");
    EXPECT_EQ(suborno::unicode::encode_bijoy("১০০"), "100");
    EXPECT_EQ(suborno::unicode::encode_bijoy("।"), "|");
    EXPECT_EQ(suborno::unicode::encode_bijoy("॥"), "||");
    EXPECT_EQ(suborno::unicode::encode_bijoy("৳"), "$");
}

TEST(BijoyEncodingTest, EncodesNasalAndVisargaSignsInPlace) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("কাঁ"), "Kv~");
    EXPECT_EQ(suborno::unicode::encode_bijoy("চাঁদ"), ";v~d");
    EXPECT_EQ(suborno::unicode::encode_bijoy("বাং"), "cv'");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দুঃখ"), "d[HX");
    // A trailing hasanta still emits its byte.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্"), "K+");
}

TEST(BijoyEncodingTest, PassesAsciiThroughByteForByte) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("Hello, World! 123"), "Hello, World! 123");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক\nখ\tল\r"), "K\nX\tl\r");
    // Bytes that mean Bangla letters in Bijoy are literal when typed as ASCII.
    EXPECT_EQ(suborno::unicode::encode_bijoy("rK+v"), "rK+v");
}

TEST(BijoyEncodingTest, DropsZeroWidthJoinersAndNonJoiners) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক\u200Dল"), "Kl");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক\u200D"), "K");
    // ক ্ ZWNJ ত requests the explicit half-form, which is exactly the
    // hasanta-joined byte sequence.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্\u200Cত"), "K+t");
}

TEST(BijoyEncodingTest, ReturnsInvalidUtf8UnchangedAndReportsItUnencodable) {
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xC0\xAF"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xED\xA0\x80"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\xE2\x82"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\x80"));
    // encode_bijoy passes malformed input through untouched rather than
    // dropping bytes from committed text.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xC0\xAF"), "\xC0\xAF");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\xE2\x82"), "\xE2\x82");
}

TEST(BijoyEncodingTest, ReportsEncodabilityOfMappedAndUnmappedText) {
    const std::string sentence = "আমার সোনার বাংলা";
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable(""));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("abc"));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("\n\t"));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable(sentence));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("ক্ষ্ণ"));
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("ধর্ম"));

    // Characters outside the mapped inventory: rare Bengali marks, a bare
    // nuqta, non-Bangla scripts and control bytes.
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("café"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\U0001F600"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u098D"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u09FA"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u09BC"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u200B"));
    EXPECT_FALSE(suborno::unicode::is_bijoy_encodable("\u0001"));
}

TEST(BijoyEncodingTest, SkipsUnmappedCharactersInsteadOfEmittingNonAsciiBytes) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক\u098Dখ"), "KX");
    EXPECT_EQ(suborno::unicode::encode_bijoy("café"), "caf");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\U0001F600ক"), "K");
}

TEST(BijoyEncodingTest, EncodingIsIdempotentBecauseOutputIsPureAscii) {
    const std::string sentence = "আমার সোনার বাংলা";
    const std::string once = suborno::unicode::encode_bijoy(sentence);
    EXPECT_EQ(suborno::unicode::encode_bijoy(once), once);
    // Reph and conjunct bytes survive a re-encode untouched.
    EXPECT_EQ(suborno::unicode::encode_bijoy("Dm^"), "Dm^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("K+S"), "K+S");
}

TEST(BijoyEncodingTest, NormalizesDecomposedNuqtaLettersAndTwoPartMatras) {
    // NFC composes ড + ় before the table lookup.
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09A1\u09BC"), "R");
    EXPECT_EQ(suborno::unicode::encode_bijoy("\u09AF\u09BC"), "Y");
    EXPECT_TRUE(suborno::unicode::is_bijoy_encodable("\u09A1\u09BC"));
    // Typed decompositions of the two-part matras compose to ো/ৌ.
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক\u09C7\u09BE"), "eKv");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক\u09C7\u09D7"), "eKo");
}

TEST(BijoyEncodingTest, EncodesEverydayWordsWithClustersAndReph) {
    EXPECT_EQ(suborno::unicode::encode_bijoy("স্কুল"), "s+K[l");
    EXPECT_EQ(suborno::unicode::encode_bijoy("প্রাণ"), "p+rvM");
    EXPECT_EQ(suborno::unicode::encode_bijoy("কৃষ্ণ"), "KfS+M");
    EXPECT_EQ(suborno::unicode::encode_bijoy("দুর্গ"), "d[I^");
    EXPECT_EQ(suborno::unicode::encode_bijoy("ক্ষতি"), "K+Sti");
}
