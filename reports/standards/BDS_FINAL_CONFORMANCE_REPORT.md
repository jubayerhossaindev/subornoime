# BDS চূড়ান্ত কনফরম্যান্স রিপোর্ট — SubornoIME

**মিশন**: COMPLETE BDS COMPLIANCE, BANGLADESH NATIONAL STANDARD CONFORMANCE &
OFFICIAL BSTI CERTIFICATION MISSION — **PHASE 2 UPDATED** (BDS FINALIZATION
PHASE 2: Official Standard Acquisition, Clause Verification & Windows Runtime
Completion)
**তারিখ**: ২০২৬-১১-২৪ (Asia/Dhaka)
**Baseline**: branch `main`, HEAD `cce3413` → মিশন commit `533b3e2` →
PHASE 2 commit (এই রিপোর্টের কমিট)
**গবেষণার সর্বশেষ অবস্থা**: সব official source ২০২৬-১১-২৪ তারিখে access করা
(`docs/standards/research/BDS_OFFICIAL_SOURCE_REGISTER.md` — SRC-19..24 নতুন)

> এই রিপোর্ট কোনো certificate নয়। BSTI-issued certificate ছাড়া এখানে
> সর্বোচ্চ সম্ভাব্য verdict-ই ব্যবহার করা হয়েছে। কোথাও
> `BSTI CERTIFIED`, `BSTI APPROVED`, certificate number বা Standard Mark-এর
> দাবি **নেই** — এবং থাকতে পারে না।
>
> **PHASE 2 সীমানা**: BDS 1520-এর official full text অর্জিত ও পড়া হয়েছে —
> তবে BCC-প্রকাশিত কপিটি **২০১১ Second Revision** সংস্করণ (cover/header
> যাচাইকৃত); বর্তমান **2018 Third Revision**-এর full text এখনও store-only —
> তাই বর্তমান সংস্করণের clause binding **BLOCKED-ই থাকে**। কোনো status
> জোর করে GREEN করা হয়নি।
>
> **PHASE 3 হালনাগাদ (২০২৬-১১-২৪, এই রিপোর্টের কমিট)**: National layout-কে
> phonetic prototype থেকে **পূর্ণ positional Jatiyo layout**-এ rewrite করা
> হয়েছে — official **National Keyboard of Bangladesh (BDS 1738:2018)**
> diagram (Unicode public document L2/19-079, Figure 4) + legacy reference IME factual
> reference অনুযায়ী। স্বাধীন engine evidence **121/121 row PASS**; unit suite
> **609/609 PASS**; Assamese ৰ/ৱ user directive অনুযায়ী excluded; GPL source
> থেকে শুধু key→character facts নেওয়া হয়েছে (clean-room MIT)। clause-level
> BDS binding **BLOCKED-ই থাকে** (full text কেনা হয়নি); Windows runtime
> **BLOCKED**; certification eligibility **PENDING**। বিস্তারিত:
> `BDS_NATIONAL_LAYOUT_DISCREPANCY_REPORT.md`, spec
> `docs/specifications/national-keymap.md`।

> এই রিপোর্ট কোনো certificate নয়। BSTI-issued certificate ছাড়া এখানে
> সর্বোচ্চ সম্ভাব্য verdict-ই ব্যবহার করা হয়েছে। কোথাও
> `BSTI CERTIFIED`, `BSTI APPROVED`, certificate number বা Standard Mark-এর
> দাবি **নেই** — এবং থাকতে পারে না।

---

## পার্ট A — Standard-প্রতি পূর্ণ ব্যাখ্যা (PART 23-এর ১৯টি প্রশ্ন অনুসারে)

### A.1 — BDS 1738:2018 (Computer Bangla Keyboard, First Revision)

1. **Standard-টি কী নিয়ে?** কম্পিউটারের জন্য জাতীয় বাংলা কীবোর্ড লেআউট —
   অক্ষর বিন্যাস, physical key position, modifier layer, বাংলা বর্ণ/সংখ্যা/
   চিহ্ন mapping।
2. **কেন প্রযোজ্য?** SubornoIME-এর `National` layout জাতীয় লেআউটের
   conformance target; বাকি ছয় layout বিকল্প কীবোর্ড হিসেবে নিজেদের পরিচয়ে
   থাকবে (জাতীয় mapping জোর করে ঢোকানো নিষিদ্ধ — mission rule)।
3. **Official document কোথা থেকে?** BSTI BDS Store
   (https://ec.eservice.bsti.gov.bd/) — **তবে full authorized copy আজ
   পাওয়া যায়নি** (account+purchase লাগে; brief-এর bstibds.com লিংক দুটি
   404)। BCC-র IT Standards তালিকায় standard-টির নাম-নম্বর নিশ্চিত (SRC-06/07)।
4. **কোন revision?** First Revision, 2018 (BDS 1738:2004 প্রতিস্থাপনকারী —
   foreword extract; full document-এ re-confirm করা বাকি)। Amendment/
   corrigendum কোনো official source-এ পাওয়া যায়নি।
5. **কোন page/clause?** **পড়া হয়নি — সব clause `NOT READ (BLOCKED)`।**
   কোনো clause নম্বর ভান করা হয়নি।
6. **কী requirement ছিল?** Public scope অনুযায়ী: সঠিক বর্ণ-বিন্যাস, key
   position, modifier behavior, সংখ্যা/চিহ্ন mapping। এগুলো mission-এর
   নিজস্ব verification program-এ ([MISSION-PROGRAM] tag) রূপ দেওয়া হয়েছে।
7. **আগে কী সমস্যা ছিল?** জাতীয় layout-এর mapping-এর clause-level তুলনা
   অসম্ভব ছিল (document নেই); আগের mission-এ কেবল non-interference নীতিতে
   কাজ হয়েছিল।
8. **কীভাবে reproduce?** 579-row engine harness + `BdsConformanceTest.*`
   unit tests (`scripts/layout_verify/`, `tests/unit/bds_conformance_test.cpp`)।
9. **Root cause** (ব্যবহারিক gap): official mapping table-এর অভাব —
   GAP-01/GAP-04 (`BDS_GAP_ANALYSIS.md`)।
10. **কী solution?** (ক) সব layout-এর পরিচয় অক্ষুণ্ণ রাখা; (খ) engine-level
    mapping/সংখ্যা/চিহ্ন প্রমাণ সম্পন্ন করা (সব PASS); (গ) official copy ক্রয়ের
    পথ নথিভুক্ত করা; (ঘ) IP clearance ছাড়া কোনো conformance দাবি না করা।
11. **কোন source file পরিবর্তন?** এই mission-এ **কোনো layout/engine source
    পরিবর্তন হয়নি**; শুধু নতুন test/tooling/doc যোগ হয়েছে
    (`tests/unit/bds_conformance_test.cpp`, `tests/CMakeLists.txt`,
    `scripts/layout_verify/bds_*`)।
12. **কোন নতুন test?** `BdsConformanceTest`-এর ১০টি test (digit row
    U+09E6..U+09EF, Khipro fix preservation, Probhat fix preservation,
    NFC কনফরম্যান্স, full-block sweep, reserved-point safety)।
13. **কোন real test চালানো হয়েছে?** unit 586/586, integration 2/2, ASan/
    UBSan 588 clean, 579-row harness 0 FAIL — সবগুলো আসল run (মিথ্যা কোনো
    ফলাফল নেই)।
14. **Expected result**: প্রতিটি token/key-এর জন্য নথিভুক্ত scalar sequence।
15. **Actual result**: প্রতিটি ক্ষেত্রে expected == actual
    (`evidence-raw/bds_engine_rows_579.csv`, `bds_repertoire_*`)।
16. **Evidence কোথায়?** `reports/standards/evidence-raw/` + EV index
    (`BDS_EVIDENCE_INDEX.md`)।
17. **কী সমস্যা বাকি?** clause-level mapping তুলনা (GAP-04), Windows
    physical keyboard test (GAP-02)।
18. **কেন BLOCKED/N/A?** official full text + Windows runtime নেই — কোনোটিই
    অনুমান করা হয়নি।
19. **পরবর্তী official step?** e-store account → Buy BDS (Sonali Payment
    Gateway) → clean-room transcription → key-per-cell corpus → প্রয়োজনে
    minimal national-layout alignment + IP clearance।

### A.2 — BDS 1520:2018 (Bangla Coded Character Set)

1. **কী নিয়ে?** বাংলা coded character set — repertoire, allocation,
   interchange।
2. **কেন প্রযোজ্য?** IME-এর প্রতিটি output character এই repertoire-এরই সদস্য
   হতে হবে ও সঠিকভাবে encode হতে হবে।
3. **Document কোথা থেকে?** BSTI BDS Store — full copy **আজও অপ্রাপ্ত**
   (SRC-02/05/18); BCC তালিকায় নাম-নম্বর নিশ্চিত; ১৯৯৫ সালে প্রথম প্রণয়নের
   official snippet (SRC-17)।
4. **Revision?** 2018 (তৃতীয় পরিমার্জন ধারা — BCC/BSTI রেকর্ড; full text-এ
   re-confirm বাকি)।
5. **Page/clause?** **পড়া হয়নি — clause `NOT READ (BLOCKED)`।**
6. **Requirement?** [MISSION-PROGRAM] স্তরে: প্রতিটি repertoire character
   সঠিক scalar, সঠিক UTF-8/UTF-16, সঠিক sequence, সঠিক normalization,
   কোনো loss/dup/reorder নেই।
7. **আগে কী সমস্যা?** আগের mission-এ দুটি নির্দিষ্ট character (virama,
   nukta) যাচাই হয়েছিল; পূর্ণ ব্লক-sweep ছিল না।
8. **Reproduce?** `bds_repertoire_verify` (part A/B/C) +
   `bds_repertoire_crosscheck.py`।
9. **Root cause** (গ্যাপ): BDS code table-এর অভাব → clause বাইন্ডিং BLOCKED।
10. **Solution?** পূর্ণ ব্লক (128 code point) engine দিয়ে sweep + স্বাধীন
    `unicodedata` crosscheck — **0 mismatch**; official NormalizationTest.txt
    (UAX #15)-এর বাংলা লাইনে engine NFC **7/7 PASS**; ডিজিট/মুদ্রা/যতিচিহ্ন
    প্রমাণ।
11. **File পরিবর্তন?** নতুন harness/test যোগ; engine অপরিবর্তিত।
12. **নতুন test?** `BengaliBlockUtf8RoundTripSweep`,
    `EngineNfcMatchesOfficialBengaliNormalizationData`,
    `NationalLayoutBengaliDigitRowProducesU09E6ToU09EF` প্রভৃতি।
13. **Real test?** 128/128, 7/7, 12/12, 579/579 — সব executed।
14–16. **Expected/Actual/Evidence**: উপরে; CSV গুলো `evidence-raw/`-এ।
17. **বাকি?** BDS-নির্দিষ্ট code position তুলনা (document নেই); Windows
    file save/reopen পরীক্ষা (`NOT TESTED`)।
18. **কেন BLOCKED?** authorized document + Windows runtime অপ্রাপ্যতা।
19. **পরবর্তী step?** copy ক্রয় → ছক তুলনা → matrix status upgrade।

### A.3 — BDS 1935:2018 (BSCII)

1. **কী নিয়ে?** বাংলা information interchange code (legacy-যুগের interchange
   code মডেল — ক্যাটালগ scope বাক্য, সেকেন্ডারি সূত্র SRC-16)।
2. **কেন প্রযোজ্যতা অডিট?** Unicode IME-এর default pipeline-এ legacy 8-bit
   code ঢোকানো Unicode সঠিকতা ভাঙে; তাই প্রযোজনার সীমানা নির্ধারণই মূল কাজ।
3. **Document?** **অপ্রাপ্ত** (scope বাক্যের একাংশ ছাড়া কিছু নেই)।
4. **Revision?** 2018 (নিশ্চিত নয় — UNVERIFIED)।
5. **Page/clause?** **পড়া হয়নি।**
6. **Requirement?** [MISSION-PROGRAM]: default output pure Unicode থাকবে;
   BSCII কখনো দরকার হলে সেটি হবে আলাদা, explicit conversion module।
7. **আগে সমস্যা?** ছিল না — legacy path কোনোদিন ছিলই না।
8. **Reproduce?** block sweep + harness + invalid-UTF8 fuzz corpus।
9. **Root cause** (গ্যাপ): code table অপ্রাপ্য → conversion module ইচ্ছাকৃতভাবে
   তৈরি করা হয়নি (fabrication নিষিদ্ধ)।
10. **Solution?** সীমানা-নথি + ইন্টিগ্রিটি প্রমাণ (579/579 byte-exact,
    invalid-UTF8 safe, sanitizer clean)।
11–13. **File/test/real test**: কোনো engine পরিবর্তন নেই; sweep/harness চালানো
    হয়েছে।
17. **বাকি?** BSCII↔Unicode conversion — **BLOCKED** (table নেই)।
18. **কেন BLOCKED?** requirement-এর মূল সূত্রই পড়া হয়নি।
19. **পরবর্তী step?** document ক্রয় → scope নির্ধারণ → প্রয়োজন হলে আলাদা
    module design (প্রক্রিয়া `BDS_BSCII_INTEROPERABILITY.md` §4-এ নথিভুক্ত)।

### A.4 — BDS 1834:2011 (Bangla Keypad for Mobile Phone)

1. **কী নিয়ে?** মোবাইল ফোনের বাংলা keypad।
2. **কেন N/A?** SubornoIME Windows ডেস্কটপ IME; মোবাইল keypad পণ্য-scope
   mismatch।
3. **Document?** অপ্রাপ্ত; BCC তালিকায় নাম-নম্বর নিশ্চিত (SRC-07)।
4. **কী সতর্কতা?** "তালিকায় না থাকা = exempt" যুক্তি চালু হয়নি — scope
   mismatch-ভিত্তিক N/A + BSTI-র লিখিত স্বীকৃতির জন্য প্রশ্ন কিউয়ে আছে
   (inquiry Q10)।
5. **পরবর্তী step?** BSTI লিখিত মতামত → master matrix-এ N/A চূড়ান্তায়ন।

---

## পার্ট B — Citation format (প্রতিটি দাবির সূত্র)

এই রিপোর্টের প্রতিটি claim নিচের সূত্র-শৃঙ্খল অনুসরণ করে:

- `BSTI | BDS 1738:2018 | clause NOT READ | full copy not obtained |
  https://ec.eservice.bsti.gov.bd/ | Accessed 2026-11-24 | EV-S-02/04`
- `BCC | IT Standards list (BDS 1738/1520/1834/1935 items) |
  http://bcc.gov.bd/pages/static-pages/688a355cf1fb5aa36c85a107 |
  Accessed 2026-11-24 | page body empty; index snapshot | EV-S-06/07`
- `Ministry of Law | BSTI Act 2018 | Section 15/16/18/21 | authentic English
  text, S.R.O 322-Law/2020 | bdlaws.minlaw.gov.bd | Accessed 2026-11-24 |
  EV-A-02..05`
- `Unicode Consortium | The Unicode Standard, Bengali chart (current;
  18.0.0 identical) | unicode.org/charts/PDF/U0980.pdf |
  Accessed 2026-11-24 | EV-U-05`
- `Unicode Consortium | UAX #15 + NormalizationTest.txt |
  unicode.org/reports/tr15/ | Accessed 2026-11-24 | EV-U-03`

Clause number কোথাও অনুমান করা হয়নি — যেখানে document পড়া হয়নি, সেখানে
`NOT READ (BLOCKED)` লেখা আছে।

---

## পার্ট C — চূড়ান্ত summary table (PHASE 2 হালনাগাদ)

| Standard | Current revision | Applicable clauses | PASS | FAIL | BLOCKED | N/A | Verdict |
|---|---|---|---:|---:|---:|---:|---|
| BDS 1738:2018 | 2018 First Revision (**identity + no-amendment verified** — Catalogue 2023 + BCC re-check; full text store-only) | matrix rows 15 | 12 | 0 | 3 | 0 | **PARTIALLY VERIFIED — clause level BLOCKED** |
| BDS 1520:2018 | 2018 Third Revision (catalogue-verified); **obtained edition = 2011 Second Revision** (BCC full text read) | matrix rows 17 | 15 | 0 | 1 | 0 | **PARTIALLY VERIFIED — obtained-edition clause level VERIFIED; current-edition binding BLOCKED; clause 2.2.2 gap recorded** |
| BDS 1935:2018 | 2018 (catalogue-verified; full text store-only) | matrix rows 8 | 4 | 0 | 4 | 0 | **APPLICABILITY AUDITED — clause level BLOCKED** |
| BDS 1834:2011 | 2011 (**full text obtained and read**) | 1 (applicability) | 0 | 0 | 0 | 1 | **N/A (grounded in official scope clauses; confirmation pending)** |

মোট: 41 rows — **31 PASS, 0 FAIL, 8 BLOCKED, 1 N/A, 1 NOT TESTED**
(`docs/standards/compliance/BDS_MASTER_COMPLIANCE_MATRIX.csv`; PHASE 1 ছিল
28 PASS / 11 BLOCKED)। Standard-wise reconciliation: 12+15+4 = 31 PASS;
3+1+4 = 8 BLOCKED; BDS 1520-এর 17-তম row = CV-17 (Windows file save/reopen,
NOT TESTED)।

### C.2 — PHASE 2 clause verification (obtained edition)

BDS 1520-এর অর্জিত সংস্করণের **সব clause (0..6 + Table 1 + Table 2)** পড়া ও
পরীক্ষা করা হয়েছে — বিস্তারিত:
`docs/standards/compliance/BDS_1520_OBTAINED_EDITION_CLAUSE_VERIFICATION.md`

সারসংক্ষেপ: 16 verification items — **12 PASS, 1 FAIL (clause 2.2.2 direct-
origination gap — device conformance clause 2.2-এর অধীনে 2.2.3 route-এ
সন্তুষ্ট), 3 N/A (informative clauses), 1 PASS-with-recorded-discrepancy
(clause 5: "90" বনাম Table 2-এর 94 rows)**। নতুন evidence: 94-row official
allocation corpus (`BDS_1520_OBTAINED_EDITION_ALLOCATION.csv`) + 5টি নতুন
unit test (`Bds1520OfficialCorpusTest.*`) + corpus verifier script।

মূল ফলাফল:
- অর্জিত টেবিলের 94 allocation row-এর প্রতিটি engine-এ সঠিকভাবে encode,
  NFC-stable, UTF-16 single-unit — **94/94 PASS**।
- Offset rule (8-bit NN ↔ U+09NN) row-by-row যাচাইকৃত; দুটি নথিভুক্ত
  exception (Daari E4/E5 → U+0964/U+0965)।
- Table 2 allocation == Unicode-6.0.0-era assigned set (92 chars) —
  modern block-এর 4টি post-6.0.0 addition (U+0980, U+09FC/FD/FE)
  standard-এর নিজস্ব scope অনুযায়ী অনুপস্থিত — সামঞ্জস্যপূর্ণ।
- 579-row harness-এর সব Bangla-script output (66 distinct code points)
  official repertoire-র ভিতরে — subset property PASS।
- CV-16-র প্রশ্নের উত্তর: official Table 2 **F0/F1 (ৰ U+09F0, ৱ U+09F1)**
  allocate করে; SubornoIME layout গুলো এগুলো emit করে না (policy), engine
  সঠিকভাবে encode করে।

## পার্ট D — আলাদা verdict (PART 1.1 ও PART 19 অনুযায়ী)

**A. Technical BDS Conformance:**
`PARTIALLY CONFORMANT — INDEPENDENT BSTI ASSESSMENT PENDING`
(কারণ: clause-level rows BLOCKED; engine/Unicode-স্তরের সব যাচাই PASS;
full standard copy ছাড়া সম্পূর্ণ conformant দাবি করা মিথ্যা হতো)

**B. BSTI Certification Eligibility:**
`PENDING OFFICIAL RESPONSE`
(আইন text বিশ্লেষণ সম্পন্ন — software CM route-এর প্রমাণ নেই; formal inquiry
draft BN+EN প্রস্তুত, পাঠানো হয়নি)

**C. Official Certification:**
`NOT CERTIFIED — APPLICATION NOT SUBMITTED`
(কোনো certificate, number বা mark-এর দাবি নেই; কোনো ভুয়া claim করা হয়নি)

**D. Production Readiness:**
`CONDITIONAL GO`
(engine-level সব সবুজ: 586/586 + 2/2 + ASan/UBSan clean + 579-row 0 FAIL +
কোনো layout ভাঙেনি + Khipro/Probhat fix সংরক্ষিত; শর্ত: (১) Windows 10 x64
runtime যাচাই W-01..W-10, (২) BDS copy ক্রয়ের পর clause-level যাচাই)

---

## পার্ট E — পরবর্তী করণীয় (ক্রম অনুসারে — PHASE 2 হালনাগাদ)

1. ব্যবহারকারীর অনুমোদনে **BDS 1738:2018, BDS 1935:2018 এবং BDS 1520:2018
   Third Revision full copy ক্রয়** (https://ec.eservice.bsti.gov.bd —
   Sign Up → Buy BDS → Sonali Payment Gateway; প্রতিটি একবারই download হয় —
   সাথে সাথে সংরক্ষণ করতে হবে; catalogue price-group অনুযায়ী প্রতিটি
   ≈BDT 500–700; বর্তমানে কোনো ক্রয় করা হয়নি — approval নেই)।
2. **BSTI/BCC inquiry পাঠানো** (PHASE 2-হালনাগাদ draft:
   `BSTI_OFFICIAL_INQUIRY_BN.md` / `EN.md`) — ব্যবহারকারীর অনুমতি ছাড়া
   পাঠানো হয়নি।
3. Windows 10 x64 মেশিনে **W-01..W-10** যাচাই (পদ্ধতি নথিভুক্ত)।
4. Official table পড়ে **clean-room corpus** → National layout clause-তুলনা
   → প্রয়োজনে minimal alignment (শুধু National; IP clearance-এর পরে);
   1520-এর ক্ষেত্রে অর্জিত corpus-এর সঙ্গে Third Revision টেবিল diff-compare।
5. BSTI-র লিখিত উত্তর অনুযায়ী certification route চূড়ান্ত করা।
