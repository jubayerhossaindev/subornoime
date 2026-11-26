# Suborno Standalone Mode — ব্যবহার গাইড (বাংলা)

> **এক লাইনে:** ইনস্টল → চালু → Ctrl+Alt+B → যেকোনো অ্যাপে বাংলা।
> ভাষা প্যাক নেই, Win+Space নেই, ভাষা-বার নেই।

## ১. এটা কী?

`SubornoStandalone.exe` হলো SUBORNO IME-র **standalone শেল** — জনপ্রিয় স্ট্যান্ডঅ্যালোন বাংলা
কিবোর্ডের মতোই সিস্টেম-ব্যাপী কিবোর্ড ইন্টারসেপ্ট করে কাজ করে, তবে ইঞ্জিন সেই
একই যা TSF IME-তে চলে (একই phonetic লেআউট, একই dictionary, একই settings.ini)।

## ২. ইনস্টল

```bat
:: প্রথমে Release বিল্ড (TSF পাইপলাইনের সাথেই standalone-ও তৈরি হয়)
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release --parallel 1

:: ইনস্টল + autostart + চালু (admin লাগবে না)
install_standalone.cmd
```

অথবা নির্দিষ্ট exe দিয়ে: `install_standalone.cmd D:\path\SubornoStandalone.exe`

## ৩. ব্যবহার

| কাজ | কীভাবে |
| --- | --- |
| বাংলা চালু/বন্ধ | **Ctrl+Alt+B** বা tray আইকনে ক্লিক |
| টাইপিং | ফোনেটিক রোমান লিখুন (`ami`, `tumi`, `bhalobasha`) — caret-এর পাশে লাইভ প্রিভিউ দেখবেন |
| শব্দ কমিট | **Space / Enter / যতিচিহ্ন** (`. , ! ? ;` ইত্যাদি) |
| ভুল হলে | Backspace দিয়ে অক্ষর পেছান — প্রিভিউও পেছাবে |
| কম্পোজিশন বাতিল | **Esc** |
| সেটিংস | tray মেনু → "Suborno সেটিংস খুলুন…" (TSF অ্যাপ ইনস্টল থাকলে) |

উদাহরণ: লিখুন `ami banglay gan gai` তারপর প্রতি শব্দের পরে Space — ফলাফল:
`আমি বাংলায় গান গাই`।

## ৪. কনফিগ (`%APPDATA%\Suborno\standalone.ini`)

```ini
[standalone]
hotkey=ctrl+alt+b     ; টগল হটকি: ctrl/alt/shift/win + f1..f24/letter/digit/space
preview=1             ; ভাসমান প্রিভিউ (0 = বন্ধ)
enabled_on_start=1    ; হোস্ট চালু হলেই বাংলা মোড শুরু হবে কি না
```

হটকির উদাহরণ: `hotkey=f12` (সাধারণ অভ্যাস) → `[standalone]`-এ
`hotkey=f12` লিখে হোস্ট রিস্টার্ট করুন। শুধু `f12`-এর মতো single-key টগল দিলে
কন্ট্রোলার কোনো modifier ছাড়াই ফিট করে নেবে না — অন্তত একটি modifier
(ctrl/alt/shift/win) রাখা বাধ্যতামূলক, কারণ প্লেইন অক্ষর-টগল সব টাইপিং নষ্ট
করবে। এজন্য ডিফল্ট **Ctrl+Alt+B**: F12 ব্রাউজার devtools-এর, Ctrl+Space VS
IntelliSense-এর সাথে সংঘর্ষ করে।

## ৫. একই ইঞ্জিন, দুই শেল

- settings.ini (`%APPDATA%\Suborno\settings.ini`) শেয়ার্ড — সেটিংস অ্যাপে
  লেআউট বদলালে (যেমন `suborno-phonetic` → `national`) স্ট্যান্ডঅ্যালোন হোস্ট
  পরবর্তী উইন্ডো-ফোকাস বদলালেই নতুন লেআউট ধরে নেবে।
- dictionary learning শেয়ার্ড — TSF-এ যা শিখেছে, standalone-ও তা জানে।
- ফিক্সড লেআউট (National/Probhat/Khipro) ব্যবহার করলে কম্পোজিশন ছাড়াই সরাসরি
  বাংলা ক্যারেক্টার রূপান্তর হবে — প্রিভিউ প্রতিটি কীতে আপডেট হবে।

## ৫.৫ প্রিভিউ-এর ফন্ট

ভাসমান প্রিভিউ বান্ডল করা **Hind Siliguri Medium** ফন্টে রেন্ডার হয়
(SIL OFL 1.1; `install_standalone.cmd` এটা exe-র পাশে বসায়, হোস্ট
`AddFontResourceExW(FR_PRIVATE)` দিয়ে প্রসেস-প্রাইভেট লোড করে — কোনো ফন্ট
ইনস্টল/admin/ভাষা প্যাক লাগে না)। সেটিংসে ইউজারের নিজের ফন্ট পিক করলে সেটাই
জিতবে; কিছুই লোড না হলে Nirmala UI (Windows built-in)। বিস্তারিত:
[FONT_BUNDLING_BANGLA.md](FONT_BUNDLING_BANGLA.md)।

## ৬. পরিচিত সীমা (সৎ তালিকা)

1. **Elevated অ্যাপ** (Run as administrator) — UIPI নিয়মে non-admin হুক সেখানে
   ঢুকতে পারে না। সমাধান: হোস্টটিও admin হিসেবে চালান, বা TSF mode ব্যবহার করুন।
2. **RAW INPUT গেম** — SendInput অনেক গেম উপেক্ষা করে; TSF mode লাগবে।
3. **কিছু UWP/টার্মিনাল** — caret তথ্য না পেলে প্রিভিউ ফোকাসড উইন্ডোর কোণায়
   বসে (টাইপিং তবু ঠিক থাকে)।
4. **ডিজিট** — কম্পোজিশনের মাঝে ডিজিট দিলে শব্দটি কমিট হয়ে ডিজিট লিখা হয়
   (সাধারণ আচরণ); ডিজিটের বাংলা রূপ চাইলে শব্দ কমিটের পরে টাইপ করুন।
5. ক্যান্ডিডেট লিস্ট UI এই সংস্করণে প্রিভিউতে নেই (TSF mode-এ আছে) — commit
   সবসময় প্রাইমারি রূপ নেয়।

## ৭. আনইনস্টল

```bat
uninstall_standalone.cmd
```

autostart + কপি করা exe সরবে; settings/dictionary রয়ে যাবে।
