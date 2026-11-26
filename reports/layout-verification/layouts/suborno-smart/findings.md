# Findings — Suborno Smart

- Brief-এর `t''` FAIL — Suborno Smart-র khanda-ta token হলো t + backtick×2 (``t````→ৎ PASS; suborno_smart_layout.cpp:77-83); apostrophe নয়।
- `y`→য় PASS (nukta handling); `R`→ড়, `Rh`→ঢ় PASS।
- SmartUnchangedHeadlines এই রানে OK — Suborno/Suborno Classic fix-এ Suborno Smart mapping অপরিবর্তিত।
- Accent key = backtick (SubornoSmartLayoutTest.HandlesAccentKeyOverrides OK)।
- Standalone ় টাইপ করা যায় না — capability note।
