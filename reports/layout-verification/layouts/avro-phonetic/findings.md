# Findings — Avro

- Brief-এর `t''` FAIL — Avro-র khanda-ta token হলো t + backtick×2 (``t````→ৎ PASS; avro_phonetic_layout.cpp:77-83); apostrophe নয়।
- `y`→য় PASS (nukta handling); `R`→ড়, `Rh`→ঢ় PASS।
- AvroUnchangedHeadlines এই রানে OK — Suborno/Borno fix-এ Avro mapping অপরিবর্তিত।
- Accent key = backtick (AvroPhoneticLayoutTest.HandlesAccentKeyOverrides OK)।
- Standalone ় টাইপ করা যায় না — capability note।
