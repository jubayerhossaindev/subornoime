# BDS 1520 — Obtained-Edition Clause Verification (PHASE 2)

**মিশন**: BDS FINALIZATION PHASE 2 — Official Standard Acquisition, Clause
Verification & Windows Runtime Completion
**তারিখ**: ২০২৬-১১-২৪ (Asia/Dhaka)
**Obtained document**: "SPECIFICATION FOR BANGLA CODED CHARACTER SET FOR
INFORMATION INTERCHANGE **(Second Revision)**" — page headers **"BDS
1520:2011"** — 10 pages.
**Provenance**: Bangladesh Computer Council official file store (linked from
the BCC IT-standards page, row 25, labelled "BDS 1520:2018");
SHA256 `ab2bc3c26896257f4447d2a1ff88d93c8ddcf21ce5c68e103fbed9743b0cf456`;
accessed 2026-11-24. PDF নিজে repository-র বাইরে রাখা হয়েছে (BSTI copyright);
শুধু allocation facts ও clause index নথিভুক্ত করা হয়েছে।

> **সততার সীমানা (read this first)**
> 1. BCC-র তালিকায় এই PDF-টি "BDS 1520:2018" লেবেলে দেওয়া আছে, কিন্তু
>    document-এর ভিতরের cover ও header দুটোই **Second Revision / 2011**
>    দেখায়, আর official BSTI Catalogue 2023-এ বর্তমান সংস্করণটি **Third
>    Revision (2018)** হিসেবে তালিকাভুক্ত। অর্থাৎ অর্জিত কপিটি **পুরোনো
>    সংস্করণ**। তাই নিচের সব clause verification **obtained edition**
>    (2011 Second Revision) বাঁধা; **2018 Third Revision clause text এখনও
>    অপঠিত — সেই বাঁধা BLOCKED**।
> 2. MISSION-PROGRAM requirement এবং official normative requirement
>    স্পষ্টভাবে আলাদা রাখা হয়েছে — প্রতিটি row-এ ট্যাগ আছে।
> 3. Clause নম্বর কোথাও অনুমান করা হয়নি; সবগুলো সরাসরি অর্জিত দলিল থেকে
>    পড়া (pdftotext + 240 DPI page images দিয়ে visual verification)।

## 1. Document integrity record

| Field | Value |
|---|---|
| Exact title (cover, verbatim) | SPECIFICATION FOR BANGLA CODED CHARACTER SET FOR INFORMATION INTERCHANGE (Second Revision) |
| Page headers (verbatim, repeated) | BDS 1520:2011 |
| Page count | 10 (printed page numbers 5..10 verified on page images) |
| SHA256 | ab2bc3c26896257f4447d2a1ff88d93c8ddcf21ce5c68e103fbed9743b0cf456 |
| Foreword adoption statement | "adopted by the Bangladesh Standards and Testing Institution on 15-02-2011" |
| Basis statement (foreword 0.4) | "based on latest revision of Unicode, a 16-bit coded character set for information interchange, issued by Unicode Consortium" |
| Scope basis (clause 1.1) | "specifies Bangla version of character code which are included in the Unicode version 6.0.0" |
| Committee | Standardization of Bangla for use in ICT (members: BCC, Bangla Academy, Ministry of Science and ICT, BUET, DU, BRAC CRBLP, BSTI, BCS, BASIS, AMTOB, BDOSN, ISPAB, …) |
| Copyright/IP notice | আলাদা copyright page পাওয়া যায়নি অর্জিত স্ক্যানে; BSTI Act 2018 §15/18 অনুযায়ী standard text BSTI-র IP — পূর্ণ টেক্সট repository-তে কমিট করা হয়নি |
| Amendment/corrigendum page | অর্জিত কপিতে নেই; Catalogue 2023-তেও কোনো amendment তালিকাভুক্ত নয় |
| Revision-label discrepancy | BCC label "BDS 1520:2018" vs document "(Second Revision)"/"BDS 1520:2011" vs Catalogue 2023 "Third Revision" — **৩-উপায় অসঙ্গতি নথিভুক্ত** |

## 2. Clause list (verified, obtained edition)

| Clause | Page (printed) | Title |
|---|---|---|
| 0 (0.1–0.4) | 3 | FOREWORD |
| 1 (1.1–1.3) | 4 | SCOPE |
| 2 (2.1, 2.2, 2.2.1–2.2.3) | 4 | CONFORMANCE |
| 3 | 4 | IMPLEMENTATION |
| 4 (4.1–4.18, 4.18.1–4.18.1.5) | 4–6 | TERMINOLOGY |
| 5 | 6 | STANDARD CODE TABLE |
| 6 (6.1–6.4) | 6–7 | EXPLANATORY NOTES |
| Table 1 | 7 | Basic Code Table |
| Table 2 | 8–10 | Graphic Character Allocations |

## 3. Clause-by-clause verification

Status vocabulary: PASS / FAIL / BLOCKED / N/A। প্রতিটি PASS-এর
evidence নিচের §4-এ। Edition scope: **obtained 2011 Second Revision**।

| # | Clause | Page | Requirement (official) | Type | Applicability | Implementation | Expected | Test | Actual | Status |
|---|---|---|---|---|---|---|---|---|---|---|
| C1 | 1.1 | 4 | Bangla version of character code included in Unicode 6.0.0 + their coded representations | Normative | Applicable | `docs/standards/compliance/BDS_1520_OBTAINED_EDITION_ALLOCATION.csv` | Table 2 allocation == Unicode-6.0.0-era assigned set + coded positions | corpus script + unicodedata | **Exact match**: 92 block chars = modern 96 − 4 post-6.0.0 additions (U+0980 Unicode 11.0; U+09FC/FD/FE Unicode 14.0) | **PASS (obtained ed.)** |
| C2 | 1.2 | 4 | Codes intended for interchange among data processing systems incl. recording on media | Normative | Applicable | engine commit pipeline (UTF-8) | Interchange-safe coded output | 579-row harness; unicode_rows | All commits interchange-safe UTF-8; byte-exact | **PASS (obtained ed.)** |
| C3 | 1.3 | 4 | Applicable to alphabets of the Bangla script | Normative | Applicable | whole-repertoire verification | Entire Bangla script repertoire handled | corpus 94/94 + block sweep 128/128 | PASS | **PASS (obtained ed.)** |
| C4 | 2.1 | 4 | CC-data-element conforms if all coded representations of characters within it conform | Normative | Applicable | commit pipeline | Every emitted coded representation conformant | harness expected==actual 579/579 + corpus round-trip | PASS | **PASS (obtained ed.)** |
| C5 | 2.2 + 2.2.1 | 4 | Device conforms if 2.2.1 AND either/both of 2.2.2, 2.2.3; device description identifies how user supplies characters; **conformance shall identify the version adopted** | Normative | Applicable | layout docs + inventory; conformance report | Description exists; adopted version identified | docs review | Descriptions exist (7 layout docs + key inventory); version identification present: "obtained 2011 Second Revision edition; 2018 Third Revision pending" | **PASS (obtained ed.)** |
| C6 | 2.2.2 | 4 | Originating device shall allow user to supply **any sequence** of characters from the version adopted and transmit their coded representations | Normative | Applicable | 7 layouts | Direct key origination of the whole repertoire | harness key coverage audit (66 distinct Bangla-script points emitted) | **PARTIAL GAP**: layouts originate a documented subset; no key path for e.g. currency numerators U+09F4..U+09F9, ISSHAR, Ganda Mark; the engine pipeline itself transmits any sequence (round-trip proven). Device conformance under 2.2 is still satisfied via 2.2.3 (see C7) | **FAIL (obtained ed.) — gap recorded**; fixing needs layout-mapping decisions outside this phase's authorized scope |
| C7 | 2.2.3 | 4 | Receiving device shall be capable of receiving and interpreting any coded representations and making characters available identifiably | Normative | Applicable | engine input path (UTF-8 validation) | Any valid coded representation accepted, characters identifiable | block sweep 128/128 + corpus 94/94 + invalid-UTF8 fuzz corpus | PASS | **PASS (obtained ed.)** |
| C8 | 3 | 4 | Implementation in physical media not covered by the standard | Informative | N/A | n/a | n/a | n/a | Out of scope for software IME | **N/A** |
| C9 | 4 | 4–6 | Definitions (terminology) | Informative | N/A | n/a | n/a | n/a | Definitional | **N/A** |
| C10 | 5 | 6 | Standard 8-bit code table: 16 columns × 16 rows = 256 positions; columns 8..15 contain **90** graphic characters | Normative | Applicable | corpus CSV | Structure verified; count statement | corpus script | Structure verified (256 positions; graphics in cols 8..15). **DISCREPANCY RECORDED: clause 5 states 90, Table 2 prints 94 rows** — internal inconsistency of the obtained edition, documented, never silently reconciled | **PASS (structure) + recorded discrepancy** |
| C11 | 6.1 | 6 | Bit identification b8..b1; example: digit one at 1110 0111 = E/7 | Normative | Applicable | corpus offset rule | E/7 holds digit one | corpus row E7 → U+09E7 ১ | Exact match | **PASS (obtained ed.)** |
| C12 | 6.2 | 6 | Code table notation x/y; one-to-one correspondence with bit combinations | Normative | Applicable | corpus CSV position column | All positions distinct x/y | corpus script (94 distinct positions) | PASS | **PASS (obtained ed.)** |
| C13 | 6.3 | 6 | At least one name assigned to each graphic character | Normative | Applicable | corpus CSV name column | Names present per character | corpus script | All 94 names present; 6 printed names paraphrase Unicode names (documented per row) | **PASS (obtained ed.)** |
| C14 | 6.4 | 7 | Uniqueness: a character allocated to a position may not be placed elsewhere; graphic characters specified in Table 2 | Normative | Applicable | corpus script uniqueness check | One position per character, no duplicates | corpus script (94 distinct hex codes; each scalar once) | PASS | **PASS (obtained ed.)** |
| C15 | Table 1 | 7 | Basic code table (visual arrangement) | Normative | Applicable | corpus + page images | Grid consistent with Table 2 | 240-DPI page image verification: structure + row/col bit headers + anchors (E/7 = digit one; row 6 = আ খ দ শ / ০) | Structure + anchors verified; full 256-cell glyph-by-glyph scan verification not performed (scan legibility); Table 2 is the binding allocation (per 6.4) | **PASS (anchors; Table 2 binding)** |
| C16 | Table 2 | 8–10 | Graphic Character Allocations — the binding repertoire | Normative | Applicable | `BDS_1520_OBTAINED_EDITION_ALLOCATION.csv` (94 rows) | Every allocation transcribed + machine-verified | corpus script + 5 unit tests | 94/94 rows transcribed; offset rule row-by-row; 2 danda exceptions; subset property vs harness PASS | **PASS (obtained ed.)** |

## 4. Evidence chain

| Evidence | ID/Location |
|---|---|
| Official PDF (SHA256, provenance) | SRC-20; PDF kept outside repository |
| Official BSTI Catalogue 2023 (revision/page/price-group record) | SRC-19 (1520: Third Revision, 10 pages, Gr-B) |
| Allocation corpus (94 rows, machine-checked) | `docs/standards/compliance/BDS_1520_OBTAINED_EDITION_ALLOCATION.csv` (EV-C-01) |
| Corpus builder/verifier script | `scripts/layout_verify/bds1520_official_corpus.py` (EV-C-02) |
| Corpus unit tests (5) | `tests/unit/bds1520_official_corpus_test.cpp` + `bds1520_corpus_data.inc` (EV-C-03) — suite 591/591 |
| Harness subset property | 579-row harness: 66 distinct Bangla-script code points, all ∈ official repertoire (EV-C-02 run log) |
| unicodedata crosscheck (Unicode 15.0.0) | block sweep 128/128 + 0 mismatches (EV-U-01/02) |

## 5. Findings (recorded honestly)

1. **Edition discrepancy**: BCC label "2018" vs document "Second Revision /
   2011" vs Catalogue "Third Revision" — অর্জিত কপি পুরোনো সংস্করণ; 2018
   clause binding BLOCKED।
2. **Internal count discrepancy**: clause 5 "90 graphic characters" vs
   Table 2-এর 94 rows — standard-এর ভিতরেই অসঙ্গতি; Table 2 (per clause 6.4)
   binding ধরা হয়েছে।
3. **Scope-basis note**: এমনকি 2018 Third Revision-এর catalogue scope-ও
   "Unicode version 6.0.0" বলে — অর্থাৎ বর্তমান সংস্করণের নিজস্ব ঘোষণাই
   Unicode-6.0.0-যুগের repertoire-কে নির্দেশ করে; তাই অর্জিত সংস্করণের
   allocation facts বর্তমান সংস্করণের scope-এর সঙ্গে সাংঘর্ষিক হওয়ার কোনো
   প্রমাণ নেই — তবে 2018 টেক্সট না পড়ে এটা *conclusion* হিসেবে দাবি করা হয় না।
4. **Post-6.0.0 Unicode additions** (U+0980 ANJI — Unicode 11.0; U+09FC/FD/FE —
   Unicode 14.0) অর্জিত টেবিলে নেই — standard-এর নিজস্ব scope অনুযায়ী
   সামঞ্জস্যপূর্ণ; engine এগুলো সঠিকভাবে encode করে (block sweep), কোনো
   layout এগুলো emit করে না।
5. **Clause 2.2.2 gap**: পুরো repertoire-র direct key origination নেই —
   বিকল্প 2.2.3 receiving route-এ device conformance ঠিক আছে; gap নথিভুক্ত।
