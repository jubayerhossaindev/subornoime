# Findings — Suborno Classic

- Brief-এর `Ta;i`/`mo;i` সিকোয়েন্স FAIL — Suborno Classic-তে `;` কোনো break key নয়, এটি literal হয়ে যায় (টা;ই/ম;ই)। Engine-এর নিজস্ব কনভেনশন: `Tai`→টাই, `moi`→মই — উভয়ই PASS (convention_sequences.csv)।
- Accent key হলো backtick (`), যা vowel-এর dependent form বাধ্য করে (`a`→া, `ka`→কা; rule_based_phonetic_layout.cpp:254-260)।
- Explicit hasanta = `,` ×2 (`k,,`→ক্ PASS); khanda-ta = `X`।
- Standalone ় এবং সাধারণ k+় টাইপ করা যায় না (য়/ড়/ঢ় শুধু dedicated token হিসেবে) — capability note, brief-mandated কোনো টেস্টে FAIL নেই।
- BUG-05 display-name fix commit cf2a0ca; EngineTest.LayoutDisplayNamesAreCorrectAndScriptClean এই রানে OK।
