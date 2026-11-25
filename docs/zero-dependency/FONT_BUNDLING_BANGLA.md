# বান্ডল করা বাংলা ফন্ট — স্ট্যান্ডঅ্যালোন হোস্টের জন্য (বাংলা)

> **এক লাইনে:** SubornoStandalone.exe-এর ভাসমান প্রিভিউ এখন বান্ডল করা
> Hind Siliguri ফন্টে রেন্ডার হয় — কোনো ফন্ট ইনস্টল, admin অথবা ভাষা প্যাক ছাড়াই।

## ১. কী করা হয়েছে (ki ki korcho)

`assets/fonts/`-এ তিনটি ওপেন-সোর্স (SIL OFL 1.1) বাংলা ফন্ট ফ্যামিলি ভেন্ডর
করা হয়েছে — মোট ১০টি টিটিএফ, ~২.৩৮ MB:

| ফ্যামিলি | ফাইল | ফাউন্ড্রি | ভূমিকা |
| --- | --- | --- | --- |
| **Hind Siliguri** | ৫ ওয়েট (Light 300…Bold 700) | Indian Type Foundry | প্রিভিউ-এর মূল ব্র্যান্ড ফন্ট (Medium 22px) |
| **Baloo Da 2** | ৩ ওয়েট | Ek Type | ব্র্যান্ড/display (About, ভবিষ্যৎ হেডিং) |
| **Noto Sans Bengali** | ২ ওয়েট | Google / Noto | কভারেজ-রেফারেন্স ফলব্যাক |

প্রতিটি ফ্যামিলির পূর্ণ OFL লাইসেন্স টেক্সট (`OFL-*.txt`), বাইট-নির্ভুল
SHA-256 ম্যানিফেস্ট (`fonts_manifest.csv`) আর প্রোভেনেন্স+QA রেকর্ড
(`FONT-CREDITS.md`) ফন্টের পাশেই আছে।

নতুন কোড:

- `src/platform/windows/standalone/font_loader.{hpp,cpp}` — exe-এর পাশের
  `fonts\` ফোল্ডার (বা ইনস্টল লেআউটে `bin\..\fonts`) থেকে প্রতিটি .ttf
  `AddFontResourceExW(FR_PRIVATE)` দিয়ে **প্রসেস-প্রাইভেট** লোড করে। সিস্টেম
  ফন্ট টেবিলে (HKLM/HKCU) কিছুই লেখা হয় না; অন্য কোনো অ্যাপ এই ফন্ট দেখে না;
  প্রসেস শেষ হলে রেজিস্ট্রেশন নিজে থেকেই শেষ।
- `src/platform/windows/standalone/preview_font_choice.hpp` — pure, Windows-মুক্ত
  পলিসি: user pick > Hind Siliguri Medium > Hind Siliguri > Nirmala UI।
  ৭টি পোর্টেবল ইউনিট টেস্ট (`StandalonePreviewFont.*`) এই অগ্রাধিকার পিন করে।
- `preview_window.cpp` এখন পলিসির দেওয়া family+weight ব্যবহার করে
  (হার্ডকোড "Nirmala UI" বাদ)।

## ২. কেন এই ডিজাইন (keno korcho)

1. **Zero-dependency থিমের ফন্ট-অংশ।** টাইপিং যেমন কোনো ভাষা প্যাক ছাড়া চলে,
   প্রিভিউ-ও এমন ফন্টে দেখাক যা যেকোনো Windows 8+ মেশিনে নিশ্চিত — Nirmala UI
   সেটা করতো, কিন্তু সেটা জেনেরিক আর Microsoft-নিজস্ব (বান্ডল করা যায় না)।
2. **FR_PRIVATE = শূন্য ইনস্টল ব্যথা।** Avro-র মতো ইউজার অভিজ্ঞতা: ইনস্টল →
   চালু → কাজ। ফন্টের জন্য আলাদা ধাপ নেই।
3. **Non-RIBBI নেমিং ব্যাপার।** Hind Siliguri-র নন-রেগুলার ওয়েটগুলো আলাদা
   ফ্যামিলি নাম বহন করে ("Hind Siliguri Medium" ইত্যাদি — fonttools দিয়ে
   name-table ভেরিফাই করা); তাই পলিসি সরাসরি ওই স্ট্রিংগুলো দেয়, "weight
   দিয়ে ম্যাচ করা"-র অনিশ্চয়তা এড়াতে।
4. **Nirmala UI শেষ সেফটি নেট।** কোনো কারণে ফন্ট ফাইল না পাওয়া গেলেও বাংলা
   কখনো টুটলে দেখাবে না — Nirmala Windows 8+ থেকে built-in।

## ৩. প্রিভিউ vs candidate window — কোথায় কোন ফন্ট

| পৃষ্ঠ | ফন্ট | কারণ |
| --- | --- | --- |
| স্ট্যান্ডঅ্যালোন ভাসমান প্রিভিউ | বান্ডল Hind Siliguri (FR_PRIVATE) | Suborno-র নিজের UI chrome — ব্র্যান্ড লুক, শূন্য কনফিগ |
| সেটিংসে ইউজারের পিক (`settings.ui.font_family`, os-default off) | ইউজারের বাছাই | ইউজারের সিদ্ধান্ত সর্বোচ্চ |
| TSF candidate window | `settings.ui.font_family`, ডিফল্ট Nirmala UI | ডকুমেন্ট-মুখী টেক্সট; বান্ডল ফন্টে স্যুইচ করতে `install_fonts.cmd` চালিয়ে নিন |

## ৪. ভেন্ডর-সময়ের QA (২০২৬-১১-২৫, fonttools)

- সব ১০ ফাইল: TTF magic + full parse OK।
- বাংলা ব্লক (U+0980–U+09FF) cmap কভারেজ: Hind 92/128, Noto 93/128,
  Baloo 88/128।
- প্রয়োজনীয় সেট (স্বর/ব্যঞ্জন/মাত্রা/০–৯/nukta/danda/chandrabindu/hasanta):
  Hind ও Noto-তে সম্পূর্ণ।
- **U+09E7 (১) বাগ ভেরিফাইড FIXED** — ২০২৩-এর google/fonts issue #6037-র
  malformed glyph বর্তমান v14 বিল্ডে সঠিক (`bnOne`, contours আছে, ৫টি ওয়েটেই)।
- পরিচিত gap: Baloo Da 2-তে বিরল U+0980 (Anji) নেই — display ফন্টের জন্য
  গ্রহণযোগ্য।

## ৫. অ্যাক্সেপটেন্স গেট

- `verify_zero_dep.cmd` **GATE 8**: `assets\fonts`-এ ১০টি ttf + ৩টি OFL +
  FONT-CREDITS.md + manifest, এবং Release exe-এর পাশে POST_BUILD কপি।
- `verify_source_sync.cmd`-এ ৬টি নতুন marker (মোট ৩৫)।

## ৬. লাইসেন্স বাধ্যবাধিতা (SIL OFL 1.1)

- ফন্টগুলো অসংশোধিত (কোনো subset/rename/convert নেই)।
- প্রতিটির লাইসেন্স টেক্সট সাথে বিতরণ হচ্ছে।
- ফন্ট আলাদাভাবে বিক্রি হয় না; অ্যাপের অংশ হিসেবে ফ্রি বিতরণ।
- Reserved Font Name কোনো derivative-এ ব্যবহার হয়নি (কোনো derivative নেই)।

## ৭. সৎ সীমা (honest boundary)

- MSVC বিল্ড স্যান্ডবক্সে চলে না — AddFontResourceExW/FR_PRIVATE-এর রানটাইম
  আচরণ আপনার মেশিনে verify_zero_dep.cmd (GATE 8) + চোখে দেখা প্রিভিউতে
  প্রমাণিত হবে। পলিসি লজিক ৭টি পোর্টেবল টেস্টে স্যান্ডবক্সেই চালানো হয়েছে।
- "Noto Sans Bengali **UI**" ভ্যারিয়েন্ট notofonts-এর অ্যাক্সেসযোগ্য মিররে
  ছিল না; নিয়মিত Noto Sans Bengali ফ্যামিলি ভেন্ডর করা হয়েছে (outlines এক,
  শুধু UI-মেট্রিক্স ভিন্ন) — এক লাইন প্রিভিউতে পার্থক্য নেই।
- GDI DrawTextW ফন্ট-লিঙ্কিং করে না: প্রিভিউ টেক্সট এক ফন্টেই রেন্ডার হয়।
  Hind Siliguri-র বাংলা কভারেজ সম্পূর্ণ, তাই বাস্তব ঝুঁকি নেই; ইউজারের
  পিক-করা ফন্টে কোনো গ্লিফ না থাকলে GDI সাবস্টিটিউট করবে।
