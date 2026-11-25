# Findings — Khipro

- Brief-এর সব 14 sequence PASS; corpus 49/49; paragraph 3/3।
- **RESOLVED (2026-11-24)**: trailing/displaced virama (ক্ = U+0995 U+09CD) এখন explicit hasanta token `qq` দিয়ে টাইপ করা যায় — আগের KNOWN LIMITATION (inventory 66/67, INV-064 NOT_DERIVABLE) দূর। Inventory এখন 74/74। Token নিয়ম: ব্যঞ্জনের পরে তাৎক্ষণিক virama + run alive; standalone/vowel-পরে silent break; repeated হলে idempotent; `/` দিয়ে undo সম্ভব। (Khipro Classic quickstart convention; source: khipro_layout.cpp)
- Investigation trail: `;` terminal extension প্রত্যাখ্যাত (documented partition key), `/` silent-ender extension প্রত্যাখ্যাত (documented slicer semantics), upstream `qq` convention গৃহীত (collision-free — কোনো corpus/test input-এ `qq` নেই)।
- National explicit hasanta (`pX`→প্) ও Khipro `qq` — দুই layout-এর নিজস্ব convention, capability parity এখন আছে।
- Case-sensitive: uppercase T/Q token নয় (literal passthrough) — `kQQ`→কQQ অপরিবর্তিত।
- দাঁড়ি = single `.`; `..` = escaped literal dot (khipro_layout.cpp)।
- `,` = nukta (k,,→ক়), `;` = vowel break (m;i→মই), `/` = chandrabindu, `t/` = ৎ — সব অপরিবর্তিত (full corpus + brief + probe re-run দ্বারা প্রমাণিত)।
