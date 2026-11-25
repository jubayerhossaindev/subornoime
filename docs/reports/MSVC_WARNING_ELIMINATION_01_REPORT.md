# MSVC-WARNINGS-REPAIR-02 — চূড়ান্ত ওয়ার্নিং অপসারণ ও সম্পূর্ণ রিগ্রেশন যাচাই প্রতিবেদন

**তারিখ:** ২০২৬-১১-২৫
**ভাষা:** C++20 / MSVC 14.44.35207 / Windows SDK 10.0.26100.0
**উৎস গাছ:** commit `b94252e` (পূর্ববর্তী `msvc-fix` commit `59196f7`-এর উপর), working tree পরিষ্কার অবস্থা থেকে শুরু
**সুযোগ (Scope):** C28182 ×৩ (WIL), C4715 ×২ (candidate window), CTest configuration mismatch, সম্পূর্ণ রিগ্রেশন যাচাই

> **সততা-সীমানা ঘোষণা:** এই প্রতিবেদনের বিশ্লেষণ ও সমস্ত পোর্টেবল যাচাই একটি Linux sandbox-এ সম্পন্ন হয়েছে। MSVC কম্পাইল, MSVC Code Analysis ও Windows CTest চালানো এখানে সম্ভব নয় — তাই Windows-নির্ভর প্রতিটি ফলাফল **প্রতীক্ষিত (PENDING — Windows runbook অনুযায়ী পুনরায় চালাতে হবে)** হিসেবে চিহ্নিত। কোনো BLOCKED/NOT RUN আইটেমকে PASS করা হয়নি।

---

## ১. বেসলাইন অডিট (Phase 1)

| আইটেম | প্রত্যক্ষ ফলাফল |
|---|---|
| `git status --short` | পরিষ্কার (commit `b94252e`); পূর্ববর্তী সব ফিক্স (`59196f7`) অন্তর্ভুক্ত |
| CMake presets | `windows-msvc` (VS 17 2022), build presets `windows-msvc-release`/`windows-msvc-debug`, test presets একই নামে — কোনো preset পরিবর্তন করা হয়নি |
| CTest নিবন্ধন | `suborno.unit`, `suborno.integration`, `suborno.windows.tsf_contract`, `suborno.windows.app_smoke` (`tests/CMakeLists.txt:80,89,121,123`); `suborno_run_tests` custom target এ `$<CONFIG>` দিয়ে `-C` desync-রোধ বিদ্যমান |
| WIL সংস্করণ | `wil:x64-windows-static@1.0.260126.7`; vcpkg baseline tree-hash `fa919d6da75c4879e2…` — এই sandbox-এ ports/wil ডিরেক্টরির git tree hash হিসাব করে **হুবহু মিল** প্রমাণিত |
| UI inventory | `docs/reports/UI_FUNCTIONAL_AUDIT_INVENTORY_V2.csv` — ঠিক **১১৩টি এলিমেন্ট** (Settings 38, Tray 13, Toast 11, Topbar 10, Candidate 9, About 8, OSK 8, Application 7, Menu 5, All-surfaces 4) — অক্ষত |
| প্রকল্পের WIL ব্যবহার | সম্পূর্ণ grep: `wil::com_ptr` ×৪২, `wil::unique_hmodule` ×১, `wil::unique_bstr` ×১ — **কোনো allocating WIL API ব্যবহার নেই** (`make_unique_nothrow`, string builder, event/folder/registry watcher — শূন্য) |
| প্রকল্পের নিজস্ব `new (std::nothrow)` | ৮টি স্থান (osk_window.cpp:794, class_factory.cpp:66, text_edit_session.cpp:79/152/170/188, text_service.cpp:1021/1168, dll_main.cpp:45) — **প্রতিটিই null-checked** (যাচাইকৃত) |

---

## ২. Warning A — C28182 ×৩ (`wil/resource.h(4467)`) তদন্ত ও নিষ্পত্তি

### ২.১ কী যাচাই করা হয়েছে (প্রমাণ-চেইন)

1. **সঠিক ইনস্টল করা ফাইল পুনর্গঠন:** vcpkg portfile-এ ঘোষিত tarball SHA512-এর সাথে `microsoft/wil` tag `v1.0.260126.7`-এর tarball **হুবহু মেলে**; vcpkg baseline-এর tree hash (`fa919d6…`) ও হিসাব করে মিলিয়েছি। সুতরাং ইনস্টল করা `resource.h` = upstream tag + vcpkg-র `cpp14.patch` — এই sandbox-এ হুবহু পুনর্গঠন করা সম্ভব হয়েছে।
2. **resource.h-এ `new (std::nothrow)`-এর সব স্থান:** মাত্র ৫টি — `wistd::make_unique_nothrow` (২টি, deref নেই), `make_unique_failfast` (২টি, deref নেই), এবং `event_watcher_t::create_take_hevent_ownership` — যেখানে allocation → `RETURN_IF_NULL_ALLOC` → `watcherState->` dereference ধারাটি ESL trace-এর আকৃতির সাথে মেলে।
3. **ফাংশনটি সঠিক কিনা:** allocation-ফেইল পথে `RETURN_IF_NULL_ALLOC(watcherState)` আগেই return করে — অর্থাৎ **dereference-টি null-guarded; WIL কোডে কোনো প্রকৃত null-deref নেই**।
4. **প্রকল্পের ব্যবহার (Case 1 খণ্ডন):** SUBORNO-র কোড থেকে এই টেমপ্লেট পরিবার ইনস্ট্যানশিয়েট করার মতো কোনো allocating WIL API call site নেই (বেসলাইন টেবিল দেখুন)। তিনটি trace-এর dereference-এর অবস্থান থার্ড-পার্টি হেডারে; প্রজেক্ট লাইনে কোনো C28182 নেই।
5. **Upstream যাচাই:** **microsoft/wil#610** (২০ জানুয়ারি ২০২৬-এ "completed" হিসেবে closed) — হুবহু একই বার্তা: `…contains the same NULL value as 'new(1*144, nothrow)' did`, resource.h-এ, WIL maintainer (Duncan Horn) নিজ পরীক্ষায় **false positive নিশ্চিত** করেছেন: `new (std::nothrow)` braced-init-এ allocation fail করলে অবজেক্ট কনস্ট্রাক্টই হয় না, তাই guarded dereference-এ সেই null পৌঁছাতে পারে না — analyzer টেমপ্লেট + ম্যাক্রো-guard মডেল করতে ব্যর্থ হয়। অন্য প্রকল্পেও (microsoft/xdp-for-windows) একই ডায়াগনস্টিক নথিভুক্ত।
6. **`1*144` ব্যাখ্যা:** ESL-এর allocation রেন্ডারিং — `event_watcher_state` অবজেক্টের MSVC লেআউট (wistd::function + handles) — upstream issue-এ একই 144 বাইট দেখা যায়।

### ২.২ নিষ্পত্তি (Disposition): **Case 3 — যাচাইকৃত থার্ড-পার্টি analysis false positive**

- **Case 1 (প্রকল্পের ত্রুটি) — খণ্ডন:** কোনো ব্যবহার-ত্রুটি নেই; প্রজেক্টের প্রতিটি নিজস্ব nothrow-new সাইট guarded।
- **Case 2 (WIL ত্রুটি) — খণ্ডন:** WIL কোড সঠিক (guard বিদ্যমান); upstream নিশ্চিত FP; নতুন WIL সংস্করণে আপগ্রেড এই ডায়াগনস্টিকের জন্য প্রয়োজন নেই এবং টাস্ক-অনুযায়ী স্বয়ংক্রিয় আপগ্রেড করা হয়নি।
- **Case 3 গৃহীত** — Microsoft-এর নথিভুক্ত external-header পদ্ধতিতে সমাধান (`/external:Wn`, `/external:I`, `/analyze:external-` — টাস্কে উদ্ধৃত দুই অফিসিয়াল ডক অনুযায়ী):
  - `/analyze`-এর ডকুমেন্টেড ডিফল্ট হলো external headers **analyze করা** — তাই C28182 এসেছিল;
  - `/external:I` কার্যকর হতে হলে `/external:Wn` থাকতে হবে (নইলে D9007 দিয়ে ignored);
  - `/external:Wn` শুধু সাধারণ ওয়ার্নিং-লেভেল নিয়ন্ত্রণ করে, analysis নয় — তাই `/analyze:external-` আলাদা দরকার।

### ২.৩ বাস্তবায়ন (কোথায়, কতটা সংকীর্ণ)

`cmake/modules/SubornoBuildOptions.cmake` → `suborno_apply_build_options()` — সব MSVC টার্গেটে (`suborno_app`, `suborno_windows_tsf`, উভয় টেস্ট টার্গেট সহ) একটি ইউনিফর্ম, ডকুমেন্টেড নীতি:

```cmake
/external:W4                                   # থার্ড-পার্টি হেডারের সাধারণ ওয়ার্নিং আজকের /W4 লেভেলেই থাকবে (শুধু analysis বদলায়)
"/external:I${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
                                               # শুধুমাত্র vcpkg-ইনস্টল করা dependency include root (wil সহ) external হবে
/analyze:external-                             # external হেডারের analysis বন্ধ — শুধু সেখানেই
```

**প্রমাণিত সীমাবদ্ধতা:**
- external চিহ্নিত হয় **শুধু** `build/windows-msvc/vcpkg_installed/x64-windows-static/include` গাছ — ঠিক সেই ডিরেক্টরি যেখানে `wil/resource.h(4467)` বসে;
- `src/` ও `include/suborno/` — প্রজেক্ট-মালিকানাধীন সব সোর্স/হেডার — **সম্পূর্ণ /W4 + /analyze কভারেজে** থাকে; প্রজেক্ট ফাইলের অবস্থানে রিপোর্ট হওয়া প্রতিটি ডায়াগনস্টিক আগের মতোই আসবে (যেমন C4715 টি ঠিক এভাবেই ধরা পড়েছিল);
- কোনো রুল প্রজেক্ট কোডের জন্য বন্ধ হয়নি; `/wd28182`, `/W0`, ব্ল্যাঙ্কেট `#pragma warning(disable:…)` — কিছুই ব্যবহার করা হয়নি;
- সুবিধা-পার্শ্বফল: আগে থেকে থাকা টেস্ট-টার্গেট `/analyze:external-` (gtest C6326 নীতি) এখন vcpkg-root external designation-এর কারণে gtest হেডারে প্রকৃতপক্ষে প্রযোজ্য হয়।
- Code Analysis **বন্ধ হয়নি** — প্রতিটি টার্গেটে `/analyze` আগের মতোই যুক্ত।

**Windows-এ পুনর্যাচাই (runbook):**
```
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release --clean-first --parallel 1
:: C28182 আর আসবে না; এবং project-owned analysis জীবিত আছে কিনা প্রমাণ:
findstr /C:"analyze" build\windows-msvc\src\platform\windows\suborno_app.vcxproj
:: প্রত্যাশিত: /analyze, /external:W4, /external:I…vcpkg_installed…, /analyze:external- সবই ClCompile-এ থাকবে
```

---

## ৩. Warning B — C4715 (`CandidateListUIElement::window_proc`, লাইন ৮৫১)

### ৩.১ মূল কারণ (পূর্ণ ফাংশন পাঠের ভিত্তিতে)

`window_proc`-এর switch-এ (candidate_list_ui_element.cpp:৭১৬–৮৫০) সব case return করে **শুধু একটি পথ বাদে** — `WM_TIMER`:

```cpp
case WM_TIMER:
    if (w_param == kSlideTimerId) { element->advance_slide(); return 0; }
    break;                                  // ← এই একটি পথ
```

এই window শুধু `kSlideTimerId` টাইমার সেট করে; ভিন্ন (foreign) টাইমার ID এলে `break` switch থেকে বেরিয়ে **ফাংশনের শেষ ব্রেসে (লাইন ৮৫১) পৌঁছে return ছাড়াই** OS-কে undefined LRESULT ফেরত দেয়। এটিই C4715 — এবং দুই টার্গেটে (`suborno_windows_tsf` + `suborno_windows_tests`) একই সোর্স-লেভেল ত্রুটির দুই কম্পাইলেশন, তাই সোর্সে একবার ঠিক করলেই দুটোই মুছে যায়।

### ৩.২ মেরামত

```cpp
// সংশোধিত (candidate_list_ui_element.cpp:752–762):
case WM_TIMER:
    if (w_param == kSlideTimerId) { element->advance_slide(); return 0; }
    // A timer ID this window did not create: delegate to the default
    // window procedure … (C4715, undefined return to the caller.)
    return ::DefWindowProcW(window, message, w_param, l_param);
```

- অহ্যান্ডেলড মেসেজের জন্য সঠিক Unicode ডিফল্ট প্রসিডিউর (`DefWindowProcW` — এই ফাংশনের ৭১৩, ৮৪৪, ৮৪৯ লাইনের সাথে সামঞ্জস্যপূর্ণ); HWND/message/WPARAM/LPARAM অবিকৃত;
- কনভেনশন (`static LRESULT CALLBACK`, হেডার ঘোষণা অপরিবর্তিত) — বদলায়নি; কোনো handler মুছেনি; অন্ধ `return 0` দেওয়া হয়নি — `DefWindowProcW` foreign টাইমারের কলব্যাক (SetTimer-নিবন্ধিত TimerProc) সঠিকভাবে dispatch করে, তাই এটিই প্রামাণিক আচরণ।

### ৩.৩ Window-lifecycle যাচাই (B3 — পূর্ণ ফাংশন পাঠ)

- `WM_NCCREATE`: `lpCreateParams` → `GWLP_USERDATA` সেট + `AddRef()` — ঠিক আছে; `lpCreateParams == nullptr` হলে `FALSE` return;
- `WM_NCCREATE`-র আগের মেসেজ (যেমন `WM_GETMINMAXINFO`): `element == nullptr` → `DefWindowProcW` — ঠিক আছে;
- `WM_NCDESTROY`: আগে `DefWindowProcW`, তারপর `Release()` + `GWLP_USERDATA` ক্লিয়ার — use-after-destroy নেই;
- ডিজাইন-চুক্তি অক্ষত: কখনো activate হয় না (`MA_NOACTIVATE`), click-to-commit (`WM_LBUTTONUP`) অপরিবর্তিত, placement/animation (`start_slide_in`/`advance_slide`) অপরিবর্তিত, UI Automation ইচ্ছাকৃতভাবে অনুপস্থিত — অপরিবর্তিত।

### ৩.৪ পোর্টেবল প্রমাণ (sandbox-এ সম্পাদিত)

`scripts/verify_window_proc_return_paths.py` — **প্রকৃত ফাংশন-বডি হুবহু উত্তোলন করে** (pre-fix = `git HEAD`, post-fix = working tree) একটি Win32-stub harness-এ `g++ -std=c++20 -Wall -Wextra -Werror=return-type` দিয়ে কম্পাইল করে:

| সংস্করণ | ফলাফল |
|---|---|
| pre-fix (HEAD) | `error: control reaches end of non-void function` — C4715-এর পোর্টেবল যমজ |
| post-fix | **পরিষ্কার (rc=0)** — প্রতিটি control path এখন return করে |

### ৩.৫ Regression test বিষয়টি

পোর্টেবল সুইটে live HWND/TSF context ছাড়া foreign-timer path চালানো সম্ভব নয়; বিদ্যমান `suborno.windows.tsf_contract` টেস্ট কম্পাইল-লেভেল কভারেজ দেয়। একটি live-window টাইমার টেস্ট যোগ করা এই পরিবেশে যাচাই করা সম্ভব নয় — **BLOCKED (Windows runtime)** হিসেবে নথিভুক্ত, PASS করা হয়নি। স্থায়ী regression guard: C4715 ওয়ার্নিং নিজেই (দুই কম্পাইলেশনে)।

---

## ৪. CTest configuration mismatch (টাস্কের সেকশন ৫) — নিষ্পত্তি

**মূল কারণ:** বিল্ড হয়েছিল Release-এ (`cmake --build --preset windows-msvc-release`), কিন্তু টেস্ট চালানো হয়েছিল `ctest -C Debug` দিয়ে — Debug executable (`Debug\suborno_windows_tests.exe`, `Debug\Suborno.exe`) বিল্ড গাছেই ছিল না → দুই Windows টেস্ট **Not Run**। এটি কোনো CMake/preset ত্রুটি নয় — operational ভুল। `suborno.unit`/`suborno.integration` চলেছিল কারণ এগুলো exe-নামের জন্য `$<TARGET_FILE>` নয়, টার্গেট-নাম ব্যবহার করে এবং ctest সেগুলোর Release বাইনারি খুঁজে পায়নি বটে `unit/integration` গ্রুপের জন্য `-C`-অজানা পথে চলে গিয়েছিল।

**সঠিক কমান্ড (কোনো preset/জেনারেটর পরিবর্তন ছাড়া):**

```
:: Phase 5 — Release (প্রথমে -N দিয়ে নিবন্ধন যাচাই):
ctest --test-dir build\windows-msvc -N -C Release
ctest --test-dir build\windows-msvc -C Release --output-on-failure

:: Phase 6 — Debug আলাদাভাবে:
cmake --build build\windows-msvc --config Debug --parallel 1
:: (সমতুল্য: cmake --build --preset windows-msvc-debug --parallel 1)
ctest --test-dir build\windows-msvc -C Debug --output-on-failure

:: Desync-প্রমাণ বিকল্প (preset-নিবন্ধিত suborno_run_tests টার্গেট):
cmake --build --preset windows-msvc-release --target suborno_run_tests
```

প্রত্যাশা: Release-এ ৪টি গ্রুপই RUN হবে (exe বিদ্যমান: `build\windows-msvc\Release\` এ `Suborno.exe`, `suborno_windows_tests.exe` — ব্যবহারকারীর বিল্ড লগেই প্রমাণিত)। **Not Run কখনো PASS গণ্য হবে না।**

---

## ৫. সম্পাদিত ফাইল (Git diff-নিশ্চিত — Phase 8)

| ফাইল | পরিবর্তন | কারণ |
|---|---|---|
| `src/platform/windows/tsf/candidate_list_ui_element.cpp` | +৬/−১ লাইন (WM_TIMER foreign-ID path: `break;` → `return ::DefWindowProcW(…)`, সহ ব্যাখ্যামূলক কমেন্ট) | C4715 — একমাত্র non-returning path |
| `cmake/modules/SubornoBuildOptions.cmake` | +৪৪ লাইন (`suborno_apply_build_options`-এ external-header analysis নীতি + সম্পূর্ণ প্রমাণ-কমেন্ট) | C28182 ×৩ — Case 3 disposition |

- অপরিবর্তিত: সব UI সোর্স, settings key, persistence, i18n, layout, public C API, COM/TSF চুক্তি, সব টেস্ট ও CTest নিবন্ধন, ডকুমেন্টেশন, clang-format (যাচাইকৃত: `clang-format --dry-run -Werror` পরিষ্কার)।
- নতুন স্ক্রিপ্ট (repo-র বাইরে, sandbox-নির্দিষ্ট): `scripts/verify_window_proc_return_paths.py` — প্রমাণ পুনঃপ্রস্তুতির জন্য।

---

## ৬. CTest ফলাফল (Section C — সততার সাথে আলাদা)

| টেস্ট গ্রুপ | Release (Windows) | Debug (Windows) | Linux portable (sandbox-এ সম্পাদিত) |
|---|---|---|---|
| suborno.unit | **প্রতীক্ষিত** — উপরের runbook চালান | **প্রতীক্ষিত** | **PASS — ৬৩৩/৬৩৩** (Release, g++, `ctest` + বাইনারি দ্বৈত যাচাই) |
| suborno.integration | **প্রতীক্ষিত** | **প্রতীক্ষিত** | **PASS — ২/২** |
| suborno.windows.tsf_contract | **প্রতীক্ষিত** (পূর্বে Not Run — Debug/Release mismatch) | **প্রতীক্ষিত** | প্রযোজ্য নয় (Windows-only) |
| suborno.windows.app_smoke | **প্রতীক্ষিত** (পূর্বে Not Run) | **প্রতীক্ষিত** | প্রযোজ্য নয় (Windows-only) |

- ব্যবহারকারীর সর্বশেষ Windows বিল্ড লগ অনুযায়ী: unit+integration Release-এ PASS (118.82s / 4.04s), windows দুটি Not Run — এই প্রতিবেদনের পরিবর্তনের পর সেগুলো চলবে।
- **কোনো টেস্ট disable/skip/unregister করা হয়নি; কোনো assertion দুর্বল হয়নি (৬৩৩+২ কাউন্ট বেসলাইনের সমান — নিশ্চিতকৃত)।**

---

## ৭. Regression coverage (Section D)

- **Inventory:** ১১৩/১১৩ এলিমেন্ট পুনঃপরীক্ষিত (CSV অক্ষত — বেসলাইন টেবিল); ২৩টি UIAudit ইউনিট টেস্ট সহ পোর্টেবল সুইট সম্পূর্ণ PASS।
- **প্রকৃত চালানো টেস্ট:** ৬৩৩ unit + 2 integration (Linux Release) — এই sandbox-এ সম্পাদিত।
- **Windows runtime যাচাই:** BLOCKED — interactive candidate-window আচরণ, Narrator/UIA client এখানে উপলব্ধ নয়। Phase 2 audit-এর BLOCKED স্ট্যাটাস অপরিবর্তিত।
- **UI চুক্তিতে পরিবর্তন:** **শূন্য।** WM_TIMER foreign-ID পথটি audited আচরণ কখনো exercise করে না (candidate window একমাত্র নিজের `kSlideTimerId` সেট করে); মেরামতটি undefined return → প্রামাণিক `DefWindowProcW` delegation। Slide-in animation, click-to-commit, keyboard navigation, placement — সব অপরিবর্তিত। CMake পরিবর্তনে runtime আচরণে কোনো প্রভাব নেই।
- **National layout / BDS 1738:2018 positional implementation:** অস্পৃশ্য — অক্ষত।

---

## ৮. বাকি থাকা বিষয় (Section F — সৎ স্বীকারোক্তি)

1. **Windows পুনর্বিল্ড ও ৪ CTest গ্রুপ** এখনো চালাতে হবে (runbook উপরে) — Release/Debug দুই কনফিগের চূড়ান্ত সবুজ স্ট্যাটাস ও "শূন্য actionable warning" দাবি সেই লগের আগে করা হচ্ছে না।
2. C28182-এর তিনটি ইনস্ট্যান্সের মধ্যে **কোন TU-তে কোন তিনটি instantiation** হয়েছিল তার TU-লেভেল ম্যাপিং এখান থেকে নিশ্চিত করা সম্ভব নয় (Windows build tree-র `*.nativecodeanalysis.xml`/SARIF এখানে নেই); তবে একই guarded implementation-এর পুনঃইনস্ট্যানশিয়েশন হওয়াই সম্ভাব্য ব্যাখ্যা — টাস্কে উল্লিখিত সম্ভাবনাগুলোর একটি — এবং disposition (Case 3) এর উপর নির্ভর করে না।
3. MSVC analyzer ভবিষ্যতে ঠিক হলে বা WIL নতুন সংস্করণে পরিবর্তন এলে `/external` scope আরও সংকীর্ণ করা যাবে — নীতিটি সচেতনভাবে সংকীর্ণ ও ডকুমেন্টেড।
4. Live-window টাইমার regression টেস্ট — Windows runtime নির্ভর, BLOCKED।

---

## ৯. চূড়ান্ত গ্রহণযোগ্যতা-চেকলিস্ট ম্যাপিং

| শর্ত | অবস্থা |
|---|---|
| Release builds successfully | পরিবর্তনগুলো পোর্টেবল বিল্ড+টেস্টে সবুজ; MSVC পুনর্বিল্ড **প্রতীক্ষিত** (runbook) |
| Debug builds where configured | **প্রতীক্ষিত** (`windows-msvc-debug` preset অক্ষত) |
| C4715 fixed with correct semantics | **সম্পন্ন** — DefWindowProcW delegation; পোর্টেবল প্রমাণসহ; lifecycle যাচাইকৃত |
| All three C28182 investigated | **সম্পন্ন** — SHA512/tree-hash-ভেরিফাইড সোর্স অডিট + upstream #610 |
| No genuine null dereference ignored | **নিশ্চিত** — ট্রেসড WIL কোড guarded; প্রজেক্টের ৮টি nothrow-new সাইটও guarded |
| Narrow third-party policy documented | **সম্পন্ন** — শুধু vcpkg include root; `/external:W4`-এ সাধারণ ওয়ার্নিং অপরিবর্তিত; CMake কমেন্টে পূর্ণ ন্যায্যতা |
| MSVC Code Analysis remains enabled | **হ্যাঁ** — `/analyze` সব টার্গেটে; প্রজেক্ট লাইনের ডায়াগনস্টিক আগের মতো আসে |
| All four CTest groups execute & pass in Release | **প্রতীক্ষিত** — সঠিক কমান্ড নথিভুক্ত; Not Run ≠ PASS নীতি বজায় |
| Debug results reported separately | এই প্রতিবেদনে আলাদা কলাম; Windows রানে পূরণ হবে |
| No test disabled/weakened | **নিশ্চিত** — ৬৩৩+২ অপরিবর্তিত |
| Phase 2 UI contracts & 113-element inventory | **অক্ষত** — CSV + সুইট যাচাইকৃত |
| National layout intact | **অক্ষত** — অস্পৃশ্য |
| Final Git diff reviewed | **সম্পন্ন** — ২ ফাইল, ৫০(+)/১(−); অপ্রাসঙ্গিক কিছু নেই |
