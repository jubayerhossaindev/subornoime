// Unit tests for the localized-UI-string loader (suborno/core/ui_strings).
// Covers the JSON reader (objects, escapes, surrogate pairs, failures),
// the fallback rules (missing file/key/never clobber), and the shipped
// locale files themselves.

#include "suborno/core/ui_strings.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

using suborno::core::ui_strings::Locale;
using suborno::core::ui_strings::clear;
using suborno::core::ui_strings::get;
using suborno::core::ui_strings::keys;
using suborno::core::ui_strings::load;
using suborno::core::ui_strings::locale_file;
using suborno::core::ui_strings::locale_from_setting;
using suborno::core::ui_strings::size;

namespace {

class TempDir {
  public:
    TempDir() {
        const auto base = std::filesystem::temp_directory_path() / "suborno-ui-strings-tests";
        std::error_code ignored;
        std::filesystem::remove_all(base, ignored);
        std::filesystem::create_directories(base);
        path_ = base / "run";
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(base_, ignored);
    }
    const std::filesystem::path& path() const {
        return path_;
    }

  private:
    std::filesystem::path base_;
    std::filesystem::path path_;
};

void write_file(const std::filesystem::path& path, const std::string& contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream << contents;
}

}  // namespace

TEST(UiStringsTest, ParsesNestedObjectsIntoDottedKeys) {
    TempDir dir;
    write_file(dir.path() / "ui.json",
               "{\"meta\":{\"locale\":\"en\"},"
               "\"topbar\":{\"menu\":{\"settings\":\"Settings…\",\"exit\":\"Exit\"}},"
               "\"settings\":{\"action\":{\"save\":\"Save\"}}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "ui.json", error)) << error;
    EXPECT_EQ(size(), 4u);
    EXPECT_EQ(get("topbar.menu.settings"), "Settings…");
    EXPECT_EQ(get("topbar.menu.exit"), "Exit");
    EXPECT_EQ(get("settings.action.save"), "Save");
    EXPECT_EQ(get("meta.locale"), "en");
    clear();
}

TEST(UiStringsTest, DecodesJsonEscapes) {
    TempDir dir;
    write_file(dir.path() / "ui.json",
               "{\"a\":{\"quote\":\"say \\\"hi\\\"\",\"backslash\":\"a\\\\b\","
               "\"newline\":\"x\\ny\",\"tab\":\"t\\tz\",\"unicode\":\"\\u09AC\\u09BE\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "ui.json", error)) << error;
    EXPECT_EQ(get("a.quote"), "say \"hi\"");
    EXPECT_EQ(get("a.backslash"), "a\\b");
    EXPECT_EQ(get("a.newline"), "x\ny");
    EXPECT_EQ(get("a.tab"), "t\tz");
    // U+09AC (ব) + U+09BE (া) as UTF-8.
    EXPECT_EQ(get("a.unicode"), "\xE0\xA6\xAC\xE0\xA6\xBE");
    clear();
}

TEST(UiStringsTest, DecodesSurrogatePairsIntoUtf8) {
    TempDir dir;
    // U+1F4A9 encoded as a JSON surrogate pair.
    write_file(dir.path() / "ui.json", "{\"glyph\":\"\\uD83D\\uDCA9\"}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "ui.json", error)) << error;
    EXPECT_EQ(get("glyph"), "\xF0\x9F\x92\xA9");
    clear();
}

TEST(UiStringsTest, DecodesOneAndTwoByteUtf8AndAllShortEscapes) {
    TempDir dir;
    write_file(dir.path() / "ui.json",
               "{\"ascii\":\"ok\",\"latin\":\"\\u00E9\",\"esc\":\"a\\/b\\b\\f\\r\\n\\t\"}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "ui.json", error)) << error;
    EXPECT_EQ(get("ascii"), "ok");  // 1-byte UTF-8 path
    EXPECT_EQ(get("latin"), "\xC3\xA9");  // 2-byte UTF-8 path (U+00E9 é)
    EXPECT_EQ(get("esc"), "a/b\b\f\r\n\t");
    clear();
}

TEST(UiStringsTest, RejectsBrokenUnicodeEscapes) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;

    const char* broken[] = {
        "{\"a\": \"\\uZZZZ\"}",  // non-hex digit in the escape
        "{\"a\": \"\\u12\"}",  // truncated escape
        "{\"a\": \"\\u12\\G\"}",  // truncated escape before a bogus escape
        "{\"a\": \"\\uD800x\"}",  // high surrogate not followed by \\u
        "{\"a\": \"\\uD800\\u0041\"}",  // high surrogate followed by a non-low one
    };
    for (const char* text : broken) {
        write_file(dir.path() / "bad.json", text);
        EXPECT_FALSE(load(dir.path() / "bad.json", error)) << text;
        EXPECT_EQ(get("k.v"), "good") << text;
    }
    clear();
}

TEST(UiStringsTest, MissingFileKeepsPreviousTableAndReports) {
    TempDir dir;
    write_file(dir.path() / "first.json", "{\"k\":{\"v\":\"kept\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "first.json", error)) << error;
    EXPECT_EQ(get("k.v"), "kept");

    EXPECT_FALSE(load(dir.path() / "does-not-exist.json", error));
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(get("k.v"), "kept");  // previous strings survive the failure
    clear();
    EXPECT_EQ(get("k.v"), "");
    EXPECT_EQ(size(), 0u);
}

TEST(UiStringsTest, MalformedJsonIsRejectedWithoutClobbering) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;

    const char* malformed[] = {
        "",
        "not json",
        "[]",
        "{\"unterminated\": \"str",
        "{\"escape\": \"bad \\q\"}",
        "{\"lone\": \"\\uD800\"}",  // unpaired high surrogate
        "{\"lone\": \"\\uDC00\"}",  // unpaired low surrogate
        "{\"ctrl\": \"raw \x01 byte\"}",
    };
    for (const char* text : malformed) {
        write_file(dir.path() / "bad.json", text);
        EXPECT_FALSE(load(dir.path() / "bad.json", error)) << text;
        EXPECT_EQ(get("k.v"), "good") << text;
    }
    clear();
}

TEST(UiStringsTest, NonStringLeavesAreSkippedButTableLoads) {
    TempDir dir;
    write_file(dir.path() / "mixed.json",
               "{\"n\": 42, \"t\": true, \"nil\": null, \"arr\": [1, \"x\"],"
               "\"ok\":{\"msg\":\"kept\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "mixed.json", error)) << error;
    EXPECT_EQ(get("ok.msg"), "kept");
    EXPECT_EQ(get("n"), "");
    EXPECT_EQ(get("arr"), "");
    clear();
}

TEST(UiStringsTest, SkipsNestedArraysAndObjectsInValues) {
    TempDir dir;
    write_file(dir.path() / "deep.json",
               "{\"a\": {\"arr\": [1, {\"x\": \"y\", \"deep\": [2, [3, \"z\"]]}, true],"
               "\"s\": \"kept\"}, \"b\": {}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "deep.json", error)) << error;
    EXPECT_EQ(get("a.s"), "kept");
    clear();
}

TEST(UiStringsTest, SkipsSpacedAndEmptyCompositesInsideArrays) {
    TempDir dir;
    write_file(dir.path() / "spaced.json",
               "{\"a\": {\"arr\": [ { \"x\" : 1 } , [] , {} , \"kept\" ]}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "spaced.json", error)) << error;
    EXPECT_EQ(get("a.arr"), "");  // arrays are skipped, not stored
    clear();
}

TEST(UiStringsTest, RejectsMalformedCompositesInsideArrays) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;

    const char* broken[] = {
        "{\"a\": [ {\"x\" 1} ]}",  // object-in-array: missing colon
        "{\"a\": [ {\"x\": } ]}",  // object-in-array: missing value
        "{\"a\": [ {\"x\": 1 ]}",  // object-in-array: never closed
        "{\"a\": [1, {\"x\": 2}",  // enclosing array never closed
        "{\"a\": [",  // array never closed at all
    };
    for (const char* text : broken) {
        write_file(dir.path() / "bad.json", text);
        EXPECT_FALSE(load(dir.path() / "bad.json", error)) << text;
        EXPECT_EQ(get("k.v"), "good") << text;
    }
    clear();
}

TEST(UiStringsTest, RejectsTruncatedEscapeAndBackslashAtEnd) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;

    write_file(dir.path() / "bad.json", "{\"a\": \"x\\");
    EXPECT_FALSE(load(dir.path() / "bad.json", error));
    EXPECT_EQ(get("k.v"), "good");
    write_file(dir.path() / "bad.json", "{\"a\": \"x\\u");
    EXPECT_FALSE(load(dir.path() / "bad.json", error));
    EXPECT_EQ(get("k.v"), "good");
    clear();
}

TEST(UiStringsTest, DecodesLowercaseHexEscapes) {
    TempDir dir;
    write_file(dir.path() / "ui.json", "{\"a\":\"\\u0041\",\"b\":\"\\u00e9\"}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "ui.json", error)) << error;
    EXPECT_EQ(get("a"), "A");  // 1-byte UTF-8 via escape
    EXPECT_EQ(get("b"), "\xC3\xA9");  // lowercase hex digits
    clear();
}

TEST(UiStringsTest, RejectsNonStringMemberInsideArrayObject) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;
    write_file(dir.path() / "bad.json", "{\"a\": [ {1: 2} ]}");
    EXPECT_FALSE(load(dir.path() / "bad.json", error));
    EXPECT_EQ(get("k.v"), "good");
    clear();
}

TEST(UiStringsTest, AcceptsLeadingWhitespaceBeforeRootObject) {
    TempDir dir;
    write_file(dir.path() / "spaced.json", "\n\t {\"a\":{\"v\":\"kept\"}}\n");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "spaced.json", error)) << error;
    EXPECT_EQ(get("a.v"), "kept");
    clear();
}

TEST(UiStringsTest, RejectsMalformedArraysAndNestedObjects) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;

    const char* broken[] = {
        "{\"a\": [1, 2}",  // array never closed
        "{\"a\": [1, }",  // garbage where an element belongs
        "{\"a\": [1 2]}",  // missing separator
        "{\"a\": {\"x\": \"y\"",  // nested object never closed
        "{\"a\": {\"x\" \"y\"}}",  // missing colon
        "{\"a\": {\"x\": }}",  // missing value
        "{\"a\": {: 1}}",  // missing key
        "{\"a\": {\"x\": \"y\"}",  // object never closed
        "{\"a\": {\"x\": \"y\"},",  // trailing comma, no member
        "{\"a\": {\"x\": \"y\"} \"b\": 1}",  // missing comma between members
    };
    for (const char* text : broken) {
        write_file(dir.path() / "bad.json", text);
        EXPECT_FALSE(load(dir.path() / "bad.json", error)) << text;
        EXPECT_EQ(get("k.v"), "good") << text;
    }
    clear();
}

TEST(UiStringsTest, RejectsTrailingContentAfterRootObject) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;
    write_file(dir.path() / "bad.json", "{\"k\":{\"v\":\"x\"}} trailing");
    EXPECT_FALSE(load(dir.path() / "bad.json", error));
    EXPECT_EQ(get("k.v"), "good");
    clear();
}

TEST(UiStringsTest, ReadFailureOnUnreadablePathIsReported) {
    TempDir dir;
    write_file(dir.path() / "good.json", "{\"k\":{\"v\":\"good\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "good.json", error)) << error;
    // Reading a directory: fopen succeeds on POSIX, fread fails -> read_error.
    EXPECT_FALSE(load(dir.path(), error));
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(get("k.v"), "good");
    clear();
}

TEST(UiStringsTest, DistinctNestedKeysCanShareALeafName) {
    TempDir dir;
    write_file(dir.path() / "dup.json", "{\"a\":{\"v\":\"first\"},\"b\":{\"v\":\"second\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "dup.json", error)) << error;
    // "a.v" and "b.v" are distinct dotted keys; a literal duplicate inside
    // one object keeps the first insertion (std::map::emplace).
    EXPECT_EQ(get("a.v"), "first");
    EXPECT_EQ(get("b.v"), "second");
    clear();
}

TEST(UiStringsTest, KeysReturnsSortedDottedList) {
    TempDir dir;
    write_file(dir.path() / "ui.json", "{\"z\":{\"a\":\"1\"},\"a\":{\"z\":\"2\",\"b\":\"3\"}}");
    std::string error;
    ASSERT_TRUE(load(dir.path() / "ui.json", error)) << error;
    const std::vector<std::string> sorted = keys();
    ASSERT_EQ(sorted.size(), 3u);
    EXPECT_EQ(sorted[0], "a.b");
    EXPECT_EQ(sorted[1], "a.z");
    EXPECT_EQ(sorted[2], "z.a");
    clear();
}

TEST(UiStringsTest, LocaleSettingMapping) {
    EXPECT_EQ(locale_from_setting("bn"), Locale::bangla);
    EXPECT_EQ(locale_from_setting("bangla"), Locale::bangla);
    EXPECT_EQ(locale_from_setting("en"), Locale::english);
    EXPECT_EQ(locale_from_setting("system"), Locale::english);  // caller resolves "system"
    EXPECT_EQ(locale_from_setting(""), Locale::english);
    EXPECT_EQ(locale_from_setting("fr"), Locale::english);
}

TEST(UiStringsTest, LocaleFileLayout) {
    const std::filesystem::path root = "/i18n";
    EXPECT_EQ(locale_file(root, Locale::english), std::filesystem::path("/i18n/en/ui.json"));
    EXPECT_EQ(locale_file(root, Locale::bangla), std::filesystem::path("/i18n/bn/ui.json"));
}

TEST(UiStringsTest, NullKeyIsSafe) {
    clear();
    EXPECT_EQ(get(nullptr), "");
}

// The real locale files ship in the repository; both must parse and the
// spot-checked keys must be present in BOTH locales so a Bangla UI switch
// cannot silently fall back mid-sentence.
TEST(UiStringsTest, ShippedLocaleFilesParseWithParallelKeys) {
    const std::filesystem::path i18n = SUBORNO_SOURCE_DIR "/data/i18n";
    std::string error;
    ASSERT_TRUE(load(locale_file(i18n, Locale::english), error)) << error;
    EXPECT_GT(size(), 40u);
    const std::vector<std::string> english_keys = keys();

    const std::string en_settings = get("settings.action.save");
    const std::string en_about = get("about.dialog.title");
    const std::string en_osk = get("osk.window.title");
    const std::string en_topbar = get("topbar.menu.settings");
    EXPECT_FALSE(en_settings.empty());
    EXPECT_FALSE(en_about.empty());
    EXPECT_FALSE(en_osk.empty());
    EXPECT_FALSE(en_topbar.empty());
    clear();

    ASSERT_TRUE(load(locale_file(i18n, Locale::bangla), error)) << error;
    // Same key set in both locales.
    EXPECT_EQ(keys(), english_keys);
    EXPECT_FALSE(get("settings.action.save").empty());
    EXPECT_FALSE(get("about.dialog.title").empty());
    EXPECT_FALSE(get("osk.window.title").empty());
    EXPECT_FALSE(get("topbar.menu.settings").empty());
    EXPECT_EQ(get("osk.mode.bangla"), "বাংলা");
    clear();
}
