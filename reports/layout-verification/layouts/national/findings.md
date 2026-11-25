# Findings — National

- Brief-এর সব 17 sequence PASS; explicit hasanta `X` (pXr→প্র, nXd→ন্দ, sXbadhInta→স্বাধীনতা)।
- Trailing virama সম্ভব: `pX`→প্ PASS (khipro-র বিপরীতে)।
- `;` = vowel break (m;i→মই, Ta;i→টাই), `..` = danda, `tw` = ৎ, `GY` = জ্ঞ, backtick = nukta token (national_layout.cpp:87; k`→ক়)।
- F-1/F-2 fix commit c1462f4; NationalExplicitHasantaFormsConjuncts / NationalPreviouslyMissingCharacters / NationalBreaksAndIndependentVowels এই রানে OK।
