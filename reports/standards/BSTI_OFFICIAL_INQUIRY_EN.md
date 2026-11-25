# Formal Inquiry Draft to BSTI/BCC (English)

> **Usage rule**: this draft is PREPARED but NOT SENT. No email, form
> submission or legal declaration may be made without the user's explicit
> approval (mission rule). Verify the institution's current official contact
> channel before sending.
>
> **PHASE 2 (2026-11-24)**: draft reviewed and corrected; standard titles
> aligned to the official BSTI Catalogue 2023; two new questions (Q12, Q13)
> raised by the phase-2 document acquisition findings; technical status
> updated. STILL NOT SENT.

---

**To:**
The Director General, Bangladesh Standards and Testing Institution (BSTI),
Dhaka.
(Cc: Director General, Bangladesh Computer Council (BCC), Agargaon, Dhaka)

**Subject: Request for Official Guidance on BDS Conformance Assessment and
Certification Eligibility for SubornoIME — Native Windows Bangla IME**

Dear Sir/Madam,

We are developing SubornoIME, a native Windows Bengali Input Method Editor
(C++20, Win32, TSF, COM), distributed as downloadable desktop software under
the MIT license. The product supports seven keyboard layouts and produces
fully Unicode-based output (UTF-8/UTF-16, NFC normalization).

We wish to assess the product's conformance to the following national
standards:
- **BDS 1738:2018** — Computer Bangla Keyboard (First Revision) — title per official BSTI Catalogue 2023
- **BDS 1520:2018** — Specification for Bangla Coded Character Set for Information Interchange (Third Revision)
- **BDS 1935:2018** — Bangladesh Standard Code for Information Interchange (BSCII)
- **BDS 1834:2011** — Bangla Keypad for Mobile Phone

For transparency: we already hold official full-text copies of the BDS 1520
document published on the BCC website (which carries the cover text "(Second
Revision)" and headers "BDS 1520:2011") and of BDS 1834:2011; questions 12
and 13 below arise directly from these documents.

We respectfully request your guidance on the following questions:

1. Does downloadable desktop software of this kind (an IME) fall within
   BSTI's certification-mark framework? What is the status of software/IT
   products under the mandatory products S.R.O. (315 products)?
2. Does BSTI perform conformity assessment or certification for software
   products? If so, under which scheme (mandatory CM / voluntary CM / test
   report / other)?
3. Is the scope of BDS 1738:2018 limited to physical keyboards/devices, or
   does it also cover software-implemented keyboard layouts?
4. Is software conformance assessment possible for BDS 1520:2018 and
   BDS 1935:2018? If so, what form of evidence/test dossier is accepted?
5. Is a voluntary certification route available for such a product, and on
   what terms?
6. What is the official test procedure, and which accredited/recognized
   laboratory or assessment authority applies?
7. Which application documents, forms and fees are required (kindly attach
   the official form numbers and fee schedule)?
8. For software, what is considered equivalent to "regular inspection or
   testing" under section 16(2)(b) of the BSTI Act 2018 — build systems, test
   reports, version audits, or something else?
9. Given the intellectual property rights referenced in the foreword of
   BDS 1738:2018 (Bijoy keyboard), does implementing the official mapping
   table in software require a separate license/permission from the rights
   owner? If so, how may it be obtained?
10. Can BSTI provide a written opinion that BDS 1834:2011 (mobile phone
    keypad) does not apply to a Windows desktop IME?
11. Should we use the BDS e-store account + Sonali Payment Gateway route to
    purchase authorized full copies, or is there a separate arrangement for
    providing the standards as part of a conformance assessment?
12. The BCC standards list labels its BDS 1520 download as "BDS 1520:2018",
    but the linked document's cover reads "(Second Revision)" and its page
    headers read "BDS 1520:2011", while the BSTI Catalogue 2023 lists BDS
    1520:2018 as "(Third Revision)". Kindly clarify: (a) which text is the
    current authorized edition, and (b) whether the character allocations of
    the Second Revision's Table 2 remain unchanged in the Third Revision.
13. The bstibds.com address (still linked from BSTI's service pages as the
    "e-Catalog & BDS Sale" software) now hosts a license-checking
    application. Is the BDS e-store at ec.eservice.bsti.gov.bd the sole
    official purchase channel?

Current technical status (summary, 2026-11-24): full Bengali-block repertoire
verification (128/128 code points), NFC conformance on the official UAX #15
`NormalizationTest.txt` Bengali lines (7/7), a 579-row multi-layout engine
evidence harness, an ASan/UBSan-clean test suite (591 + 2 PASS), and a 94-row
allocation corpus transcribed from the official BDS 1520 document and
verified against the engine (94/94). A complete evidence dossier is prepared
and available for assessment.

We would be grateful for your written guidance.

Sincerely,
[Applicant name / organization]
[Address, email, phone]
[Date]

---

**Attachments to prepare before sending**: product brief,
BDS_MASTER_COMPLIANCE_MATRIX.csv, test summary, license inventory.
