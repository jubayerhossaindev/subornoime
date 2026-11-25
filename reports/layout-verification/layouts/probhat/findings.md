# Findings — Probhat

- Physical scan-code path-এ সব প্রোব PASS — পূর্বের 82/83 (BRIEF-060 NOT_TYPEABLE) + নতুন নুকতা প্রোব; BRIEF-060 এখন PASS।
- **RESOLVED (2026-11-24)**: standalone ় (U+09BC) এখন AltGr (Ctrl+Alt) + scan 0x34 দিয়ে টাইপ করা যায় — document-কৃত **SubornoIME Probhat Compatibility Extension**। Upstream MIT Probhat KLC (princebillygk/probhat-layout-windows @ debc8310, byte-level re-verified 2026-11-24)-তে U+09BC কোথাও নেই; 0x34-এর AltGr column `-1` — কোনো existing key overwrite হয়নি। (docs/specifications/probhat-keymap.md; source: probhat_layout.cpp)
- NFC commit parity PASS: য় commit হয় U+09AF U+09BC (decomposed, composition exclusion) হিসেবে; নতুন নুকতা আউটপুটও NFC-stable।
- Canonical reordering: ক+্+় → commit-এ ক+়+্ (U+0995 U+09BC U+09CD) — nukta ccc=7 < virama ccc=9, Unicode Standard মেনে।
- Backspace এক grapheme cluster মুছে (ক় = এক cluster — এক BS-এ পুরোটা)।
- AltGr/CapsLock স্তর upstream KLC column অনুযায়ী — শুধু 0x34 AltGr slot documented extension।
