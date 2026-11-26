#include "suborno/unicode/bijoy.hpp"

#include <exception>
#include <optional>
#include <string>
#include <string_view>

#include "suborno/unicode/bengali.hpp"

namespace suborno::unicode {
namespace {

// ---------------------------------------------------------------------------

// Bijoy ASCII byte assignments (clean-room; provenance in bijoy.hpp).

//

// The table is organized by two conventions so every row is defensible:

//

//  1. Anchored core — ক=K, খ=X, গ=I, ঘ=L, ঙ=: (the shifted form of the

//     ';' key that carries চ) and চ=; follow the documented Bijoy ASCII

//     layout and are fixed reference points.

//  2. Key sharing — where the anchor leaves a key free, related sounds share

//     it through case: the base byte carries the plain/short member and the

//     shifted (uppercase) byte carries the long/aspirated partner, e.g.

//     ত=t / থ=T and র=r / ড়=R. This mirrors the anchor's own ; / : pair.

//

// Bytes occupied by the anchor (b, i, l, ...) force documented

// displacements, marked per row below. Where a byte cannot be verified

// against public documentation we prefer a self-consistent, stated choice

// over an invented claim: every assignment below is derivable from the

// conventions stated here plus the anchor.

// ---------------------------------------------------------------------------

struct GlyphMapping {
    char32_t code_point;
    std::string_view bijoy;
};

// Independent vowels, vowel signs (matras), consonants, marks, digits and

// punctuation that map to a single ASCII byte. ো/ৌ are two-part composites

// handled by the emitter, not rows here.

constexpr GlyphMapping glyphs[] = {
    // Independent vowels. Short/plain quality on the base byte, long quality

    // on the shifted byte of the same key (long-a আ=A over অ=a, and so on).

    {U'অ', "a"},  // inherent vowel: anchors the vowel family on the a key.

    {U'আ', "A"},  // long-a partner, shift of অ's byte.

    {U'ই', "b"},  // short i: the i bytes carry the i-matras, so the

    // independent i-vowels take the free b key.

    {U'ঈ', "B"},  // long-i partner, shift of ই's byte.

    {U'উ', "u"},  // short u, the phonetic u byte.

    {U'ঊ', "U"},  // long-u partner, shift of উ's byte.

    {U'ঋ', "F"},  // vocalic r; its matra ৃ keeps the base byte f below.

    {U'এ', "E"},  // e-vowel: shift of the e key that carries the e-matra.

    {U'ঐ', "Q"},  // oi-diphthong, shift of the ai-matra's q byte.

    {U'ও', "O"},  // o-vowel, phonetic o byte.

    {U'ঔ', "P"},  // ou-diphthong: the key immediately right of ও's O key.

    // Vowel signs. Left-side matras (ে, ৈ) and the two-part ো/ৌ are reordered

    // by the emitter; the rest print after their consonant in Unicode order.

    {U'া', "v"},  // aa-kar: 'a' is held by the independent vowel অ.

    {U'ি', "i"},  // i-kar: phonetic i (কি types as K i); 'I' is anchored গ.

    {U'ী', "k"},  // long-i: 'I' is occupied by the anchored গ, so the

    // matra displaces to the free base byte of ক's key.

    {U'ু', "["},  // u-kar: the u bytes carry the u-vowels, so the u-matras

    {U'ূ', "]"},  // take the bracket pair, the last adjacent free byte pair.

    {U'ৃ', "f"},  // ri-kar: base byte of ঋ's key.

    {U'ে', "e"},  // e-kar: emitted BEFORE its consonant (Bijoy prints the

    // left-hand matra in front).

    {U'ৈ', "q"},  // ai-kar: likewise emitted before its consonant.

    {U'ৗ', "o"},  // au-length mark: base byte of ও's key; the right half of

    // the ৌ composite.

    // Consonants. Anchor rows first, then the shared-key series, then the

    // documented displacements.

    {U'ক', "K"},  // anchor.

    {U'খ', "X"},  // anchor: kh on the conventional x key.

    {U'গ', "I"},  // anchor.

    {U'ঘ', "L"},  // anchor.

    {U'ঙ', ":"},  // anchor: shifted form of the ';' key that carries চ.

    {U'চ', ";"},  // anchor.

    {U'ছ', "x"},  // the other aspirated fricative keeps the free base byte

    // of the anchored খ key (X), kh/ch adjacent in the table.

    {U'জ', "j"},  // phonetic j byte.

    {U'ঝ', "J"},  // aspirate partner, shift of জ's byte.

    {U'ঞ', "N"},  // nasal pair on the n key: ny on the shift byte.

    {U'ট', "w"},  // retroflex t-family: the t bytes carry the more frequent

    // dental series (ত=t / থ=T), so the retroflexes displace.

    {U'ঠ', "W"},  // aspirate partner, shift of ট's byte.

    {U'ড', "g"},  // voiced retroflex: the d bytes carry the dental series

    // (দ=d / ধ=D), so this pair takes the free g key.

    {U'ঢ', "G"},  // aspirate partner, shift of ড's byte.

    {U'ণ', "M"},  // retroflex nasal: n/N carry ন/ঞ, so ণ sits beside ম's m.

    {U'ত', "t"},  // dental t, phonetic t byte.

    {U'থ', "T"},  // aspirate partner, shift of ত's byte.

    {U'দ', "d"},  // dental d, phonetic d byte.

    {U'ধ', "D"},  // aspirate partner, shift of দ's byte.

    {U'ন', "n"},  // dental nasal, phonetic n byte.

    {U'প', "p"},  // phonetic p byte.

    {U'ফ', "V"},  // ph: the f bytes carry ৃ/F, so ফ takes the free V byte.

    {U'ব', "c"},  // ba family displaces to the c key because the b bytes

    // carry the independent i-vowels (ই=b / ঈ=B).

    {U'ভ', "C"},  // aspirate partner, shift of ব's byte.

    {U'ম', "m"},  // phonetic m byte.

    {U'য', "y"},  // phonetic y byte.

    {U'র', "r"},  // phonetic r byte.

    // The nuqta letters ড়/ঢ়/য় are not rows here: NFC rewrites them into

    // ড/ঢ/য + U+09BC (they are on the script-specific composition exclusion

    // list), so the walker matches that decomposed sequence below and prints

    // R/Z/Y — the shift bytes of the r/g/y keys.

    {U'ল', "l"},  // phonetic l byte ('L' is anchored ঘ).

    {U'শ', "z"},  // fricatives beyond স/ষ cluster on the free bottom-row

    // bytes: শ on z, next to ছ's x.

    {U'ষ', "S"},  // retroflex sha, shift of স's byte.

    {U'স', "s"},  // phonetic s byte.

    {U'হ', "h"},  // phonetic h byte.

    {U'ৎ', "-"},  // khanda-ta: a deliberately non-letter byte, marking its

    // clipped word-final nature.

    // Signs and special marks.

    {U'ঁ', "~"},  // chandrabindu on the tilde (a raised nasal mark).

    {U'ং', "'"},  // anusvara on the apostrophe (a raised nasal mark).

    {U'ঃ', "H"},  // visarga: the aspirant h-final shares হ's key (shift).

    {U'্', "+"},  // hasanta on a non-letter byte so a virama can never be

    // confused with a consonant byte; conjuncts embed it.

    {U'৳', "$"},  // taka sign on the currency byte.

    // Bengali numerals map positionally to their ASCII digits.

    {U'০', "0"},
    {U'১', "1"},
    {U'২', "2"},
    {U'৩', "3"},
    {U'৪', "4"},
    {U'৫', "5"},
    {U'৬', "6"},
    {U'৭', "7"},
    {U'৮', "8"},
    {U'৯', "9"},

    // Danda punctuation prints as the vertical bar(s) of the legacy layout.

    {U'।', "|"},
    {U'॥', "||"},
};

// Conjuncts, stored as explicit hasanta-joined component bytes. Legacy Bijoy

// text embeds the virama byte inside a cluster and the font renders the

// ligature glyph, so the composed form is exactly what the per-letter rows

// produce. Dedicated single-byte aliases are deliberately not invented: they

// are not part of the publicly documented core. র্ leading a cluster is

// reph, handled separately by the emitter (it prints after the base).

struct ConjunctMapping {
    std::u32string_view unicode;
    std::string_view bijoy;
};

constexpr ConjunctMapping conjuncts[] = {
    {U"ক্ক", "K+K"}, {U"ক্ত", "K+t"}, {U"ক্র", "K+r"},    {U"ক্ষ", "K+S"},    {U"ক্ষ্ণ", "K+S+M"},
    {U"ঙ্ক", ":+K"}, {U"ঙ্গ", ":+I"}, {U"চ্চ", ";+;"},    {U"জ্জ", "j+j"},    {U"জ্ঞ", "j+N"},
    {U"ঞ্চ", "N+;"}, {U"ট্ট", "w+w"}, {U"ড্ড", "g+g"},    {U"ত্ত", "t+t"},    {U"ত্ত্ব", "t+t+c"},
    {U"ত্র", "t+r"}, {U"দ্দ", "d+d"}, {U"দ্ধ", "d+D"},    {U"দ্ম", "d+m"},    {U"ন্ত", "n+t"},
    {U"ন্দ", "n+d"}, {U"ন্ধ", "n+D"}, {U"ন্ন", "n+n"},    {U"ন্ত্র", "n+t+r"}, {U"প্ত", "p+t"},
    {U"প্র", "p+r"}, {U"ব্দ", "c+d"}, {U"ব্ধ", "c+D"},    {U"ব্ল", "c+l"},    {U"ম্প", "m+p"},
    {U"ম্ব", "m+c"}, {U"ম্ভ", "m+C"}, {U"ম্ম", "m+m"},    {U"ম্ত্র", "m+t+r"}, {U"শ্চ", "z+;"},
    {U"শ্র", "z+r"}, {U"ষ্ট", "S+w"}, {U"ষ্ঠ", "S+W"},    {U"স্ক", "s+K"},    {U"স্ত", "s+t"},
    {U"স্থ", "s+T"}, {U"স্প", "s+p"}, {U"স্প্র", "s+p+r"}, {U"স্ল", "s+l"},    {U"হ্ণ", "h+M"},
    {U"হ্ম", "h+m"}, {U"হ্ল", "h+l"},
};

constexpr char32_t virama = U'্';
constexpr char reph_byte = '^';
constexpr char32_t nuqta = 0x09BC;
constexpr char32_t zero_width_non_joiner = 0x200C;
constexpr char32_t zero_width_joiner = 0x200D;

// The nuqta letters after decomposition: NFC never emits U+09DC/U+09DD/U+09DF

// (composition exclusions), so ড় ঢ় য় always arrive as base + U+09BC and are

// encoded with the shift bytes of their base letter's key.

struct NuqtaLetter {
    char32_t base;
    std::string_view bijoy;
};
constexpr NuqtaLetter nuqta_letters[] = {
    {U'\u09A1', "R"},  // ড় (rr).

    {U'\u09A2', "Z"},  // ঢ় (rrh); 'Z' is the last free uppercase letter.

    {U'\u09AF', "Y"},  // য় (ya-phashe): য + ়, shift of য's byte.

};

const GlyphMapping* find_glyph(char32_t code_point) {
    for (const GlyphMapping& glyph : glyphs) {
        if (glyph.code_point == code_point) {
            return &glyph;
        }
    }
    return nullptr;
}

bool is_consonant_code_point(char32_t code_point) {
    // ক..হ plus ৎ; the nuqta letters appear only as decomposed base + U+09BC

    // sequences, which the walker matches separately.

    return (code_point >= U'ক' && code_point <= U'হ') || code_point == U'ৎ';
}

bool is_passthrough_ascii(char32_t code_point) {
    // Letters, digits, punctuation and space pass through untouched, along

    // with the remaining ASCII whitespace so line structure survives DTP

    // round-trips. Other control bytes have no legacy rendering and are

    // treated as unmapped.

    if (code_point >= 0x20 && code_point <= 0x7E) {
        return true;
    }
    return code_point >= 0x09 && code_point <= 0x0D;
}

struct ConjunctMatch {
    const ConjunctMapping* entry;
    std::size_t length;
};

// Returns the Bijoy bytes of a decomposed nuqta letter (ড ় / ঢ ় / য ়) at

// position `i`, or nullopt when those code points are something else.

std::optional<std::string_view> match_decomposed_nuqta(const std::u32string& code_points,
                                                       std::size_t i) {
    if (i + 1 >= code_points.size() || code_points[i + 1] != nuqta) {
        return std::nullopt;
    }
    for (const NuqtaLetter& letter : nuqta_letters) {
        if (code_points[i] == letter.base) {
            return letter.bijoy;
        }
    }
    return std::nullopt;
}

// Longest-match lookup against the conjunct table at position `i`. Longest

// wins so ক্ষ্ণ (three code points) beats the shorter ক্ষ prefix.

std::optional<ConjunctMatch> match_conjunct(const std::u32string& code_points, std::size_t i) {
    std::optional<ConjunctMatch> best;
    for (const ConjunctMapping& candidate : conjuncts) {
        const std::size_t length = candidate.unicode.size();
        if (best.has_value() && best->length >= length) {
            continue;
        }
        if (i + length <= code_points.size()
            && code_points.compare(i, length, candidate.unicode) == 0) {
            best = ConjunctMatch{&candidate, length};
        }
    }
    return best;
}

// Emits one consonant-cluster unit, applying the Bijoy print-order rules for

// the matras that surround it: ে/ৈ print before the cluster (they sit on the

// left side in the rendered glyph) and the two-part ো/ৌ split around it as

// ে + cluster + া/ৗ. `i` must already point past the cluster's code points.

void emit_consonant_unit(std::string_view unit,
                         const std::u32string& code_points,
                         std::size_t& i,
                         std::string& out) {
    if (i < code_points.size()) {
        switch (code_points[i]) {
        case U'ে':
            out += 'e';
            out += unit;
            ++i;
            return;
        case U'ৈ':
            out += 'q';
            out += unit;
            ++i;
            return;
        case U'ো':
            out += 'e';
            out += unit;
            out += 'v';
            ++i;
            return;
        case U'ৌ':
            out += 'e';
            out += unit;
            out += 'o';
            ++i;
            return;
        default:
            break;
        }
    }
    out += unit;
}

// Strict UTF-8 decoder for input that is_valid_utf8() has already accepted.

std::u32string decode_utf8(std::string_view text) {
    std::u32string code_points;
    code_points.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        char32_t code_point = 0;
        std::size_t length = 1;
        if (lead < 0x80) {
            code_point = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            length = 2;
            code_point = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
            code_point = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
            code_point = lead & 0x07;
        } else {
            return {};  // unreachable: the caller validated the input first
        }
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto continuation = static_cast<unsigned char>(text[i + offset]);
            code_point = (code_point << 6) | (continuation & 0x3F);
        }
        code_points.push_back(code_point);
        i += length;
    }
    return code_points;
}

// Shared engine for encode_bijoy() and is_bijoy_encodable(): walks the

// (already NFC-normalized) code points and appends Bijoy bytes to `out`.

// Returns true when the input encodes loss-free; unmapped code points are

// skipped in the output but reported through the return value, so callers

// can either commit the result or fall back to Unicode encoding.

bool append_bijoy_bytes(const std::u32string& code_points, std::string& out) {
    bool encodable = true;
    std::size_t i = 0;
    while (i < code_points.size()) {
        if (auto conjunct = match_conjunct(code_points, i); conjunct.has_value()) {
            i += conjunct->length;
            emit_consonant_unit(conjunct->entry->bijoy, code_points, i, out);
            continue;
        }

        const char32_t code_point = code_points[i];

        // Bijoy has no zero-width characters: the joiners are layout hints

        // for Unicode shaping only and vanish in the legacy byte stream.

        if (code_point == zero_width_non_joiner || code_point == zero_width_joiner) {
            ++i;
            continue;
        }
        if (is_passthrough_ascii(code_point)) {
            out += static_cast<char>(code_point);
            ++i;
            continue;
        }

        // Reph (র্ attached to a following consonant) prints AFTER the base

        // consonant in Bijoy — the reverse of the Unicode order. A dedicated

        // reph byte keeps র্প ("p^") distinguishable from the plain cluster

        // পর ("pr"), which the naive reordering would collapse.

        if (code_point == U'র' && i + 2 < code_points.size() && code_points[i + 1] == virama
            && is_consonant_code_point(code_points[i + 2])) {
            if (const GlyphMapping* base = find_glyph(code_points[i + 2]); base != nullptr) {
                std::string unit(base->bijoy);
                unit += reph_byte;
                i += 3;
                emit_consonant_unit(unit, code_points, i, out);
                continue;
            }
        }

        // A two-part matra with no consonant to attach to (word-initial or

        // after a vowel) still encodes: emit both halves in order.

        if (code_point == U'ো') {
            out += "ev";
            ++i;
            continue;
        }
        if (code_point == U'ৌ') {
            out += "eo";
            ++i;
            continue;
        }

        if (auto decomposed_nuqta_match = match_decomposed_nuqta(code_points, i);
            decomposed_nuqta_match.has_value()) {
            i += 2;
            emit_consonant_unit(*decomposed_nuqta_match, code_points, i, out);
            continue;
        }

        if (const GlyphMapping* glyph = find_glyph(code_point); glyph != nullptr) {
            ++i;
            if (is_consonant_code_point(code_point)) {
                emit_consonant_unit(glyph->bijoy, code_points, i, out);
            } else {
                out += glyph->bijoy;
            }
            continue;
        }

        // Unmapped Bengali/rare code point: dropped from the output, flagged

        // through the return value so is_bijoy_encodable() reports false.

        encodable = false;
        ++i;
    }
    return encodable;
}

std::u32string to_normalized_code_points(const std::string& utf8_bangla) {
    // NFC first: it composes the typed two-part matras (ে+া → ো, ে+ৗ → ৌ) into

    // the single code points the tables above are written against. The nuqta

    // letters stay decomposed (composition exclusions) and are matched as

    // sequences by the walker.

    return decode_utf8(normalize_nfc(utf8_bangla));
}

}  // namespace

std::string encode_bijoy(const std::string& utf8_bangla) {
    if (!is_valid_utf8(utf8_bangla)) {
        // Malformed input is passed through untouched rather than truncated:

        // the caller is committing text, and silently dropping bytes would

        // corrupt the document without any signal that something went wrong.

        return utf8_bangla;
    }
    try {
        std::string out;
        append_bijoy_bytes(to_normalized_code_points(utf8_bangla), out);
        return out;
    } catch (const std::exception&) {
        // normalize_nfc() can only throw on environmental ICU failures

        // (invalid UTF-8 is rejected before it is called). Fall back to the

        // input so the text service never loses a commit.

        return utf8_bangla;
    }
}

bool is_bijoy_encodable(const std::string& utf8_bangla) {
    if (!is_valid_utf8(utf8_bangla)) {
        return false;
    }
    try {
        std::string out;
        return append_bijoy_bytes(to_normalized_code_points(utf8_bangla), out);
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace suborno::unicode
