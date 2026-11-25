# Zero Bengali Language Pack Dependency — গভীর গবেষণা ও স্থাপত্য পরিকল্পনা

তারিখ: ২০২৬-১১-২৫ · লেখক: সাবর্ণ (SUBORNO IME) ইঞ্জিনিয়ারিং · স্ট্যাটাস: বাস্তবায়িত (V3)

---

## ১. সমস্যা বিবরণ

SUBORNO IME শুরু থেকে **TSF (Text Services Framework) IME** হিসেবে তৈরি — অর্থাৎ
`suborno_windows_tsf.dll` কম্পিউটারে একটি "text service" হয়ে বসে, আর Windows-এর
input-language মডেলের ভেতর দিয়ে কাজ করে। এই মডেলের জন্মগত শর্ত:

1. Windows Settings → Time & Language → **Bengali (Bangladesh)** ইনপুট ভাষা যোগ করতে হয়;
2. ভাষা বদলাতে **Win+Space** (বা Alt+Shift / ভাষা বার) ব্যবহার করতে হয়;
3. কোনো কোনো মেশিনে বাংলা ইনপুট চালু করতে **Bengali Language Pack/feature** চায়।

কিন্তু বাস্তব ব্যবহারকারী (আমাদের টার্গেট ইউজার) কখনো Win+Space ব্যবহার করেন না।
তিনি অভ্যস্ত **Bijoy, Avro Keyboard, Borno**-র মডেলে: সফটওয়্যার ইনস্টল → চালু →
যেকোনো অ্যাপে সরাসরি বাংলা। ভাষা প্যাক নেই, Win+Space নেই, ভাষা-বারে হাত যায় না।
সুতরাং "Zero Bengali Language Pack Dependency" শুধু সুবিধা নয় — এটা বাজারে টিকে
থাকার **প্রবেশদ্বার শর্ত (entry condition)**।

## ২. প্রতিযোগীদের মেকানিজম — গভীর গবেষণা

### ২.১ Avro Keyboard (OmicronLab)

- প্রোডাক্ট পাতা নিজেই বলছে: *'Write anywhere "ami banglay gan gai", it will be
  automatically typed — "আমি বাংলায় গান গাই"'* + **Floating Preview Window**
  ("See on the fly how your English text is converting to Bangla")।
- স্থাপত্য: TSF IME নয় — এটি একটি স্বতন্ত্র অ্যাপ যা সিস্টেম-ব্যাপী কিবোর্ড
  ইন্টারসেপ্ট করে, নিজের phonetic ইঞ্জিনে ট্রান্সলিটারেট করে, আর টেক্সট
  ইনজেক্ট করে (SendInput/Unicode পাইপলাইন)। ফলে ভাষা প্যাক, ভাষা বার বা
  Win+Space-এর প্রয়োজনই পড়ে না। ডিফল্ট টগল শর্টকাট **F12** (পরে বদলানো যায়)।
- সূত্র: omicronlab.com/avro-keyboard.html; Bengali input methods (Wikipedia)।

### ২.২ Borno (CodePotro, Windows)

- প্রোডাক্ট পাতা: *"Get instant transliteration results as you type. Press the
  **F10** key to instantly switch between Bangla and English modes"* — অর্থাৎ
  এখানেও ইনপুট সুইচ Windows-এর ভাষা মডেলে নয়, **অ্যাপের নিজের হটকিতে** হয়।
- সূত্র: codepotro.com/borno।

### ২.৩ Bijoy (Ananda Computers)

- Wikipedia: প্রোপ্রাইটারি লেআউট, *"an ASCII-Unicode-based Bengali input
  software"*, Windows-এ কিবোর্ড-লেআউট ড্রাইভার (KLID) হিসেবে বসে — অর্থাৎ এটি
  ভাষা-প্যাকের চেয়ে নিচের স্তরে (keyboard layout) নেমে যায়, তাই ইনস্টলের
  পরপরই লেআউট সুইচ করে টাইপ করা যায়। মূল্য: প্রতি কম্পিউটারে লাইসেন্স।
- সূত্র: en.wikipedia.org/wiki/Bengali_input_methods।

### ২.৪ Windows-এর নিজের IME রাস্তা (TSF)

- Microsoft-এর নিজস্ব গাইড: ইনপুট লেআউট/ভাষা যোগ ও সুইচ করতে Settings আর
  Win+Space ব্যবহার করতে হয় — TSF TIP-ও একই মডেলে বসে (langprofile LANGID
  0x0445)। সুতরাং TSF IME = "পূর্ণ IME অভিজ্ঞতা, কিন্তু ভাষা-মডেল নির্ভরতা"।
- সূত্র: support.microsoft.com — "Manage the language and keyboard/input
  layout settings in Windows"।

### ২.৫ তুলনা টেবিল

| বৈশিষ্ট্য | Bijoy | Avro | Borno | Suborno TSF (আগে) | Suborno Standalone (এখন) |
| --- | --- | --- | --- | --- | --- |
| ভাষা প্যাক লাগে? | না | না | না | **হ্যাঁ (ভাষা কার্ড)** | **না** |
| Win+Space লাগে? | না (লেআউট সুইচ) | না (F12) | না (F10) | **হ্যাঁ** | **না (Ctrl+Alt+B)** |
| ইনস্টল-পরবর্তী ব্যবহার | তাৎক্ষণিক | তাৎক্ষণিক | তাৎক্ষণিক | সেটআপ দরকার | **তাৎক্ষণিক** |
| কম্পোজিশন UX | fixed layout | ভাসমান প্রিভিউ | ইনস্ট্যান্ট | ক্যান্ডিডেট উইন্ডো | ভাসমান প্রিভিউ |
| ইঞ্জিন | লেআউট ড্রাইভার | হুক+ইনজেকশন | হুক+ইনজেকশন | TSF এডিট সেশন | **হুক+ইনজেকশন (একই ইঞ্জিন)** |

## ৩. তিনটি সম্ভাব্য রাস্তা — মূল্যায়ন

### রাস্তা A: শুধু TSF-ই রাখা (status quo) — বাতিল

Win+Space/ভাষা কার্ড নির্ভরতা থেকেই যায়; টার্গেট ইউজারের অভ্যাসের সাথে না মেলা।

### রাস্তা B: legacy keyboard-layout DLL (Bijoy-স্টাইল) — বাতিল

HKLM keyboard-layouts রেজিস্ট্রি + .kbd ড্রাইভার দিয়ে ভাষা কার্ড ছাড়া লেআউট
বসানো যায়, কিন্তু: (১) fixed-layout-ই প্রায় একমাত্র বাস্তব পথ — phonetic
transliteration প্রায় অসম্ভব; (২) আমাদের মূল ইউএসপি phonetic ইঞ্জিন; (৩) legacy
layout ড্রাইভার লেখা ও সাইন করা আলাদা বড় প্রকল্প; (৪) বাংলা ডিজিট/যতি ইত্যাদির
ম্যাপিং সীমিত।

### রাস্তা C: standalone host — হুক + SendInput (Avro/Borno মডেল) — **নির্বাচিত**

একটি স্বতন্ত্র `SubornoStandalone.exe`:
`WH_KEYBOARD_LL` হুক → একই portable ইঞ্জিন (`suborno_core`) দিয়ে ট্রান্সলিটারেট
→ `SendInput` (KEYEVENTF_UNICODE) দিয়ে বাংলা ইনজেকশন → caret-অনুসারী ভাসমান
প্রিভিউ। ভাষা প্যাক/Win+Space/ভাষা বার — কোনোটাই ছোঁয় না।

**কেন এটাই সঠিক:**

1. **একই ইঞ্জিন, দুটি শেল** — TSF IME-র সব বুদ্ধি (suborno_core: layouts,
   candidate engine, dictionary, learning) অবিকৃত থেকে পুনঃব্যবহৃত হয়; আমরা
   শুধু "ড্রাইভিং লেয়ার" বদলাই।
2. **ব্যবহারকারীর মানসিক মডেল** — Avro/Borno ব্যবহারকারী শূন্য শেখায় চালাতে পারবে।
3. **ঝুঁকি বিচ্ছিন্নকরণ** — TSF পাইপলাইন (MSVC-WARNINGS-REPAIR-02 সহ) এক মিলিমিটারও
   বদলায় না; নতুন কোড পুরোপুরি additive।
4. **একই settings.ini + dictionary** — TSF শেল ও standalone শেল এক কনফিগ শেয়ার
   করে (app_paths.cpp পুনঃব্যবহার), তাই লেআউট/থিম বদলালে দুটোই একসাথে বদলায়।

**ট্রেড-অফ (সৎ স্বীকারকরণ):** elevated (admin) অ্যাপে non-admin হুক-প্রসেস UIPI-র
কারণে টাইপ করতে পারে না; RAW INPUT-নির্ভর কিছু গেমে SendInput কাজ নাও করতে পারে;
UWP অ্যাপে WM_CHAR আচরণ ভিন্ন হতে পারে। এই ক্ষেত্রে TSF IME mode-ই উত্তর — তাই
দুই mode-ই রাখা হয়েছে (Avro-ও তার "System Keyboard/compatibility" পথ রাখে)।

**Win+Space কি থাকবে?** TSF IME mode-এ থাকবে (ওটা Windows-এর মডেল), standalone
mode-এ দরকারই নেই — টগল হটকি (ডিফল্ট Ctrl+Alt+B; Avro-র F12 ব্রাউজার devtools-এর
সাথে, Ctrl+Space VS IntelliSense-এর সাথে সংঘর্ষের ঝুঁকিতে এটি বেছে নেওয়া হয়েছে,
`standalone.ini`-তে বদলানো যায়)।

## ৪. স্থাপত্য

```
কিবোর্ড (হার্ডওয়্যার)
   │  WM_KEYDOWN
   ▼
WH_KEYBOARD_LL (keyboard_hook.cpp)
   │  LLKHF_INJECTED ফিল্টার → ফিডব্যাক-লুপ গার্ড
   │  ToUnicodeEx → HookKeyInput{letter|digit|space|enter|punct|...}
   ▼
TypingController (typing_controller.hpp — portable, unit-tested)
   │  layout::InputSession (suborno_core: phonetic parser+layouts+candidates)
   │  সিদ্ধান্ত: passthrough / compose / commit(erase_units, inject_utf8, preedit)
   ▼
Win32 এক্সিকিউশন
   ├─ injector::send_backspaces(n)           (latin মুছুন)
   ├─ injector::send_text_utf8(বাংলা)        (KEYEVENTF_UNICODE)
   ├─ injector::send_virtual_key(VK_SPACE)   (গিলে-ফেলা কী ফিরিয়ে দিন)
   └─ PreviewWindow                          (caret-অনুসারী ভাসমান প্রিভিউ)
```

**টাইপিং ফ্লো (উদাহরণ "ami "):**

1. `a` → প্যাসথ্রু (অ্যাপে "a" দেখা যায়), সেশন কম্পোজ করে, প্রিভিউ "আ"।
2. `m` → প্যাসথ্রু, প্রিভিউ "আম"; `i` → প্যাসথ্রু, প্রিভিউ "আমি"।
3. `space` → গিলে ফেলা হয় → ৩টি backspace ইনজেক্ট → "আমি" ইনজেক্ট →
   VK_SPACE আবার ইনজেক্ট → স্ক্রিনে ফলাফল: "আমি "।

**কেন retroactive replacement (Avro-স্টাইল) বেছে নিলাম, "সব কী গিলে ফেলা" নয়:**
কী-গিলে-ফেলা মডেলে টাইপ করার সময় স্ক্রিনে কিছুই দেখা যায় না (টাইপিং অন্ধ হয়ে
যায়), আর যতিচিহ্ন/মিক্সড ইনপুটে ভঙ্গুর। Retroactive মডেলে ইউজার সবসময় তার
কীস্ট্রোক দেখে; বাংলা রূপ প্রিভিউ উইন্ডোতে থাকে; কমিটে একবারে বসে যায়।

## ৫. বাস্তবায়ন ইনভেন্টরি (V3)

| ফাইল | ধরন | দায়িত্ব |
| --- | --- | --- |
| `src/platform/windows/standalone/typing_controller.hpp` | নতুন (portable) | পুরো টাইপিং পলিসি — unit-tested |
| `src/platform/windows/standalone/keyboard_hook.hpp/.cpp` | নতুন | LL হুক, ফিডব্যাক-লুপ গার্ড, কী শ্রেণিবিন্যাস |
| `src/platform/windows/standalone/unicode_injector.hpp/.cpp` | নতুন | SendInput: backspace / UTF-16 টেক্সট / VK |
| `src/platform/windows/standalone/preview_window.hpp/.cpp` | নতুন | caret-অনুসারী ভাসমান প্রিভিউ (NOACTIVATE) |
| `src/platform/windows/standalone/standalone_tray.hpp/.cpp` | নতুন | tray আইকন + মেনু + settings চালুকরণ |
| `src/platform/windows/standalone/standalone_main.cpp` | নতুন | হোস্ট: single-instance, engine, WinEvent, pump |
| `src/platform/windows/standalone/CMakeLists.txt` | নতুন | `suborno_standalone` টার্গেট (MSVC v143) |
| `src/platform/windows/CMakeLists.txt` | সম্পাদিত | `SUBORNO_BUILD_STANDALONE` অপশন (ডিফল্ট ON) |
| `tests/unit/standalone_typing_controller_test.cpp` | নতুন | ১৪টি gtest সিনারিও |
| `tests/CMakeLists.txt` | সম্পাদিত | টেস্ট সোর্স যোগ (marker অক্ষত) |
| `install_standalone.cmd` / `uninstall_standalone.cmd` | নতুন | HKCU ইনস্টল/আনইনস্টল (admin-মুক্ত) |
| `verify_zero_dep.cmd` | নতুন | ৭টি acceptance gate + evidence |
| `verify_source_sync.cmd` | সম্পাদিত | ২১ → ২৯ markers (V3 যোগ) |
| `RUN_GUIDE_ZERO_DEP.md` | নতুন | বাংলা রানবুক |
| `docs/zero-dependency/STANDALONE_MODE_GUIDE_BANGLA.md` | নতুন | ব্যবহার গাইড |

## ৬. যাচাই পরিকল্পনা (যা আমরা আসলেই চালিয়েছি)

- **Controller policy (sandbox, GCC 14, -Wall -Wextra -Wpedantic): ১৫/১৫ সিনারিও
  পাস** — passthrough, compose, preview, commit("ami"→"আমি"), enter/punct commit,
  backspace trim, escape, literal-passthrough, input_limit, digit-commit,
  focus-lost, disable/resume, rebind।
- একই টেস্ট gtest হিসেবে MSVC ইউনিট বাইনারিতে ঢুকেছে → আপনার মেশিনে
  `verify_zero_dep.cmd` GATE 5 একই assertion চালাবে।
- বিদ্যমান প্রমাণ: `phonetic_input_session_test.cpp`-তে "ami" → preedit "আমি" /
  commit "আমি" — আমাদের controller-এর ধরে-নেওয়া repo-র নিজস্ব টেস্টে প্রমাণিত।

## ৭. সীমাবদ্ধতা ও পরবর্তী ধাপ

- MSVC/Windows রানটাইম বিল্ড শুধু আপনার মেশিনে সম্ভব (স্যান্ডবক্সে নেই) — RUN GUIDE
  STEP 5 প্রমাণের জায়গা।
- Elevated apps, RAW INPUT গেম, কিছু UWP ক্ষেত্রে TSF mode ব্যবহার করুন।
- পরবর্তী (V4 ধারণা): প্রিভিউ উইন্ডোতে ক্যান্ডিডেট লিস্ট + নাম্বার-কী নির্বাচন,
  প্রতি-অ্যাপ নীরব তালিকা (exclude list), UIAccess ম্যানিফেস্ট দিয়ে elevated
  সাপোর্ট, সেটিংস অ্যাপে "Standalone" পাতা।

সূত্র: omicronlab.com/avro-keyboard.html · codepotro.com/borno ·
en.wikipedia.org/wiki/Bengali_input_methods ·
support.microsoft.com (manage language and keyboard/input layout settings)
