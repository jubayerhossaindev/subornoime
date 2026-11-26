# BDS 1738:2018 National Layout — Discrepancy Report (legacy reference IME রেফারেন্স বনাম Official Diagram বনাম SubornoIME)

**তারিখ**: 2026-11-24
**Mission**: National layout positional rewrite — official diagram (Unicode
L2/19-079 Figure 4) এর প্রতিটি key-এর চারটি Bangla layer, legacy reference IME-এর output
এবং SubornoIME-এর (পরিবর্তনের আগে ও পরে) mapping পাশাপাশি মিলিয়ে
key-by-key discrepancy নির্ণয় ও ফিক্স।
**Evidence**: `reports/standards/evidence-raw/bds_national_positional_engine_rows.csv`
(121/121 PASS), `tests/unit/national_layout_test.cpp` (20 tests PASS),
`tests/unit/layout_typing_regression_test.cpp`, spec
`docs/specifications/national-keymap.md`।
**Status honesty note**: এই comparison official **diagram**-level evidence
(Unicode-এর public document-এ প্রকাশিত National Keyboard of Bangladesh
(BDS 1738:2018) চিত্র) এর ভিত্তিতে হয়েছে। Authorized full standard text এখনো
অর্জিত হয়নি (GAP-01 অমীমাংসিত), তাই clause-level binding **BLOCKEDই থাকল** —
কোনো status জোর করে GREEN করা হয়নি।

---

## 1. Executive summary (সারসংক্ষেপ)

legacy reference IME-এর GitHub repository-র `National.cpp` থেকে ১৩৬টি regex rule-এর
সম্পূর্ণ factual mapping extract করা হয়েছে (GPL-3.0 source থেকে শুধু
key→character **facts**; কোনো code copy হয়নি — MIT clean-room)। Official
BDS 1738:2018 National Keyboard diagram (Unicode L2/19-079, Figure 4) থেকে
প্রতিটি keycap-এর চারটি Bangla position পড়া হয়েছে। SubornoIME-র আগের
"National" layout আসলে একটি **phonetic prototype** ছিল (README-তেও "National
prototype" লেখা ছিল) — positional Jatiyo layout ছিল না। এই phase-এ
SubornoIME-র National layout **সম্পূর্ণ positional হিসেবে rewrite** করা হয়েছে।

**ফলাফল**: 47 physical key-এর মধ্যে সব letter/vowel-sign/digit key-এর base ও
Shift layer **official diagram ও legacy reference IME উভয়ের সাথে exact match**। Link
(AltGr) layer — যা legacy reference IME-এর regex engine-এ `g`-prefix sequence দিয়ে
emulate করা কিন্তু আলাদা physical modifier layer হিসেবে নেই (user-এর Finding
1) — SubornoIME-তে এখন **আসল Ctrl+Alt layer** হিসেবে বাস্তবায়িত। দুটি
Assamese character (ৰ, ৱ) user directive অনুযায়ী **excluded**। Windows
runtime verification BLOCKEDই আছে (Windows 10/11 x64 environment নেই)।

| Category | সংখ্যা |
|---|---|
| Official diagram + legacy reference IME + SubornoIME (নতুন) — full match | 47 key × 4 layer মিলিত verification |
| legacy reference IME fact → SubornoIME নতুন table-এ গৃহীত | 111 mapping row |
| SubornoIME-তে সচেতনভাবে excluded (Assamese ৰ/ৱ) | 2 Link position |
| legacy reference IME-এ থাকা কিন্তু অপ্রয়োজনীয়/duplicate rule (g^ = g6 duplicate, g" → `` ` `` quirk) | documented, behavior-identical |
| Windows runtime verification | BLOCKED (environment নেই) |

---

## 2. পদ্ধতি (Method)

1. **legacy reference IME source extraction**: `National.cpp` (GPL-3.0) থেকে 136 rule
   python দিয়ে parse করে প্রতিটি output-এর exact code point
   (`unicodedata.name` সহ) verify করা হয়েছে। Regular ও Traditional — দুই
   typing style-এর rule আলাদা করে বের করা হয়েছে।
2. **Official diagram reading**: Unicode L2/19-079 PDF (8 page)-এর page 4-এর
   Figure 4 কে row-by-row crop করে প্রতিটি keycap-এর চার Bangla position পড়া
   হয়েছে। Diagram legend: "লিংক শিফট বাংলা" (top-left), "শিফট বাংলা"
   (top-right), "ইংরেজি" (centre), "লিংক স্বাভাবিক বাংলা" (bottom-left),
   "স্বাভাবিক বাংলা" (bottom-right)।
3. **Three-way comparison**: প্রতিটি key-এর জন্য diagram-এর position vs
   legacy reference IME fact vs SubornoIME (পুরনো phonetic + নতুন positional) তুলনা।
4. **Independent engine evidence**: `scripts/layout_verify/
   national_positional_verify.cpp` — স্বাধীনভাবে transcribe করা expected
   table বনাম বাস্তব engine output (string API + real session KeyEvents),
   121 row, সব PASS।

---

## 3. Key-by-key comparison (মূল টেবিল)

নিচের টেবিলে প্রতিটি row: **key → [diagram base | diagram shift] বনাম
[legacy reference IME fact] বনাম [SubornoIME আগে (phonetic)] বনাম [SubornoIME এখন]**।
"◆" দিয়ে মার্ক করা ঘরগুলো exact match।

### 3.1 Q row (top letter row)

| Key | Diagram (base/shift) | Legacy reference IME | SubornoIME আগে | SubornoIME এখন |
|---|---|---|---|---|
| q | ঙ ◆ / ং ◆ | q→ঙ, Q→ং ◆ | "ng" token (contextual) | ঙ ◆ / ং ◆ |
| w | য ◆ / য় ◆ | w→য, W→য় ◆ | "y" → য | য ◆ / য় ◆ (decomposed) |
| e | ড ◆ / ঢ ◆ | e→ড, E→ঢ ◆ | "D"/"dh" → ড/ঢ (phonetic) | ড ◆ / ঢ ◆ |
| r | প ◆ / ফ ◆ | r→প, R→ফ ◆ | "r" → র | প ◆ / ফ ◆ |
| t | ট ◆ / ঠ ◆ | t→ট, T→ঠ ◆ | "t"/"T" → ত/ট | ট ◆ / ঠ ◆ |
| y | চ ◆ / ছ ◆ | y→চ, Y→ছ ◆ | "ch" → ছ | চ ◆ / ছ ◆ |
| u | জ ◆ / ঝ ◆ | u→জ, U→ঝ ◆ | "j"/"jh" → জ/ঝ | জ ◆ / ঝ ◆ |
| i | হ ◆ / ঞ ◆ | i→হ, I→ঞ ◆ | "i" → ই (vowel!) | হ ◆ / ঞ ◆ |
| o | গ ◆ / ঘ ◆ | o→গ, O→ঘ ◆ | "o" → ও (vowel!) | গ ◆ / ঘ ◆ |
| p | ড় ◆ / ঢ় ◆ | p→ড়, P→ঢ় ◆ | "p"/"ph" → প/ফ | ড় ◆ / ঢ় ◆ |

**মূল discrepancy এখানেই ছিল**: আগের phonetic prototype-এ একই physical key
ভিন্ন অক্ষর দিত (i → ই, r → র, p → প)। এখন positional fact প্রতিটি key-এ
official diagram অনুযায়ী বসেছে।

### 3.2 A row (home row)

| Key | Diagram (base/shift) | Legacy reference IME | SubornoIME আগে | SubornoIME এখন |
|---|---|---|---|---|
| a | ৃ ◆ / র্ (reph) | a→ৃ, A→র্ (contextual) ◆ | "a" → আ | ৃ ◆ / র্ (reph engine) |
| s | ু ◆ / ূ ◆ | s→ু, S→ূ ◆ | "s" → স | ু ◆ / ূ ◆ |
| d | ি ◆ / ী ◆ | d→ি, D→ী ◆ | "d"/"dh" → দ/ধ | ি ◆ / ী ◆ |
| f | া ◆ / অ ◆ | f→া, F→অ ◆ | "f" → ফ | া ◆ / অ ◆ |
| g | ্ ◆ / । ◆ | g→্, G→। ◆ | "g"/"gh" → গ/ঘ | ্ ◆ / । ◆ |
| h | ব ◆ / ভ ◆ | h→ব, H→ভ ◆ | "h" → হ | ব ◆ / ভ ◆ |
| j | ক ◆ / খ ◆ | j→ক, J→খ ◆ | "j" → জ | ক ◆ / খ ◆ |
| k | ত ◆ / থ ◆ | k→ত, K→থ ◆ | "k"/"kh" → ক/খ | ত ◆ / থ ◆ |
| l | দ ◆ / ধ ◆ | l→দ, L→ধ ◆ | "l" → ল | দ ◆ / ধ ◆ |

### 3.3 Z row (bottom row)

| Key | Diagram (base/shift) | Legacy reference IME | SubornoIME আগে | SubornoIME এখন |
|---|---|---|---|---|
| z | ্র ◆ / ্য ◆ | z→্র, Z→্য ◆ | নেই | ্র ◆ / ্য ◆ |
| x | ও ◆ / ৗ ◆ | x→ও, X→ৗ ◆ | "X" → hasanta break | ও ◆ / ৗ ◆ |
| c | ে ◆ / ৈ ◆ | c→ে, C→ৈ ◆ | "c"/"ch" → চ/ছ | ে ◆ / ৈ ◆ |
| v | র ◆ / ল ◆ | v→র, V→ল ◆ | "v" → ভ | র ◆ / ল ◆ |
| b | ন ◆ / ণ ◆ | b→ন, B→ণ ◆ | "b"/"bh" → ব/ভ | ন ◆ / ণ ◆ |
| n | স ◆ / ষ ◆ | n→স, N→ষ ◆ | "n" → ন (contextual) | স ◆ / ষ ◆ |
| m | ম ◆ / শ ◆ | m→ম, M→শ ◆ | "m" → ম | ম ◆ / শ ◆ |

### 3.4 Digit row

| Key | Diagram base ◆ | Diagram shift | legacy reference IME ◆ | SubornoIME আগে | SubornoIME এখন |
|---|---|---|---|---|---|
| 1..0 | ১..০ | `! @ # ৳ % ^ ঁ * ( )` | digits→বাংলা; $→৳, &→ঁ ◆ | 0..9→১..৯ | ১..০ ◆; Shift: ! @ # ৳ % ^ ঁ * ( ) ◆ |

**Shift+4 → ৳ এবং Shift+7 → ঁ** — National layout-এর signature positions;
diagram, legacy reference IME ও নতুন implementation — তিনটিই match। আগের prototype-এ ৳/ঁ
একদম টাইপ করা যেত না (ঠিক করা হয়েছে — GAP-04-এর "previously missing
characters" অংশ)।

### 3.5 Link (AltGr) layer — user-এর Finding 1-এর সমাধান

legacy reference IME-এর `InitKeyStrokeTable()`-এ explicit AltGr physical layer নেই; তারা
regex sequence (`g` + key) দিয়ে emulate করে। Official diagram legend-এ
"লিংক বাংলা"/"লিংক শিফট বাংলা" position স্পষ্টভাবে আছে। SubornoIME এখন
আসল Ctrl+Alt layer দেয়:

| Keys | legacy reference IME sequence (fact) | Diagram position | SubornoIME এখন |
|---|---|---|---|
| Link+1..5 | g1..g5 → ৴ ৵ ৶ ৷ ৸ ◆ | bottom-left ◆ | Ctrl+Alt+1..5 → ৴ ৵ ৶ ৷ ৸ ◆ |
| Link+0 | g0 → ৹ ◆ | bottom-left ◆ | Ctrl+Alt+0 → ৹ ◆ |
| Link+6/7/8/9 | g6→ৠ, g7→৺, g8→ৡ, g9→ঌ ◆ | bottom-left ◆ | Ctrl+Alt+6..9 → ৠ ৺ ৡ ঌ ◆ |
| Link+Shift+1..9,0 | g!→ঁ? নয় — g!→ঀ(U+0980), g#→ঽ, g$→৲, g%→ৄ, g^→ৠ, g&→৾, g*→ৣ, g(→ৢ, g)→৽ ◆ | top-left ◆ | Ctrl+Alt+Shift+… → ঀ ঽ ৲ ৄ ৠ ৾ ৣ ৢ ৽ ◆ |
| Link+f/d/s/a/c/x | gf→আ, gd→ই, gs→উ, ga→ঋ, gc→এ, gx→ো ◆ | bottom-left ◆ | Ctrl+Alt+… → আ ই উ ঋ এ ো ◆ |
| Link+Shift+D/S/C/X | gD→ঈ, gS→ঊ, gC→ঐ, gX→ঔ ◆ | top-left ◆ | Ctrl+Alt+Shift+… → ঈ ঊ ঐ ঔ ◆ |
| Link+Shift+G | gG→॥ ◆ | top-left ◆ | Ctrl+Alt+Shift+G → ॥ ◆ |
| Link+- / Link+Shift+- | g-→×, g_→÷ ◆ | bottom-left/top-left ◆ | Ctrl+Alt+- → ×; +Shift → ÷ ◆ |
| Link+g | gg→্‌ (double-hasanta sequence) | — | g,g sequence → ্‌ (ZWNJ); Ctrl+Alt+G = empty |
| Link+`,` / Link+`.` | g,→ৰ, g.→ৱ (Assamese) ◆ | diagram-এ বর্ণিত | **excluded** (empty) — দেখুন §5 |

**নোট**: legacy reference IME-এ `g^` (Link+Shift+6) ও `g6` (Link+6) উভয়ই ৠ — duplicate
rule; SubornoIME এটি হুবহু রেখেছে এবং report করেছে (behaviour-identical)।

### 3.6 Punctuation

| Key | Diagram | legacy reference IME ◆ | SubornoIME আগে | SubornoIME এখন |
|---|---|---|---|---|
| `\` | ৎ (base) ◆ / ঃ (shift) ◆ | \→ৎ, \|→ঃ ◆ | নেই (X-break ছিল) | ৎ ◆ / ঃ ◆ |
| `` ` `` | `` ` `` / “ | ~→“ ◆ | নেই | `` ` `` ◆ / “ ◆ |
| `'` | ' / `` ` `` | "→`` ` `` ◆ | নেই | ' ◆ / `` ` `` ◆ |
| `[ ] ; , . /` | English only ◆ | pass-through ◆ | pass-through | pass-through ◆ |

---

## 4. Audit findings প্রতি সিদ্ধান্ত (user-এর 4 finding-এর disposition)

| Finding | User-এর সিদ্ধান্ত | এই phase-এ প্রয়োগ |
|---|---|---|
| 1. Four-layer implementation প্রশ্ন (AltGr/Link layer নেই) | "Needs verification" | **যাচাই হয়েছে**: legacy reference IME-এ Link layer নেই, g-prefix emulate মাত্র; SubornoIME এখন আসল Ctrl+Alt layer দেয় — user-এর সন্ধান সমাধান হয়েছে |
| 2. README "Tested (WIP)" | "Independently verified compliance ধরা হয়নি" | **সম্মানিত**: legacy reference IME-কে factual reference হিসেবেই ব্যবহার করা হয়েছে; কোনো BDS-compliance দাবি করা হয়নি; KV-03 clause binding BLOCKEDই আছে |
| 3. Assamese ৰ/ৱ mapping | "Exclude from SubornoIME" | **প্রয়োগ করা হয়েছে**: Link+`,`/Link+`.` empty; `AssameseLinkCharactersAreExcluded` test assert করে |
| 4. GPL-3.0 license | "Mapping reference হিসেবে, স্বাধীন implementation" | **প্রয়োগ করা হয়েছে**: শুধু key→character facts নেওয়া হয়েছে (facts copyrightable নয়); regex rule structure, code, table format — কিছুই copy হয়নি; স্বাধীন C++ scan-code table + session engine; IP note spec-এ recorded |

---

## 5. সচেতন deviation (documented deviations)

1. **Assamese ৰ (U+09F0) / ৱ (U+09F1)**: official Link layer-এ থাকলেও
   SubornoIME inventory decision অনুযায়ী excluded। Position empty, কোনো key
   overwrite হয়নি।
2. **Traditional typing style**: legacy reference IME-এ Regular (modern standard) ও
   Traditional (classic, vowel-sign-first) — দুই mode আছে। SubornoIME শুধু
   **Regular** implement করেছে (modern standard = user-এর নির্দেশিত
   standard); Traditional mode documented limitation।
3. **g-prefix emulation নেই**: legacy reference IME-এর regex engine g-prefix sequence
   (যেমন `gf` → আ) ব্যবহার করে কারণ তাদের আসল Link modifier নেই।
   SubornoIME-তে আসল AltGr আছে, তাই g-prefix emulation **সচেতনভাবে বাদ** —
   এটি legacy reference IME-এর implementation artifact, National layout-এর মৌলিক
   behavior নয়। ফলে string API-তে `gf` টাইপ করলে ্ + া literal output হয়
   (এটিই positional সঠিক আচরণ)।
4. **legacy reference IME duplicate**: g6/g^ উভয়ই ৠ — হুবহু রাখা হয়েছে।

---

## 6. Verification evidence (প্রমাণ)

| Evidence | Result |
|---|---|
| `reports/standards/evidence-raw/bds_national_positional_engine_rows.csv` | 121/121 PASS (স্বাধীন expected table vs real engine) |
| Unit suite | 609/609 PASS (আগে 591; National rewrite-এ +18 net) |
| Integration | 2/2 PASS |
| ASan/UBSan (GCC) | 609 + 2 PASS, zero diagnostics |
| Other 6 layouts | কোনো source change হয়নি — non-interference নিশ্চিত |

## 7. যা এখনো দাবি করা যাবে না (residual honesty)

- **Clause-level BDS 1738:2018 equivalence**: authorized standard text না
  পাওয়া পর্যন্ত **BLOCKED**। বর্তমান verification standard-এর official
  diagram-এর public republishing (Unicode L2/19-079) এর ভিত্তিতে — এটি
  strong reference, কিন্তু purchased standard-এর clause table নয়।
- **Windows runtime**: W-01..W-10 campaign Windows 10/11 x64 ছাড়া
  **BLOCKED**। Physical typing, Notepad, installer — সব unverified।
- **BSTI certification**: inquiry draft ready কিন্তু পাঠানো হয়নি; eligibility
  **PENDING**।

---

*Report body বাংলায়; Technical term / function name / file path / test ID
English-এ (প্রচলিত project convention অনুযায়ী)।*
