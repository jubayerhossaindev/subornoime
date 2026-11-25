# Layout Report — English (`english`)

- **Execution**: 2026-11-23 17:51Z run (Asia/Dhaka 2026-11-23 23:51) — fresh run from HEAD `a241477`
- **Environment**: Linux sandbox (Debian 13, GCC 14.2.0, engine-level InputSession); Windows/MSVC unavailable
- **Overall Verdict**: **ENGINE VERIFIED (passthrough identity) — WINDOWS RUNTIME BLOCKED**

## 1. Implementation

Passthrough — EnglishLayout::transliterate = identity; TSF কি কখনো intercept করে না (text_service.cpp:325,597,678); src/layouts/english/english_layout.cpp

`EnglishLayout::transliterate` ইনপুট অপরিবর্তিত ফেরত দেয়; `generate_candidates` engine-side fallback session-এ ইনপুট-নিজকে candidate হিসেবে দেয়। বাস্তব Windows-এ TSF সার্ভিস passthrough kind দেখে কি কখনো ধরে না — কি সরাসরি হোস্ট অ্যাপে পৌঁছায়।

## 2. Passthrough Verification (engine-level)

| Test | Input | Actual | Verdict |
|---|---|---|---|
| passthrough-identity | `hello` | `hello` | PASS |
| passthrough-identity | `world` | `world` | PASS |
| passthrough-identity | `SubornoIME` | `SubornoIME` | PASS |
| passthrough-identity | `Hello 2026!` | `Hello 2026!` | PASS |
| passthrough-identity | `MiXeD CaSe 123 !@#` | `MiXeD CaSe 123 !@#` | PASS |
| passthrough-identity | `bangla.amar.desk` | `bangla.amar.desk` | PASS |
| passthrough-editing | `helo + VK_BACK + lo` | `hello` | PASS |

বি.দ্র.: engine fallback session-এর snapshot-এ preedit/candidates দেখা যায় — এটি engine-ভেতরের defensive fallback; TSF-স্তরের passthrough আচরণ text_service.cpp:325,597,678 দ্বারা নিশ্চিত এবং চূড়ান্ত প্রমাণ Windows runtime টেস্টে হবে (BLOCKED)।

## 3. Bangla Corpus

**NOT APPLICABLE** — বাংলা corpus/paragraph/inventory English passthrough layout-এর ক্ষেত্রে প্রযোজ্য নয় (brief §6.3)।

## 4. Layout Switching

`khipro→english` ও `english→suborno-phonetic` chain PASS (switch-chain rows in brief-results.csv); switch-এর পর English identity অটুট।

## 5. Windows Runtime

**BLOCKED** — windows-results.csv।

