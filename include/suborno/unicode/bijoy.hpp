#pragma once

#include <string>

namespace suborno::unicode {

// Legacy Bijoy (ANSI) encoding for committed Bangla text.
//
// The `preferred_encoding = ansi` product setting requires committed Bangla
// text to be rendered in the legacy Bijoy byte encoding rather than Unicode:
// Bangladesh desktop publishing (DTP) workflows — newspaper composition,
// printing presses and legacy page-layout tools — are built around fonts
// (SutonnyMJ and relatives) whose glyphs are addressed by plain ASCII bytes
// (0x20–0x7E) instead of Bengali code points. This header provides the
// portable Unicode → Bijoy direction of that conversion; it is intentionally
// one-way (Bijoy bytes carry no canonical decomposition back to Unicode).
//
// Clean-room provenance: the byte assignments are derived from first
// principles around the publicly known Bijoy/national keyboard conventions,
// not copied from any third-party mapping table. The anchored core (ক=K,
// খ=X, গ=I, ঘ=L, ঙ=:, চ=;) follows the documented Bijoy ASCII layout; the
// remaining glyphs follow the conventions documented in bijoy.cpp:
// related sounds share a key via case (base byte = plain/short member,
// shifted byte = long/aspirated member), and every row carries its rationale.
//
// Ordering rules encoded here (mirroring how Bijoy prints):
//   - the ে/ৈ left matras are emitted BEFORE the consonant they follow in
//     Unicode (কে → "eK"), and ো/ৌ are composites of ে + the consonant +
//     া/ৗ ("eKv"/"eKo");
//   - reph (র্ attached to a following consonant) is emitted AFTER the base
//     consonant (র্ক → "Kr"), the reverse of the Unicode order;
//   - conjuncts are stored as explicit hasanta-joined components (ক্ষ →
//     "K+S"), which is how legacy Bijoy text embeds conjuncts;
//   - ZWNJ/ZWJ are dropped (Bijoy has no zero-width characters);
//   - ASCII printable text, space, tab and newlines pass through untouched.
//
// Reuse note: the Windows text service calls this from the commit path when
// the user selects ANSI encoding, so the functions never throw and never
// allocate beyond the returned string.

// Converts Unicode Bengali text (UTF-8) to the Bijoy ASCII legacy encoding —
// every byte of the result is in the ASCII range. ASCII text passes through.
// If `utf8_bangla` is not valid UTF-8 the input is returned unchanged. Bengali
// glyphs without a Bijoy mapping (rare Vedic/archaic marks and code points
// outside the Bengali block) are dropped from the output; callers that need
// loss-free encoding should check is_bijoy_encodable() first.
[[nodiscard]] std::string encode_bijoy(const std::string& utf8_bangla);

// True when every Bengali grapheme of `utf8_bangla` has a Bijoy mapping, so
// that encode_bijoy() is loss-free for this input. ASCII letters, digits,
// punctuation, whitespace and the zero-width joiner/non-joiner marks are
// encodable (the latter are removed during encoding); unknown code points —
// including non-ASCII characters outside the mapped Bengali inventory — make
// the result false, as does invalid UTF-8.
[[nodiscard]] bool is_bijoy_encodable(const std::string& utf8_bangla);

}  // namespace suborno::unicode
