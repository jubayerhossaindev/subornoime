# BDS Official Source Register

**Mission**: COMPLETE BDS COMPLIANCE, BANGLADESH NATIONAL STANDARD CONFORMANCE &
OFFICIAL BSTI CERTIFICATION MISSION — **PHASE 2 UPDATED** (BDS FINALIZATION
PHASE 2, 2026-11-24)
**Research dates**: 2026-11-24 (phase 1 + phase 2, Asia/Dhaka)
**Repository state at phase-2 research time**: branch `main`, HEAD `0fd4874`,
working tree clean.

Rule applied: no standard was inferred from titles, snippets or other
software's implementation. Every entry below records what was actually
retrieved, from where, and its classification. Where only a search-index
snippet of an official page was observable, it is marked as such and is used
ONLY for standard identification — never as a requirement source.

**PHASE 2 change summary**: BCC IT-standards page body now RENDERS (SRC-22
supersedes the phase-1 SRC-06/07 empty-body finding); official full-text PDFs
of BDS 1520 (obtained edition = 2011 Second Revision) and BDS 1834:2011 were
downloaded from BCC's official file store (SRC-20/21) — no purchase, payment
or account action involved; official BSTI Catalogue 2023 (SRC-19) verifies
titles/revisions/page counts/price groups; bstibds.com is repurposed (SRC-23);
e-store API confirmed auth-gated (SRC-24).

## 1. Register

| ID | Source | URL | What was retrieved | Access date | Classification |
|---|---|---|---|---|---|
| SRC-01 | BSTI official website (home) | https://bsti.gov.bd/ | Institutional structure: মান উইং (Standards Wing), CM উইং (Certification Marks Wing), Metrology Wing, Management System Certification Wing; schemes: বাধ্যতামূলক সার্টিফিকেশন মার্কস (Mandatory CM), স্বেচ্ছামূলক সার্টিফিকেশন মার্কস (Voluntary CM), Management System Certification, Halal Certification; navigation to Standards Catalogue (id=57), CM process maps (id=47, 48), license grant/renewal steps (id=53), mandatory products list (id=49) | 2026-11-24 | Official, live, retrieved |
| SRC-02 | BSTI BDS e-store | https://ec.eservice.bsti.gov.bd/ | Live store: standards categories (Chemical; Agriculture and Food; Jute and Textile; Electrical and Electronics; Civil and Mechanical Engineering; Halal Standards); "Explore and purchase officially approved Bangladesh Standards from BSTI"; "For purchasing Non-Adopted product for ISO or IEC standard, please contact with One Stop Service"; Sign In/Sign Up required | 2026-11-24 | Official, live, retrieved |
| SRC-03 | BSTI BDS e-store purchase procedure | https://ec.eservice.bsti.gov.bd/How-To-Buy | Verbatim purchase route: search/BDs section → "Buy BDS" → document preview and scope review → Sonali Payment Gateway → download (each purchased BDS downloadable only once). Stored via the BSTI Maan Wing e-commerce platform, replacing in-person OSS purchase | 2026-11-24 | Official, live, retrieved verbatim |
| SRC-04 | BSTI BDS store record — BDS 1738:2018 (link supplied by mission brief) | https://bstibds.com/bookre/book_rewrite/f9c6a76f… | **HTTP 404 Not Found** — record link no longer resolves | 2026-11-24 | Official link, DEAD on access date |
| SRC-05 | BSTI BDS store record — BDS 1520:2018 (link supplied by mission brief) | https://bstibds.com/bookre/book_rewrite/8ffd7500… | **HTTP 404 Not Found** — record link no longer resolves | 2026-11-24 | Official link, DEAD on access date |
| SRC-06 | BCC main site — "Information Technology (It) Related Bangladesh Standards" | http://bcc.gov.bd/pages/static-pages/688a355cf1fb5aa36c85a107 | Page heading present (page last content-updated 30 Jan 2023); **standards list body renders empty** in served HTML (verified with headless browser: full DOM text contains no BDS number). Search-indexed snapshot of the same URL shows: "BSTI has adopted 23 IT related ISO/IEC standards as Bangladesh Standards" and item "27, Bangladesh Standard Code for Information Interchange (BSCII), BDS 1935:2018" | 2026-11-24 | Official page live but content body NOT retrievable; identification evidence from index snapshot only |
| SRC-07 | BCC regional sites — same standards list (Khulna / Sylhet variants) | http://khulna.bcc.gov.bd/pages/static-pages/6922de28933eb65569e18a45 ; https://bcc.sylhet.gov.bd/pages/static-pages/696cbb57c4774958d7baad09 | Index snapshots show the numbered list items: "24, National Keyboard (Bengali) Layout, BDS 1738:2018"; "25, Specification for Bangla Coded Character Set for Information Interchange, BDS 1520:2018"; "26, Bangla Keypad for Mobile Phone, BDS 1834:2011"; "27, Bangladesh Standard Code for Information Interchange (BSCII), BDS 1935:2018"; closing note "If you need any help in adopting/using these Standards Contact to BSTI". Live pages currently render the same empty body as SRC-06 | 2026-11-24 | Official page body NOT retrievable; identification from index snapshot only |
| SRC-08 | Bangladesh Standards and Testing Institution Act, 2018 — authentic English text | https://bdlaws.minlaw.gov.bd/upload/act/2022-01-31-10-51-20-58.The-Bangladesh-Standards-and-Testing-Institution-Act%2C-2018.pdf | Full PDF (470,972 bytes) downloaded and text-extracted. Act No. LXVII of 2018; authentic English text published under section 52 by S.R.O No. 322-Law/2020, dated 26 November 2020. Sections 6, 15, 16, 18, 21 captured verbatim (see EV-A-01..05) | 2026-11-24 | Official law text, full copy obtained |
| SRC-09 | BSTI mandatory products page | https://bsti.gov.bd/pages/static-pages/6922dcba933eb65569e11af8 | Heading "বিএসটিআই'র বাধ্যতামূলক পণ্যের তালিকা" present (content updated 8 Nov 2026); **list body renders empty** in served page. Index snapshot of the same URL shows "৩১৫ টি বাধ্যতামূলক পণ্যের এস,আর,ও" (S.R.O. for 315 mandatory products). No software/IT item observable in any retrievable portion | 2026-11-24 | Official page live but list NOT retrievable; pending BSTI written clarification |
| SRC-10 | BSTI CM process pages (3 URLs supplied by mission brief) | https://bsti.gov.bd/pages/static-pages/6922dddd933eb65569e16c1a ; …6922de0b933eb65569e17c82 ; …6922de6c933eb65569e1a87f | First page: heading "প্রোডাক্ট সার্টিফিকেশনের আইনগত কার্যক্রম" (Product Certification Legal Process) present; **procedural body renders empty**. Navigation confirms CM process maps and license grant/renewal step pages exist (SRC-01). Detailed steps/fees therefore UNVERIFIED from primary source on this access date | 2026-11-24 | Official pages live, bodies NOT retrievable |
| SRC-11 | Unicode Standard — Bengali chart (current revision) | https://www.unicode.org/charts/PDF/U0980.pdf | Full official Bengali code chart PDF downloaded (122,168 bytes); text-extracted. Confirms U+09CD BENGALI SIGN VIRAMA, U+09BC BENGALI SIGN NUKTA, U+0995 KA, and the canonical decompositions ড় ≡ 09A1 09BC, ঢ় ≡ 09A2 09BC | 2026-11-24 | Official, full copy obtained |
| SRC-12 | Unicode 18.0.0 — Bengali chart | https://www.unicode.org/Public/18.0.0/charts/PDF/U0980.pdf | Full official chart for Unicode 18.0.0 downloaded (122,168 bytes — byte-identical size to SRC-11, Bengali block unchanged in 18.0) | 2026-11-24 | Official, full copy obtained |
| SRC-13 | Unicode Standard versions page | https://www.unicode.org/versions/ | Current version **Unicode 18.0.0** (2026). Citation format confirmed | 2026-11-24 | Official, live, retrieved |
| SRC-14 | UAX #15 — Unicode Normalization Forms | https://www.unicode.org/reports/tr15/ | Normative conformance clauses UAX15-C1..C5 retrieved verbatim; NFC/NFD/NFKC/NFKD definitions; canonical vs compatibility equivalence; composition exclusions and singletons | 2026-11-24 | Official, live, retrieved |
| SRC-15 | NormalizationTest.txt (official UAX #15 conformance data file) | https://www.unicode.org/Public/UCD/latest/ucd/NormalizationTest.txt | Full file (20,232 lines) downloaded; 7 Bengali-block conformance lines extracted (U+09CB, U+09CC, U+09DC, U+09DD, U+09DF + two tilde-overlay sequences) and executed through the engine (EV-U-03) | 2026-11-24 | Official, full copy obtained |
| SRC-16 | BSTI Standards Catalogue 2018 (third-party copy) | https://www.scribd.com/document/437204867/BSTI ; https://rise.esmap.org/…BSTI%20Standards%20Catalogue_2018.pdf | Catalogue descriptions including BDS 1935:2018 scope sentence: "specifies Bangla Character Codes intended for the interchange of information among…" (truncated in index). **Secondary copy, not authoritative** — used only to corroborate standard identity | 2026-11-24 | Secondary (non-authoritative) |
| SRC-17 | BCC standards-development page (Bangla) | https://bcc.gov.bd (standards development page, indexed) | Index snippet: "…এ সমস্যা নিরসনে ১৯৯৫ সনে এ সংক্রান্ত জাতীয় মান BDS 1520 প্রণয়ন করা হয়…" (BDS 1520 originally developed 1995). Corroborates revision history only | 2026-11-24 | Official origin, snippet only |
| SRC-18 | BSTI BDS e-store product-list API | https://ec-api.eservice.bsti.gov.bd/api/v1/product-list?... | Requires session token (`{"message":"Token not provided"}` without sign-in). **Not bypassed** — per mission rule against credential/paywall bypass. Store records for the four BDS standards require a (free) account sign-in | 2026-11-24 | Official API, BLOCKED (auth) |
| SRC-19 | **BSTI Standards Catalogue 2023** (official PDF, "Till June 2023", file generated 2025-04-30, from bsti.gov.bd catalogue page) | https://bsti.gov.bd/pages/static-pages/6922df49933eb65569e210e0 → https://objectstorage.ap-dcc-gazipur-1.oraclecloud15.com/n/axvjbnqprylg/b/V2Ministry/o/office-bsti/2024/12/f94fbe51409d4b23ae61af9662f27c72.pdf | Official catalogue PDF (23,438 text lines) fetched; entries verified for all four standards: BDS 1520:2018 "Bangla coded character set for information interchange (Third Revision)", 10 pages, Gr-B, ET (scope: "included in the Unicode version 6.0.0"); BDS 1738:2018 "Computer Bangla keyboard (First Revision)"; BDS 1834:2011 "Mobile phone bangla keypad" (scope = 3×4 keypad); BDS 1935:2018 "Bangladesh Standard Code for Information Interchange (BSCII)", 13 pages, Gr-C, ET (full scope sentence captured). No amendment/corrigendum listed for any of the four. Price schedule Annexure-A: national BDS Gr-A..K = BDT 500..2000 | 2026-11-24 | Official, full copy obtained (factual entries cited; not redistributed) |
| SRC-20 | **BDS 1520 full-text PDF (obtained edition = 2011 Second Revision)** — BCC official file store, linked as row 25 of the BCC IT-standards page | https://objectstorage.ap-dcc-gazipur-1.oraclecloud15.com/n/axvjbnqprylg/b/V2Ministry/o/office-bcc/2024/12/3ee2ba6e90764b8497b45da752b6879a.pdf | Full scanned document (10 pages) downloaded and read. Cover: "SPECIFICATION FOR BANGLA CODED CHARACTER SET FOR INFORMATION INTERCHANGE (Second Revision)"; headers "BDS 1520:2011"; foreword adoption 15-02-2011; clauses 0..6 + Table 1 + Table 2 (94 allocation rows) transcribed and machine-verified. **The BCC row label says "BDS 1520:2018" but the document itself is the older 2011 Second Revision edition — three-way revision-label discrepancy recorded.** SHA256 ab2bc3c26896257f4447d2a1ff88d93c8ddcf21ce5c68e103fbed9743b0cf456. PDF kept OUTSIDE the repository (copyright); only allocation facts + clause index committed | 2026-11-24 | Official, full copy obtained and read (obtained edition) |
| SRC-21 | **BDS 1834:2011 full-text PDF** — BCC official file store, linked as row 26 of the BCC IT-standards page | https://objectstorage.ap-dcc-gazipur-1.oraclecloud15.com/n/axvjbnqprylg/b/V2Ministry/o/office-bcc/2024/12/e53a9f9bc9c743b192675a832455cf71.pdf | Full scanned document (6 pages) downloaded and read: clause 1.1 ("layouts for input of Bangla characters, signs, symbols and currency marks using industry standard 3 X 4 Mobile Keypad"), clause 2 ("12 keys (3 X 4 Keypad)"). SHA256 0e9f2364edf7ade14480f5d8e233322e74d62ae3458d1cf42a0fb91aba46e942. PDF kept outside the repository | 2026-11-24 | Official, full copy obtained and read |
| SRC-22 | BCC IT-standards page RE-VERIFICATION (body now renders) | http://bcc.gov.bd/pages/static-pages/688a355cf1fb5aa36c85a107 | Full 27-row list now retrievable (content embedded base64 in page HTML): advisory text verbatim — "BSTI has adopted 23 IT related ISO/TEC standards as Bangladesh Standards. Besides these are three IT related Standards which were developed by BCC for Bangladesh Government. All public Institutions and private organizations… strongly advised to follow these standards…"; rows 1–23 = ISO/IEC adoptions (vocabulary, biometric ID, security 27001/27002, software evaluation 14598 series, maintenance 14764, etc.); row 24 BDS 1738:2018 (no link); row 25 BDS 1520:2018 (PDF link = SRC-20); row 26 BDS 1834:2011 (PDF link = SRC-21); row 27 BDS 1935:2018 (no link); footer "If you need any help in adopting/using these Standards Contact to BSTI". Internal inconsistency noted: advisory says 23+3 standards, table lists 23+4 | 2026-11-24 | Official, live, retrieved (supersedes phase-1 SRC-06/07 empty-body finding) |
| SRC-23 | bstibds.com status change | https://bstibds.com/ | Now a Next.js "BSTI License Checking Application" (license/certificate/QR verification); zero BDS-store content; the BSTI service page still hyperlinks bstibds.com as the "e-Catalog & BDS Sale" software — stale link recorded | 2026-11-24 | Official-adjacent, live, repurposed |
| SRC-24 | BSTI e-store API + TLS re-verification (phase 2) | https://ec-api.eservice.bsti.gov.bd/api/v1/product-list → `{"message":"Unauthenticated."}`; store TLS cert CN=admin.eservice.bsti.gov.bd (Let's Encrypt, valid 2026-11-06→2027-02-04) — hostname mismatch noted for strict clients | Product listing requires account; purchase flow (SRC-03) unchanged; store-listed titles/prices remain UNVERIFIED without account | 2026-11-24 | Official API, BLOCKED (auth) — unchanged |

## 2. Access-blocked items and the exact official acquisition path

Full authorized copies of **BDS 1738:2018 and BDS 1935:2018** (and the BDS
1520:2018 Third Revision text) are **still** not obtainable without a purchase
or account sign-in. BDS 1520 (older 2011 edition) and BDS 1834:2011 full texts
are now legitimately available from BCC's official file store (SRC-20/21) —
downloaded 2026-11-24 without any purchase, payment or account action.
Per the mission's own rules no paywall was bypassed and no copyrighted text
was redistributed. The official acquisition path for the remaining store-only
texts (SRC-03, verbatim) is:

1. Visit https://ec.eservice.bsti.gov.bd/ and sign up (free registration).
2. Search the BDS section for the standard number (e.g. "BDS 1738").
3. Select the standard, click **"Buy BDS"**, review the document preview and
   scope.
4. Pay through the **Sonali Payment Gateway**.
5. Download — **each purchased BDS can be downloaded only once**, so it must
   be saved immediately.

Alternative: BSTI One Stop Service (OSS) for non-adopted ISO/IEC-related
purchases. Contact to BSTI for adoption/usage help is the BCC-listed route
(SRC-07).

## 3. Honesty rules enforced in this register

- No clause number anywhere in this repository is inferred; clause-level
  matrices carry `BLOCKED — official document not obtained` where the text
  was not read.
- Search-index snippets are labelled as such and were never used to state a
  requirement — only to identify standards and list status.
- The two bstibds.com record links supplied by the mission brief are recorded
  as dead (404) rather than being silently treated as accessed.
- No copyrighted standard text (beyond the officially free law text and
  short identification references) enters this repository.
