# MSVC-WARNINGS-REPAIR-02 — Windows 同步与最终验证指南 / Windows sync & final verification guide (V2)

> **কেন এই প্যাকেজ / Why this package**
> ২০২৬-১১-২৫ তারিখে প্রথম সিঙ্কের পর Release বিল্ড সফল হয়েছে (২৪টি টার্গেট: Suborno.exe,
> suborno_windows_tsf.dll, সব টেস্ট exe), কিন্তু CTest-এ ৩১টি unit + ১টি TSF
> contract টেস্ট ব্যর্থ হয়েছে এবং `--clean-first`-এ MSB5004 (solution-এ দুটো
> "run_tests" প্রজেক্ট) দেখা দিয়েছে। **V2 প্যাকেজ** সেই ৬ শ্রেণির সব ত্রুটি ঠিক করে
> (MSB5004, cmd /c কোটিং, ফাইল-লক lifetime, settings CRLF, quarantine টেস্ট
> semantics, TSF page-start assertion) — কোনো অ্যাপ ফাংশনালিটি বদলায়নি।
>
> After the first sync the Release build PASSED (all 24 targets) but 31 unit +
> 1 TSF contract tests failed and --clean-first hit MSB5004. V2 fixes all six
> defect classes without touching application functionality.

## ধাপসমূহ / Steps (run on the Windows machine)

### 0) বর্তমান অবস্থা ব্যাকআপ / Capture current state first
```bat
cd /d F:\my-windows-app\suborno\SUBORNOIME\SUBORNOIME
git rev-parse HEAD
git status --short
git stash push -m "pre-msvcrepair02-sync-backup" --include-untracked
```
(যদি git ব্যবহার না করেন: পুরো `src\` ও `cmake\` ফোল্ডার দুটির একটি জিপ ব্যাকআপ নিন।
If you do not use git: zip `src\` and `cmake\` as a backup instead.)
**কোনো লোকাল পরিবর্তন মুছে ফেলা হবে না — stash/ব্যাকআপ রাখবেন। Do not delete local
changes — stash/backup keeps them.**

### 1) প্যাকেজ এক্সট্র্যাক্ট / Extract the sync package over the repo
`SUBORNOIME_SOURCE_MSVCREPAIR02_SYNC_V2.zip`-এর ভিতরের **ফোল্ডারের ভিতরের**
ফাইলগুলো (`src\`, `cmake\`, `tests\`, `CMakeLists.txt`, …) সরাসরি repo রুটে
কপি-ওভাররাইট করুন। `build\` ফোল্ডার অন্তত রাখুন — zip-এ `build\` নেই।
Extract the zip's inner folder contents over the repo root (copy/overwrite).
The zip contains no `build\` folder — keep the existing one.

### 2) সিঙ্ক যাচাই / Verify the sync
```bat
verify_source_sync.cmd
```
২০টি marker-এর সবগুলো `SYNC-OK` দেখাবে এবং exit code 0 হবে। একটিও `SYNC-STALE`
দেখালে এক্সট্র্যাকশন অসম্পূর্ণ — আবার কপি করুন।
All 20 markers must print `SYNC-OK` (exit code 0).

### 2b) IME নিবন্ধন — বাংলা টাইপিং চালু করতে / Register the IME — REQUIRED FOR TYPING
বাংলা টাইপিং কাজ করবে শুধুমাত্র টেক্সট সার্ভিস নিবন্ধিত হলে (admin দরকার):
1. Win+X → **Terminal (Admin)** খুলুন
2. `cd /d F:\my-windows-app\suborno\SUBORNOIME\SUBORNOIME\build\windows-msvc\Release`
3. zip-এর রুট থেকে কপি করা `install_suborno_ime.cmd` চালান
4. সফল হলে: **Win+Space** → "Suborno (Bengali, Bangladesh)" বেছে নিন → Notepad/Word/ব্রাউজারে টাইপ করুন
5. নিবন্ধন ছাড়া কোনো অ্যাপে বাংলা টাইপ হবে না — TSF টেক্সট সার্ভিস তখনই তৈরি হয়

The test run already proved registration never happened: the TSF contract
suite SKIPPED `RegistersAndUnregistersTheLanguageProfile` because the shell
was not elevated, and the profile is only created by `DllRegisterServer`
(elevated `regsvr32 suborno_windows_tsf.dll`).

### 3) চূড়ান্ত ভেরিফিকেশন / Run the final verification
```bat
run_final_verification.cmd
```
- স্ক্রিপ্টটি নিজেই evidence লিখবে: `evidence\MSVC-WARNINGS-REPAIR-02\`
- শেষে `VERDICT.txt` তৈরি হবে (PASS হলে "ALL GATES PASS")
- ব্যর্থ গেট থাকলে স্ক্রিপ্ট **nonzero exit** দেবে (fixed 2026-11-25)
- প্রত্যাশিত সময়: প্রায় ৩০–৬০ মিনিট (clean-first Release + Debug build + সব টেস্ট, serial)

### 4) Evidence ফেরত পাঠান / Return the evidence
`evidence\MSVC-WARNINGS-REPAIR-02\` ফোল্ডারটি জিপ করে পাঠান। এর ভিতরে থাকবে:
git status, configure log, clean Release build log, warning inventory (C4715 /
C28182 / D9007 counts), `.vcxproj` + `CL.command.1.tlog` কমান্ড-লাইন প্রমাণ,
Release ও Debug CTest আউটপুট, UIAudit ফলাফল, VERDICT.txt।
Zip and share the evidence folder — it contains every gate log plus VERDICT.txt.

## এই ভেরিফিকেশনে কী হবে / What the verification gates

| Gate | শর্ত |
|---|---|
| Release build | clean-first, ৫টি target (`Suborno.exe`, `suborno_windows_tsf.dll`, unit/integration/windows tests) |
| Warnings | C4715 = 0, C28182 = 0 (external-header policy), D9007 = 0, নতুন project-owned warning নেই |
| Code Analysis config | `/analyze`, `/external:W4`, `/external:I<vcpkg include>`, `/analyze:external-` — `.vcxproj` + tlog-এ প্রমাণিত |
| Release CTest | ৪টি গ্রুপ (`suborno.unit`, `suborno.integration`, `suborno.windows.tsf_contract`, `suborno.windows.app_smoke`) — NOT RUN ≠ PASS |
| Debug build+CTest | একই ৪টি গেট স্বাধীনভাবে |
| UI regression | ১১৩ UI element inventory + `UIAudit*` subset |

## সীমাবদ্ধতা / Known limits (BLOCKED items — কখনো PASS বলা হবে না)
- Narrator announcement যাচাই — interactive desktop দরকার (manual)
- UI Automation client-এ ১১৩ element live walk — interactive shell দরকার (manual)
- Live candidate-window slide/click-to-commit — বাস্তব TSF session-এ typing দরকার (manual)

## zip-এর sha256 / Integrity
শা-২৫৬ হ্যাশ রিপোর্টে দেওয়া আছে; এক্সট্র্যাক্টের আগে মিলিয়ে নিন।
Verify the SHA-256 printed in the delivery report before extracting.
