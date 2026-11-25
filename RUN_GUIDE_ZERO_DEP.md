# RUN GUIDE — Zero Bengali Language Pack Dependency (V4)

> এই রানবুকটি `MSVC-WARNINGS-REPAIR-02` ফ্লো-এর **সম্পূর্ণ additive সংস্করণ**।
> পুরনো ২০টি marker অক্ষত আছে — `run_final_verification.cmd` আগের মতোই চলবে।
> নতুন যোগ হয়েছে: standalone mode (SubornoStandalone.exe) + ৮টি standalone
> marker + ৬টি bundled-font marker (মোট ৩৫), এবং GATE 8 (ফন্ট বান্ডল)।

## এক নজরে

```
git stash  →  zip extract  →  verify_source_sync.cmd (35/35)
           →  run_final_verification.cmd      (পুরনো পাইপলাইন, TSF IME)
           →  verify_zero_dep.cmd             (৮ গেট, standalone + fonts)
           →  install_standalone.cmd          (HKCU, admin লাগে না)
           →  install_fonts.cmd               (optional: সব অ্যাপে ফন্ট)
           →  Ctrl+Alt+B চাপুন → যেকোনো অ্যাপে বাংলা লিখুন
```

ভাষা প্যাক (Bengali Language Pack) লাগবে না। Win+Space লাগবে না।
TSF IME না ইনস্টল করেও standalone mode কাজ করবে।

## STEP 1 — ব্যাকআপ (একবারই)

```bat
cd /d F:\my-windows-app\suborno\SUBORNOIME\SUBORNOIME
git stash push -m "pre-zero-dep-v4-sync-backup" --include-untracked
```

## STEP 2 — নতুন zip রিপো রুটে extract

`SUBORNOIME_SOURCE_ZERO_DEP_V4.zip` এই ফোল্ডারে extract করুন
(`build\` ফোল্ডার অক্ষত থাকবে — zip এ `build/` নেই)।

## STEP 3 — marker pre-flight

```bat
verify_source_sync.cmd
```

- প্রত্যাশিত ফল: `SYNC-VERDICT: OK — all 35 markers present`
- ২১টি পুরনো (MSVC-WARNINGS-REPAIR-02 + V2) + ৮টি standalone + ৬টি bundled-font marker।

## STEP 4 — পুরনো পাইপলাইন (TSF IME) ভেরিফাই

```bat
run_final_verification.cmd
```

- আগের মতোই: Release + Debug build, CTest, warning inventory, UI regression।
- Evidence: `evidence\MSVC-WARNINGS-REPAIR-02\VERDICT.txt`
- নতুন standalone ফাইলগুলো এই রানের গেটগুলো ভাঙবে না
  (STEP 0-এর git status এখন ইচ্ছাকৃতভাবে বড় — সেটা শুধু প্রিন্ট হয়, gate নয়)।

## STEP 5 — standalone mode ভেরিফাই

```bat
verify_zero_dep.cmd
```

- গেট: configure → build `suborno_standalone` → artifact →
  `StandaloneTypingController*` + `StandalonePreviewFont.*` unit tests →
  `suborno.unit` → helper scripts → GATE 8 bundled fonts (assets + build copy)।
- Evidence: `evidence\ZERO-DEPENDENCY\VERDICT.txt`
- সাবধান: এই মেশিনে Visual Studio 2022 + vcpkg (`VCPKG_ROOT`) আগে থেকেই
  configure করা থাকতে হবে — STEP 4 চললে সেটা নিশ্চিত।

## STEP 6 — ইনস্টল (admin লাগে না)

```bat
install_standalone.cmd
```

- `%LOCALAPPDATA%\Suborno\bin\SubornoStandalone.exe` এ কপি হবে,
  `%LOCALAPPDATA%\Suborno\fonts\` এ বান্ডল ফন্ট (preview-এ Hind Siliguri),
  HKCU Run key-তে autostart বসবে, আর সাথে সাথে চালু হবে।
- Tray icon: **Suborno Standalone**। টগল: **Ctrl+Alt+B**।
- কনফিগ: `%APPDATA%\Suborno\standalone.ini`
  (`hotkey=ctrl+alt+b`, `preview=1`, `enabled_on_start=1`)

## STEP 6.5 — (optional) সব অ্যাপে বান্ডল ফন্ট চালু করতে

```bat
install_fonts.cmd
```

- Hind Siliguri / Baloo Da 2 / Noto Sans Bengali (SIL OFL 1.1) per-user ফন্ট
  হিসেবে রেজিস্টার করে (Win10 1809+, admin লাগে না) — এরপর সেটিংস অ্যাপের ফন্ট
  পিকার বা `settings.ui.font_family` থেকে TSF candidate window-ও এই ফন্টে
  রেন্ডার করা যায়। স্ট্যান্ডঅ্যালোন হোস্টের জন্য এটা দরকার নেই।
- আনইনস্টল: `install_fonts.cmd /uninstall`

## STEP 7 — ব্যবহার

1. যেকোনো অ্যাপে (Notepad, browser, Word) টাইপ শুরু করুন: `ami banglay gan gai`
2. প্রতিটি অক্ষরের সাথে caret-এর পাশে ভাসমান preview দেখবেন।
3. **Space / Enter / যতিচিহ্ন** চাপলেই শব্দটি বাংলা হয়ে বসে যাবে:
   `আমি বাংলায় গান গাই `
4. English-এ ফিরতে: Ctrl+Alt+B (বা tray menu)।
5. Backspace দিয়ে মাঝপথে শব্দ ছোট/বড় করা যায়; Esc কম্পোজিশন বাতিল করে।

## আনইনস্টল

```bat
uninstall_standalone.cmd
```

## সমস্যা হলে

| লক্ষণ | সম্ভাব্য কারণ / সমাধান |
| --- | --- |
| `verify_source_sync.cmd` STALE | zip পুরো রিপো রুটে extract হয়নি; STEP 2 আবার করুন |
| `verify_zero_dep.cmd` GATE 3 fail | `02/03_*.txt` evidence দেখুন; `SUBORNO_BUILD_STANDALONE=OFF` দিলে পুরনো পাইপলাইন একা চলবে |
| `verify_zero_dep.cmd` GATE 8 fail | `assets\fonts` এ ১০টি ttf + OFL + FONT-CREDITS + manifest আছে কি না; build output-এ `Release\fonts` না থাকলে GATE 3 rebuild করুন (POST_BUILD কপি চলবে) |
| Preview দেখা যাচ্ছে না | `standalone.ini`-তে `preview=1` আছে কিনা দেখুন; caret না থাকলে প্রিভিউ ফোকাসড উইন্ডোর নিচে-ডানে বসে |
| Preview সাধারণ (Nirmala) ফন্টে দেখাচ্ছে | `SubornoStandalone.exe`-এর পাশে `fonts\` ফোল্ডার পাওয়া যায়নি — install_standalone.cmd আবার চালান |
| Elevated (Run as administrator) অ্যাপে বাংলা হচ্ছে না | UIPI সীমাবদ্ধতা — standalone host-ও admin হিসেবে চালান, অথবা TSF IME mode ব্যবহার করুন |
| কিছু গেম/RAW INPUT অ্যাপে কাজ করছে না | SendInput মডেলের পরিচিত সীমা — TSF IME mode ব্যবহার করুন |

## সৎ সীমা (honest boundary)

- এই গিটে MSVC বিল্ড স্যান্ডবক্সে চলে না — তাই SubornoStandalone.exe-এর
  Windows রানটাইম আচরণ প্রমাণিত হয় **শুধু আপনার মেশিনে STEP 5-এ**।
  Controller policy ১৫/১৫ সিনারিও sandbox (GCC 14) + MSVC CI-তে টেস্টেড।
- Narrator/UIA হাতে-কলমে চেক এখনও BLOCKED (এজেন্ট দূর থেকে চালাতে পারে না)।
