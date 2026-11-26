# COMPLETE EXISTING UI/UX AUDIT REPORT — SubornoIME

- **Audit Date:** 2026-11-24
- **Repository HEAD:** `f8f33d2fb2e13854b9fb4442ce1dfe3f9340e02a` (branch `main`, working tree clean)
- **Mission type:** Audit-only — বর্তমানে Implemented UI/UX-এর পরীক্ষা, বিশ্লেষণ ও Documentation। এই Audit-এ কোনো UI Design বা Source Code পরিবর্তন করা হয়নি (Report-এর শেষে "Fix Pass" আলাদা Section-এ বর্ণিত, যা User-এর পৃথক নির্দেশে সম্পন্ন হয়েছে)।
- **Verdict (short):** ENGINE/LOGIC VERIFIED — **WINDOWS RUNTIME BLOCKED**। বিস্তারিত Section 32-এ।

---

## 1. Executive Summary

SubornoIME-এর বর্তমানে Implemented Native Windows Application UI হল একটি সম্পূর্ণ Custom-drawn Win32 shell — কোনো React/WebView/Electron/Qt নেই; প্রতিটি Surface C++20 + Win32 API দিয়ে আঁকা (owner-drawn), এবং প্রতিটি Surface-এর জন্য আলাদা UI Automation provider আছে। এই Audit HEAD `f8f33d2`-এ সম্পন্ন হয়েছে; পুরোনো Audit Baseline `5724528`-এর পর থেকে UI Source-এ একমাত্র পরিবর্তন হল BUG-05 fix — `kLayouts`-এ `L"বাংলা (Suborno Classic)"` (mixed-script corruption দূর)।

মূল সংখ্যা (সব Count Source Data থেকে স্ক্রিপ্ট দিয়ে হিসাব):

- **12 UI Surface** Implemented (Settings window, About, Topbar pill, OSK, Toast, Menu popup, Tray menu, Tray icon, TSF Candidate window, Error/Confirmation dialogs, Tooltips, Installer/Uninstaller)। Help নামে আলাদা Window **নেই** (help text Settings/About-এর ভেতরে — NOT APPLICABLE)।
- **Functional Verification Matrix: 133 rows** — Functional: **24 PASS / 0 FAIL / 95 BLOCKED / 0 NOT TESTED / 14 NOT APPLICABLE**; Visual: **1 PASS (executed contrast recompute) / 0 ISSUE FOUND / 95 BLOCKED / 23 NOT TESTED / 14 NOT APPLICABLE**।
- **95টি BLOCKED row-এর একক কারণ:** এই Sandbox-এ Windows desktop নেই — Screenshot, live UIA tree, Narrator, tray click, DPI transition, Installer run কোনোটিই সম্ভব নয়। **কোনো BLOCKED row PASS-এ convert করা হয়নি; কোনো Screenshot fabricate করা হয়নি।**
- পুরোনো Reports 01–09-এর **27টি Fixed claim বর্তমান Source-এ re-verified** — সবই **VERIFIED FIXED**; 0 STILL PRESENT, 0 REGRESSION।
- Executed evidence (আজ, HEAD-এ): **557/557 unit + 2/2 integration test PASS**, locale parity **160 = 160 (en/bn), 0 missing**, WCAG contrast recompute **22 pair-এ 20 PASS** (বাকি 2 pair-এর কোনো consumer নেই — documented pairing rule মেনে চলছে)।
- **নতুন Findings (এই Audit-এ প্রথমবার ধরা পড়ল):** N-01 ও N-02 (Medium) — পুরোনো Report-গুলোতে যে `i18n_key_check.py`, `contrast_check.py`, `merge_audit_results.py`, `checkmap.json` উল্লেখ/ব্যবহৃত ছিল, সেগুলো Repository-তেই নেই; N-03 (Low) — Automation README Script-এর চেয়ে বেশি দাবি করছে; N-04 (Low) — Interface-language/Font dropdown-এ দৃশ্যমান caption নেই, `settings.interface.language` key অব্যবহৃত; N-05/N-06 (Informational) — কোনো embedded Manifest নেই (DPI শুধু runtime call-এ), Installer English-only; Brief-এর "85 automation checks" সংখ্যাটি আসলে **84**; N-07 — সব Windows-runtime evidence BLOCKED।

**Overall Verdict: NEEDS IMPROVEMENT** — Logic/Engine স্তরে প্রমাণসহ সবুজ, কিন্তু এটি এখনো Windows-runtime-audited প্রোডাক্ট নয়। Production-এর জন্য যা বাকি আছে তা Section 30-এ।

---

## 2. Audit Scope

এই Audit **শুধুমাত্র বর্তমানে Implemented UI/UX** পরীক্ষা করেছে। Scope-এ ছিল: Repository/Environment verification, সম্পূর্ণ UI Inventory, প্রতিটি Surface-এর Visual + Functional + Accessibility পরিদর্শন, Bilingual (bn/en) Audit, Typography, DPI, Automation infrastructure, User Workflow, পুরোনো Findings-এর Reconciliation।

**নিষিদ্ধ ছিল (এবং পালন করা হয়েছে):** নতুন UI, Redesign, Modernization, Layout/Control/Color/Font/Spacing/Icon পরিবর্তন, Feature যোগ-বিয়োগ, Refactor, Bug fix (Report-এর মূল অংশে), Backend behavior পরিবর্তন। Technology অপরিবর্তিত: C++20 / Win32 / TSF / COM / MSVC v143 / CMake / Native controls।

পদ্ধতি: (1) প্রতিটি UI-relevant Source File সম্পূর্ণ পড়া হয়েছে — `application.cpp` (4,243 lines), `topbar.cpp` (1,002), `osk_window.cpp` (854), `menu_popup.cpp` (425), `toast_window.cpp` (364), `ui_accessibility.cpp` (843), `candidate_list_ui_element.cpp` (845), `ui_theme.hpp` (687), দুইটি locale JSON, installer script, automation script; (2) Executed evidence পুনরায় চালানো হয়েছে (test suite, parity, contrast, brace scan, layering); (3) পুরোনো 16টি Report + 129-row Traceability Matrix-এর সঙ্গে প্রতিটি claim-এর Reconciliation করা হয়েছে। কোনো উপস্থিত-নেই এমন UI অনুমান করে তালিকায় যোগ করা হয়নি।

## 3. Repository Baseline

| Item | Verified value (executed 2026-11-24) |
|---|---|
| Repository root | `/home/z/my-project/workspace/SUBORNOIME/SUBORNOIME/` |
| Branch | `main` |
| HEAD | `f8f33d2fb2e13854b9fb4442ce1dfe3f9340e02a` |
| Working tree | clean (audit শুরুতে) |
| পুরোনো UI Audit baseline | `5724528` (Report 09); তারপর fix commits `993968c`, `5724528`, `6a124ea`, `cf2a0ca` (BUG-05), `c1462f4` (typing campaign) |
| UI Source diff `5724528..HEAD` | মাত্র 2 file, 6 changed line — সবই BUG-05 display-name fix (`application.cpp`, `topbar.cpp` comment) |
| Test suite | **557/557 unit + 2/2 integration PASS** (পুনরায় চালানো) |
| Project version | 0.1.0 (`CMakeLists.txt`) |
| CI | `ci.yml`-এ `windows-msvc` job (windows-2022 runner): MSVC build + ctest + `suborno_app --smoke-test`; `release.yml`-এ InnoSetup 6.3.3 installer |
| Brief-এর উল্লেখিত ফাইল | `WINDOWS_UI_FUNCTIONAL_AUDIT.md`, `WINDOWS_ACCESSIBILITY_AUDIT.md`, `FINAL_UI_FUNCTIONAL_VERIFICATION.md`, `UI_FUNCTIONAL_AUDIT_TRACEABILITY_MATRIX.csv` (129 rows), `UI_UX_REMAINING_ITEMS_IMPLEMENTED.md`, `UI_FUNCTIONAL_AUDIT_09_SUMMARY.md`, `Invoke-SubornoUiAudit.ps1` — **সব পাওয়া গেছে ও পড়া হয়েছে**; `UI_UX_FINDINGS_FULLY_APPLIED.md` ও `checkmap.json` — **নেই** (N-02/N-06) |

পুরোনো Report-কে Current UI-এর চূড়ান্ত প্রমাণ হিসেবে নেওয়া হয়নি — প্রতিটি claim বর্তমান Source-এ যাচাই করা হয়েছে (Section 26)।

## 4. Execution Environment

- **OS:** Linux sandbox — **Windows desktop নেই, MSVC নেই।** তাই Windows Runtime evidence ক্লাসের প্রতিটি জিনিস (launch, mouse, keyboard, UIA tree, Screenshot, Narrator, DPI, Installer) **BLOCKED**।
- **যা সত্যিই চালানো গেছে:** portable test suite (GCC), locale/contrast/brace/layering স্ক্রিপ্ট, source-level audit।
- **Windows-এ execution-এর অনুমোদিত পথ:** (a) CI-র `suborno.windows.app_smoke` (`suborno_app --smoke-test`, `tests/CMakeLists.txt:120`); (b) `scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1 -ExePath <built Suborno.exe>` (PS 5.1, in-box .NET UIA — কোনো নতুন dependency নেই); (c) `Manual-Test-Protocol.md`-এর 35টি manual step।
- **নিয়ম পালিত:** Linux Sandbox-এ Screenshot বা Source দেখে "বাস্তব Windows UI কেমন দেখায়" — এমন কোনো দাবি করা হয়নি। যে UI বাস্তবে দেখা হয়নি তার Visual Status PASS ঘোষণা করা হয়নি।

## 5. Existing UI Architecture

Architecture-এর ভিত্তি হলো একটি **shared visual token system** (`ui_theme.hpp`, 687 lines): primitive (kSubornoDark/kSubornoLight literal RGB) → semantic (ThemePalette role + `theme_*()` accessor — প্রতিটি drawn surface শুধু এই path দিয়ে পড়ে) → component geometry (`metrics::kButtonHeight=34`, `kDropdownHeight=34`, spacing scale 4/8 grid)। High Contrast সক্রিয় হলে `system_color_palette()` system colors থেকে palette বানায় এবং সব saved theme override করে।

- **Localization read path:** `ui::text(key, fallback)` — UTF-8 JSON → UTF-16, per-key cache; ফাইল না পেলে compiled English fallback (shell কখনো খালি string render করে না)। `format_text()` `{placeholder}` ভরে (status line, toast)।
- **UIA shape:** প্রতিটি window `ui_access::ElementSource` implement করে; WM_GETOBJECT-এ `handle_get_object()` IRawElementProviderSimple + Fragment(+Root) provider দেয়; pattern হিসেবে Invoke/Toggle/ExpandCollapse/RangeValue; liveness token (shared_ptr) দিয়ে stale-provider crash ঠেকানো; সব callback exception-barrier।
- **Process-level:** Per-Monitor v2 DPI awareness runtime API দিয়ে (`enable_process_dpi_awareness()`, fallback chain সহ), `scaled()`/`dpi_fitting()` দিয়ে geometry। Theme/Font পরিবর্তনে `WM_THEMECHANGED`/`refresh_active_theme()`।
- **TSF surface:** candidate window TSF-এর ভেতরে `ITfCandidateListUIElement(Behavior)` হিসেবে — UIA তাকে TSF plumbing দিয়ে দেখে; accessible name "Suborno candidates"।

## 6. Complete UI Inventory

পূর্ণ field-সহ তালিকা `UI_INVENTORY.csv`-তে; সারসংক্ষেপ:

| # | Surface | Window Class | Source | Status |
|---|---|---|---|---|
| 1 | Settings window (Main Window, 4 pages, 36 interactive elements) | `SubornoDesktopWindow` | application.cpp | Implemented; source-verified; runtime BLOCKED |
| 2 | About dialog (5 hotspots) | `SubornoAboutWindow` | application.cpp L588–1205 | Implemented; BLOCKED |
| 3 | Topbar pill (4 zones + drag + context menu) | `SubornoTopbarWindow` | topbar.cpp | Implemented; BLOCKED |
| 4 | On-Screen Keyboard (37 invokable elements) | `SubornoOskWindow` | osk_window.cpp | Implemented; BLOCKED |
| 5 | Toast notification (2.6 s auto-dismiss, WS_EX_NOACTIVATE) | `SubornoToastWindow` | toast_window.cpp | Implemented; BLOCKED |
| 6 | Menu popup component | `SubornoMenuPopupWindow` | menu_popup.cpp | Implemented; BLOCKED |
| 7 | Native tray context menu (10 items + 2 submenus) | Win32 `HMENU` | application.cpp L3799 | Implemented; BLOCKED |
| 8 | System tray icon (`Shell_NotifyIconW` v4) | — | application.cpp L3736 | Implemented; BLOCKED |
| 9 | TSF candidate window | TSF UI element | candidate_list_ui_element.cpp | Implemented; BLOCKED |
| 10 | Error/Confirmation dialogs (`MessageBoxW`, 24 localized error keys) | — | application.cpp L302 | Implemented; BLOCKED |
| 11 | Native tooltips (topbar 4 zones) | `TOOLTIPS_CLASS` | topbar.cpp L324 | Implemented; BLOCKED |
| 12 | Installer/Uninstaller wizard (InnoSetup, English) | — | packaging/windows/suborno_installer.iss | Implemented; BLOCKED |
| — | Help Window | — | — | **NOT APPLICABLE (Implemented নেই; help Settings hint text ও About-এ)** |
| — | Candidate Window হিসেবে আলাদা App window | — | — | শুধু TSF candidate (SURF-09) আছে |

## 7. Main Window Visual Audit

"Main Window" = Settings window (`SubornoDesktopWindow`, 736×620 design px, frameless `WS_POPUP`, DWM rounded corners — Windows 11-এ `DWMWA_WINDOW_CORNER_PREFERENCE=ROUND`, Windows 10-এ hand-cut region fallback)।

- **Title:** localized (`settings.window.title` = "সুবর্ণ সেটিংস" / "Suborno Settings"); App icon `IDI_SUBORNO`; custom-painted dark surface `#2C2C2C`।
- **Structure:** title band (40 px) + close X zone (696–728 × 4–36), left sidebar (28,132×64 থেকে, stride 44, pill 36) 4টি page pill, content area x=232 থেকে, footer Save (498,560,110×34) + Cancel (616,560,100×34)।
- **Typography:** `create_message_font()` (NONCLIENTMETRICS lfMessageFont + configured family), heading 20/semibold, title 16/semibold, caption 12 — সব DPI-scaled।
- **Honest limitation:** এই সব Visual মান Source-level থেকে **traced**; বাস্তব Render (Bengali glyph shaping, spacing দেখতে কেমন, text clipping) দেখা যায়নি — **Visual Status = BLOCKED**। একটি executed visual-metric evidence আছে: contrast recompute (Section 17)।
- Text clipping প্রতিরোধ মেকানিজম Source-এ আছে: section description `DT_WORDBREAK`-এ আঁকা (L3415), menu item `DT_END_ELLIPSIS` (menu_popup.cpp L232), candidate string ellipsis + anchor clamp (L821, L455–478)।

## 8. Main Window Functional Audit

Staged-settings model: সব control `staged_` struct-এ লেখে, **Save**-এ `save_settings_from_ui()` (startup preference → settings save → rollback-on-failure → profile refresh → live apply: theme/language/hotkeys/fonts/topbar → toast → window hide)। **Cancel/Close(X)/Esc/WMS_CLOSE** staged discard + tray-তে hide (L1858–1867)।

- প্রতিটি checkbox-এর explicit backend mapping (`toggle_staged_checkbox` L2408–2470: 16 action), প্রতিটি dropdown-এর explicit mapping (`open_dropdown` L2514–2590), slider mapping (`apply_staged_slider`)।
- Invalid input: slider value clamp (min/max); shortcut dropdown-এ Windows-reserved পুরোনো মান visible থাকে (data loss নেই); settings file corrupt হলে quarantine + defaults (557-suite-এ executed)।
- Persistence: registry-first `write_startup_preference`, save failure-এ rollback + localized `error.settings_save_failed`; atomic write (`persistence/atomic_file.hpp`)।
- Functional Status: **সব runtime interaction BLOCKED**; logic-layer contracts executed tests-এ PASS (557/557)।

## 9. Settings Window Audit

**Control Inventory (বর্তমান Source থেকে, সব localized):**

- **Sidebar:** 4 ListItem (`sidebar.*`)।
- **General:** Remember Last Input Layout, Start with Windows (2 checkbox); "When Windows starts, show" (3 option), "Layout switching mode" (2 option), "Default keyboard layout" (7 layout) dropdown; Windows input status line।
- **Interface:** Show Candidate Window, Next Word Prediction, Fixed Height, Sliding Effect, Follow the Caret, Follow Mouse Pointer…, Use OS Default Bangla Font (7 checkbox); Orientation (2), Theme (3), Interface language (3: system/English/বাংলা), App & Candidate Font (OS default + enumerated Bangla-capable families) dropdown।
- **Topbar:** Enable Idle Dimming, Remember Topbar Position (2 checkbox); Idle Opacity (%), Idle Time (Seconds) (2 slider, keyboard: ←/→/PgUp/PgDn/Home/End)।
- **Input:** Map Numpad, Ignore Caps Lock, Select candidate using number keys, Switch input language to bn-BD, Disable encoding shortcut (5 checkbox); Suggestions & Learning (2), Layout Switching Shortcut (registerable list), Encoding (Unicode/Bijoy ANSI) dropdown; **Clear Learnt Words…** button (immediate action + MB_YESNO confirm, default No)।
- **Footer:** Save, Cancel, Close (X)।

**বিশেষ নোট (নতুন Finding N-04):** Theme (y=410), Interface language (y=448), Font (y=514) dropdown — তিনটিই "Appearance" section-এর নিচে **দৃশ্যমান caption ছাড়া**; `settings.interface.language` ("ইন্টারফেস ভাষা") key দুই locale-এই আছে কিন্তু কোথাও render হয় না। UIA Name-এও তিনটির fallback একই "Appearance" (unique শনাক্তকরণ সম্ভব শুধু AutomationId দিয়ে: `action.ui-theme`/`action.ui-language`/`action.font`)। Severity: Low (content-based শনাক্তকরণ সম্ভব); Fix-এর জন্য layout reflow লাগবে — এই Audit/Pass-এ পরিবর্তন করা হয়নি, Recommendation দেওয়া হয়েছে।

## 10. Layout Selector Audit

Layout selection UI-র সব পথ: (1) Tray/Topbar context menu → **Keyboard Layout** submenu — 7টি radio item (English, বাংলা (Suborno), বাংলা (Suborno Classic), Suborno Smart, Khipro, Probhat, National), active-তে MF_CHECKED, নির্বাচনে `save_layout()` সাথে সাথে persist + topbar label + status line + toast "লেআউট: {layout}"; (2) Settings General → Default keyboard layout dropdown (staged); (3) Topbar mode icon (A/ব) — English↔last-Bangla toggle; (4) OSK-র English/বাংলা mode buttons; (5) Global hotkey `cycle_layout()` (visible order English → বাংলা (Suborno) → …)।

- **নামের সততা:** "Phonetic" শব্দ user-visible নামে নেই; BUG-05 fix-এর পর ৭টি নামই পরিষ্কার (`application.cpp:136-145`)।
- **UI State vs Engine State:** `save_layout()` engine-এ সাথে সাথে লেখে এবং `engine_->active_layout_id()` থেকেই সব UI label আঁকা হয় — highlight শুধু দেখিয়ে আটকে থাকার path নেই (menu নির্মাণে প্রতিবার fresh `engine_->settings()` পড়া হয়, L3815)। Runtime-এ বাস্তব typing-সহ মিল দেখা **BLOCKED** (layout-verification report-এ engine-level switch-chain 7/7 PASS evidence আছে)।
- Keyboard accessibility: menu পথ native TrackPopupMenu (arrow/Enter/Esc); dropdown পথ Tab+Space+arrow; hotkey পথ সর্বত্র।

## 11. System Tray Audit

- **Icon:** `Shell_NotifyIconW` NIM_ADD → NIM_SETVERSION (NOTIFYICON_VERSION_4); failure-এ localized `error.tray_icon_add_failed`; `TaskbarCreated` broadcast-এ re-add (Explorer restart recovery — L1809); tooltip localized: `Suborno - <enabled/disabled> - <layout>`।
- **Left click / double click / NIN_SELECT:** topbar দেখায়; **right click / WM_CONTEXTMENU:** tray menu; `SetForegroundWindow` + `PostMessage(WM_NULL)` — stale-menu সমস্যার মানসক প্যাটার্ন ঠিক আছে।
- **Dark/Light taskbar contrast, sharpness, High-DPI icon rendering** — runtime observation **BLOCKED** (icon asset আছে: `resources/suborno.ico`; multi-size PNG assets Linux packaging-এ আছে)।

## 12. Context Menu Audit

`show_tray_menu_at()` — tray right-click ও topbar right-click **একই builder** ব্যবহার করে। বর্তমান গঠন: Show Topbar (checkmark state সহ) → Keyboard Layout ▸ (7 radio) → Font ▸ (System Default + families, active-তে checkmark) → separator → On-Screen Keyboard… → Settings… → Enable/Disable Suborno in Windows (live state label) → Check for Updates… → Documentation → Clear Learnt Words… → Uninstall Suborno → separator → Exit।

- প্রতিটি item-এর real handler `handle_menu_command()`-এ (L3899–3945) — dead menu item নেই (legacy `kMenuOpen` id dispatcher-এ compat case; কোনো builder আর emit করে না)।
- Label সব `ui::text`-ভিত্তিক (bn/en); **keyboard:** native menu model; **Accessibility Name:** native HMENU accessibility। Runtime visual **BLOCKED**।

## 13. Dialog Audit

- **Error dialogs:** `show_error()` — message + localized `error.code_line` ("ত্রুটি: {code}") + `MB_ICONERROR`, title = `common.app_name`। 24টি `error.*` string দুই locale-এই (N-01 audit-এ 0 missing প্রমাণিত)। Pre-locale-load failure-এ (`kSettingsFailureMessage`) ইচ্ছাকৃত English — code-এ documented (locale table তখনো load হয়নি)।
- **Confirmation:** Clear Learnt Words → `MB_YESNO`, localized title/text, **default button No** — accidental destructive action প্রতিরোধ সঠিক।
- **About dialog:** frameless 560×430, 5 hotspot (Updates/Documentation/License/Notices/Close), Tab order + Space/Enter + Esc, single-instance focus-restore, DPI-fitted (`dpi_fitting`)।
- Text wrapping/size/button labels — Source-traced; **runtime visual BLOCKED**। Mixed-script corruption কোনো dialog string-এ পাইনি (EV-002-এ bn মানগুলোর script check)।

## 14. Notification Audit

Toast (`SubornoToastWindow`): frameless `WS_EX_NOACTIVATE | WS_EX_TOPMOST` tool window, tray-এর উপরে position (work-area clamp, `MonitorFromWindow` nearest), **2.6 s auto-dismiss timer** (kill ও renew সঠিক), SW_SHOWNOACTIVATE — typing/TSF composition কখনো ভাঙে না। 9টি event: settings saved, language changed, layout switched, font changed, encoding unicode/bijoy, profile enabled/disabled, learnt cleared — সব localized + `{placeholder}` expansion। Status color (success/information/warning) **সবসময় text label-সহ** — color-only indicator নেই। DPI change-এ reposition (L239–251)। Focus impact: নেই (noactivate)। Overlap: নতুন toast আগেরটি replace করে। **Runtime visual/position observation BLOCKED।** Error দশমতে toast ব্যবহার হয় না — native MessageBox (documented design decision)।

## 15. Bilingual UI Audit

- **Parity (executed):** en/bn **প্রতিটি 160 keys, 0 missing, 0 placeholder mismatch** (EV-002)। সব Label/Button/Menu/Dialog/Toast/Tooltip/Status/About — একই key-set দুই ভাষায়।
- **বাংলা Text গঠন (executed script check):** বাংলা মানের ভেতরে mixed-script token 0টি প্রকৃত সমস্যা ("Caps Lock", "English", "Shift", "bn-BD", "Ctrl+Alt+E" — অনুমোদিত technical term; `{layout}।`-এর daari placeholder-সহ — সঠিক)। যুক্তবর্ণ/কার/রেফ ইত্যাদি UI string-গুলোতে ভাঙার মতো কিছু পাইনি — তবে **বাস্তব glyph shaping দেখা যায়নি (BLOCKED)**।
- **Language switching:** Settings → Interface language dropdown (system/English/বাংলা) → Save-এ `apply_ui_language()` → locale table reload + cache clear + সব খোলা window refresh; change হলে toast "ইন্টারফেসের ভাষা বদলে গেছে"। "system" মানে Windows UI language (`GetUserDefaultUILanguage` == বাংলা) অনুসরণ। Restart-এ persisted মান থেকে পুনরায় load — logic executed tests-এ কভার (ui_strings round-trip, shipped-files parity test)।
- **Translation quality (মানব পর্যবেক্ষণ):** অনুবাদগুলো স্বাভাবিক বাংলায়; terminology সুসংগত ("সংরক্ষণ/বাতিল", "ক্যান্ডিডেট উইন্ডো", "লেআউট"); কোনো placeholder হারায়নি। ছোট style-নোট: error strings-এ কিছু জায়গায় passive ("লোড করা যায়নি") আর কিছু জায়গায় active ("করতে পারেনি") কাঠামো — অর্থ ঠিক, অসামঞ্জস্য নয়; কোনো অনুবাদ পরিবর্তন করা হয়নি।
- **Missing translation:** 0। **Mixed-script corruption:** 0।

## 16. Typography Audit

- Font model: `create_message_font()` (OS message font + configurable family) — সব পৃষ্ঠ একই family; heading/title/caption তিন স্তর, সব DPI-scaled। OS Default Bangla Font checkbox ও Font dropdown (Bangla-capable family enumeration: Bengali/Bangla/Siliguri/Ekushey/Kalpurush/Solaiman… keyword ভিত্তিক) পুরো app + candidate + OSK-তে apply হয় (`ui_font_family()` + `refresh_font()`)।
- Baseline/alignment: `DT_VCENTER | DT_SINGLELINE` মেকানিজম প্রতিটি row-এ; checkbox 24 / button-dropdown 34 / menu-item 28 মানগুলো সুসংগত (Phase-01 fix)।
- Truncation/ellipsis: menu + candidate-এ `DT_END_ELLIPSIS`; section description `DT_WORDBREAK`; topbar pill auto-width দীর্ঘ বাংলা নাম ধরে (clamp 150–360)।
- **বাস্তব Bengali glyph shaping, fallback chain, clipping** — **BLOCKED** (rendering পর্দায় দেখা যায়নি)। Font পরিবর্তন করা হয়নি।

## 17. Visual Consistency Audit

- **Token discipline:** প্রতিটি surface `theme_*()` accessor দিয়ে আঁকে — hardcoded color literal নেই (HC/system theme-ও এক path-এ আসে)।
- **Executed metric evidence:** contrast recompute (EV-003): dark `text` 13.97:1, `text_inactive` 4.72:1 (0x969696 — Phase-01-এর 4.15:1 fix), light `text` 15.12:1, `on_accent`/`accent_fill` 4.61–4.63:1 — **AA floors PASS**। Sub-floor 2 pair (`text_inactive` × `surface_raised`/`surface_hover`) — **বর্তমান consumer 0টি** (4 consumer-ই `surface`-এ আঁকে: About copyright L901, section hint L3478, status line L3588, disabled menu item menu_popup L230) — documented pairing rule মেনে চলছে; ভবিষ্যতে ওই pair ব্যবহার হলে token #A6A6A6-তে উঠানোর নির্দেশনা code comment-এই আছে।
- Component geometry শেয়ার্ড (button=dropdown=34)। **বাস্তব পর্দায় cross-window visual consistency (spacing চোখে, alignment) — BLOCKED।**

## 18. DPI Audit

- **Mechanism (source-verified):** `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)` → fallback SYSTEM_DPI_AWARE → `shcore SetProcessDpiAwareness` → `SetProcessDpiAware`। পাঁচটি window-ই `WM_DPICHANGED` handle করে (settings L1908, about L1061, topbar L798, toast L239, osk L683) — font recreate, suggested rect-এ SetWindowPos, rounded region re-cut।
- `dpi_fitting()` — ছোট work-area-তে (768 px laptop @125%) fixed design overflow আটকায়।
- **Manifest gap (N-05):** কোনো embedded application manifest নেই — DPI declaration শুধু runtime call; এটি valid মেকানিজম, তবে manifest-embedded declaration আরও নিরাপদ।
- **Runtime PASS দাবি করা হয়নি:** 100/125/150/175/200% scaling, multi-monitor transition, tray icon rendering — সব **BLOCKED** (Manual-Test-Protocol §4-এর 4 step-এই এগুলো যাচাইয়ের নির্ধারিত পথ)।

## 19. Accessibility Audit

পূর্ণ তালিকা `ACCESSIBILITY_AUDIT.csv` (23 rows):

- **UIA (source-verified, 6/6 custom surface):** IRawElementProviderSimple/Fragment/FragmentRoot; AutomationId scheme — `sidebar.*`, `action.*` (29 semantic action), `zone.*` ×4, `osk.*`, `toast.message`, `menu.item.<n>`; patterns Invoke/Toggle/ExpandCollapse/RangeValue; HelpText = tooltip text; IsEnabled/IsOffscreen/focusable provider-এ expose হয়; focus change-এ `notify_focus_changed`। Stale-provider safety (UIA_E_ELEMENTNOTAVAILABLE) + exception barriers।
- **Keyboard:** Settings-এ সম্পূর্ণ cycle (Tab/Shift+Tab section/status skip করে, Ctrl+Tab page, Space action, slider 6 key, Esc=Cancel/Enter=Save); About 5 hotspot; menu popup arrow/Enter/Esc + keystroke routing; tray native। সব logic-স্তরে traced — **কিন্তু focus ring বাস্তবে দেখা BLOCKED**।
- **Screen Reader (Narrator):** **সম্পূর্ণ BLOCKED** — Manual-Test-Protocol §1-এর 8 step ছাড়া উপায় নেই।
- **Visual accessibility:** contrast metric executed PASS (উপরে); High Contrast palette + live repaint mechanism আছে (BLOCKED runtime); color-only indicator নেই; icon-only control-এ tooltip+HelpText দুটোই আছে।
- **Compliance দাবি:** এই Audit কোনো WCAG/Narrator certification দাবি করে না — যথেষ্ট runtime প্রমাণ নেই।

## 20. Keyboard Navigation Audit

Workflow-ভিত্তিক: Settings পুরোনো (F1-era) থেকে সম্পূর্ণ বদলেছে — এখন প্রতিটি control keyboard-reachable, focus order নথিভুক্ত (page controls → Save → Cancel), footer-এ Enter/Space activate। Dropdown-এ Space=open, arrow navigation menu popup-এ। Hotkey: layout-cycle global, Ctrl+Alt+E encoding toggle (disable checkbox সহ)। OSK Esc-only (pointer tool হিসেবে ইচ্ছাকৃত)। Toast কখনো focus নেয় না। **প্রতিটি বাস্তব keypress আচরণ runtime-এ BLOCKED**, তবে handler-গুলোর সম্পূর্ণ traced path আছে এবং Settings-এর staged-state logic 557-suite-এ কভার।

## 21. UI Automation Results

- **Existing harness:** `Invoke-SubornoUiAudit.ps1` (333 lines, PS 5.1, in-box .NET UIA; UIA-Invoke ও real mouse click আলাদা check — Invoke-এর PASS mouse-PASS-এর প্রমাণ নয়, সঠিক নিয়ম)।
- **Check count (এই Audit-এ 1:1 enumerate করা):** **84 happy-path check ID** — SET-OPEN(1), NAV×8, CHK×32, DD×20, SLD×8, Save/Cancel×6, TOAST(1), ABT(1), OSK(1), TOP×5। Brief-এর "85 checks" সংখ্যাটি Repository-র কোথাও নেই — canonical count **84** (N-06)।
- **Sandbox-এ চালানো গেছে: 0 / 84 — সব BLOCKED** (Windows + built exe লাগবে)।
- **Infrastructure ফাঁক (নতুন):** শেষ লাইনের merge hint ও README-র merge step-এ উল্লিখিত `merge_audit_results.py` ও `checkmap.json` ছিল না (N-02); README কিছু অতিরিক্ত দাবি করে (N-03)। এই Audit-এর fix pass-এ দুটি ফাইল যোগ করা হয়েছে ও merge path synthetic results দিয়ে পরীক্ষা করা হয়েছে (Section 26-এর পরের Fix Pass নোট)।
- **UIA unit-level coverage:** `tests/unit/ui_strings_test.cpp` (24 tests) executed PASS; `tests/windows/windows_build_smoke_test.cpp`-এর 9 test (TSF contract + CandidateListUIElement) Windows runner-এ চলে (sandbox-এ BLOCKED)।

## 22. User Workflow Audit

Brief-এর Workflow A–F বর্তমান Source মডেল-এ যাচাই:

- **A — Application Start:** `run()` → settings load/quarantine → theme apply → hotkeys → `apply_start_mode()` (topbar/tray/window/last-state; sign-in launch-এ settings window ইচ্ছাকৃতভাবে আসে না) → tray add → second-instance mutex (`Local\Suborno.Desktop.Application`, দ্বিতীয় launch-এ topbar show message)। Logic PASS; **বাস্তব launch BLOCKED।**
- **B — Layout Change:** Section 10 — সব পথে engine-first persist; feedback: topbar label + status line + toast। **BLOCKED (runtime)।**
- **C — Settings Change:** staged → Save (atomic, rollback) → reopen-এ restored; Cancel non-persisting (by construction); 557-suite-এ round-trip/quarantine/atomic executed PASS; **বাস্তব reopen BLOCKED।**
- **D — Language Change:** Section 15 — bn/en/system switching logic সম্পূর্ণ; **বাস্তব UI refresh BLOCKED।**
- **E — Tray Usage:** icon → menu → layout/settings/topbar/Exit সব handler-যুক্ত; Explorer restart re-add mechanism আছে; **বাস্তব ক্লিক BLOCKED।**
- **F — Error Recovery:** নিরাপদ নেতিবাচক দৃশ্য (corrupt settings quarantine, save-failure rollback + localized error, OSK create failure-এ localized error, missing locale file → English fallback) Source-স্তরে সব present; অনেকগুলোই engine-suite-এ executed; **বাস্তব dialog দেখা BLOCKED।**

## 23. Functional Verification Matrix

`UI_UX_FUNCTIONAL_VISUAL_MATRIX.csv` — **133 rows** (129 carried-forward + 4 new: SET-LANG-LBL-01, INST-UI-01, INST-UI-02, CI-RUNTIME-01)। প্রতি row-তে Expected/Actual/Handler-evidence/UIA id/Functional/Visual/Evidence। Count (স্ক্রিপ্ট-হিসাব):

| Functional | Count | | Visual | Count |
|---|---:|---|---|---:|
| PASS | 24 | | PASS | 1 |
| FAIL | 0 | | ISSUE FOUND | 0 |
| BLOCKED | 95 | | BLOCKED | 95 |
| NOT TESTED | 0 | | NOT TESTED | 23 |
| NOT APPLICABLE | 14 | | NOT APPLICABLE | 14 |

24 PASS-এর প্রতিটির evidence এই Audit-এ পুনঃনিশ্চিত (engine corpus 294/294, probe 36/36, paragraph 18/18, switch-chain 7/7, 557-suite, i18n recompute); **কোনো row-কে evidence ছাড়া উন্নীত করা হয়নি।**

## 24. Visual Quality Assessment

| Category | Observation (Source-level) | Evidence | Strength | Weakness | User Impact | Recommendation |
|---|---|---|---|---|---|---|
| Visual Consistency | একক token system, সব surface এক accessor | ui_theme.hpp | theme/HC switch সর্বত্র একসাথে | runtime যাচাই নেই | — | Windows-এ Report-06 protocol চালান |
| Typography | 3-tier scale, configurable Bangla font | ui_theme L415-466 | সুসংগত | shaping runtime BLOCKED | — | Screenshot evidence নিন |
| Spacing | 4/8 grid + shared metrics | ui_theme L257-279 | মিস-অ্যালাইনমেন্টের সুযোগ কম | — | — | — |
| Hierarchy | section+hint মডেল, sidebar grouping | build_pages | স্পষ্ট grouping | ৩ dropdown unlabeled (N-04) | discoverability সামান্য কম | caption যোগ করুন |
| Color | AA recompute PASS | EV-003 | WCAG-সামঞ্জস্য মেকানিজম | পর্দায় দেখা নেই | — | — |
| Iconography | hand-drawn Fluent-convention glyphs (round pen) | topbar.cpp | ধারাবাহিক | বাস্তব sharpness BLOCKED | — | — |
| Navigation | sidebar + tray + topbar + hotkey | — | বহু পথ | Help window নেই | খুঁজে পাওয়া সহজ | — |
| Discoverability | section hint ভালো | — | — | language/font dropdown caption নেই (N-04) | Low | caption যোগ |
| Feedback | toast 9 event + status line + menu checkmark | — | দ্বিগুণ feedback | — | — | — |
| Accessibility | UIA 6/6 + keyboard | ACC CSV | ব্যাপক | Narrator BLOCKED | — | Manual protocol §1 |
| Localization | 160=160 parity | EV-002 | পূর্ণ parity | installer bn নেই (N-05) | Low | bn.isl |
| DPI | PMv2 + fitting + 5×WM_DPICHANGED | — | মেকানিজম পূর্ণ | runtime BLOCKED; manifest নেই (N-05) | — | manifest embed |
| Error UX | localized + HRESULT + rollback | — | পরিষ্কার action | — | — | — |
| Usability | staged settings, immediate tray actions | — | নিরাপদ মডেল | runtime usability test নেই | — | Manual protocol |

নিজের পছন্দ Technical Finding হিসেবে উপস্থাপন করা হয়নি — প্রতিটি সারি Source/evidence-ভিত্তিক।

## 25. Existing Issues

বর্তমানে খোলা সমস্যা (Fix করা হয়নি, Report করা হয়েছে):

1. **N-04 (Low):** language/font dropdown unlabeled + unused key `settings.interface.language` + UIA fallback-এ ৩টি "Appearance"।
2. **N-01 (Medium):** cited evidence scripts repo-তে ছিল না — reproducibility ভাঙা (Fix pass-এ যোগ করা হয়েছে, নিচে দ্রষ্টব্য)।
3. **N-02 (Medium):** automation merge pipeline broken (Fix pass-এ সম্পূর্ণ হয়েছে)।
4. **N-03 (Low):** README overstatement (Fix pass-এ সংশোধিত)।
5. **N-05 (Informational):** manifest অনুপস্থিত; installer English-only; dead `kMenuOpen` id; `meta.*` keys (by-design metadata)।
6. **Khipro trailing virama (engine স্তরের KNOWN LIMITATION):** `ক্` standalone টাইপ করার কোনো key নেই — inventory 401/402 (NOT_DERIVABLE 1) — layout-verification report-এ documented।

**সতর্কতা:** 1–4-এর fix pass এই mission-এর পরের ধাপে সম্পন্ন হয়েছে (User-এর "full fix production ready zip" নির্দেশ); তবে সেগুলো UI/Source নয় — evidence tooling ও docs। UI পরিবর্তন (N-04, N-05) **করা হয়নি** — Recommendation হিসেবেই থাকল।

## 26. Previously Fixed Findings Reconciliation

পুরোনো Report-এর প্রতিটি Fixed claim বর্তমান Implementation-এ যাচাই (পূর্ণ সারণি `UI_UX_FINDINGS.csv`, 35 rows):

| Previous claim | Current status at f8f33d2 | Evidence |
|---|---|---|
| BUG-01..BUG-06 (Reports 08/FINAL) | **VERIFIED FIXED ×6** (re-executed) | EV-001/002/006 + grep + 557-suite regression names |
| F-1..F-5 (typing defects) | **VERIFIED FIXED ×5** (re-executed) | corpus 49/49 প্রতি layout, probe 36/36 |
| F1–F7 (পুরোনো settings audit) | **VERIFIED FIXED ×7** (source re-check; F4 = redesign-এ superseded — বর্তমান window non-resizable) | keyboard/font/DPI/ellipsis/clamp code |
| Phase-01: 3 High + 6 Medium + 4 Low | **VERIFIED FIXED ×13** | slider math, 0x969696, About keyboard, UIA, 34=34, OSK font/region, hover, themed border, fill_rounded, popup clamp, kMenuOpen, brush churn |
| পুরোনো "84 interactive BLOCKED" | **CARRIED BLOCKED** — আজও runtime ছাড়া PASS হয় না | execution-summary.json |

- **VERIFIED FIXED: 31 claims** · STILL PRESENT: 0 · REGRESSION: 0 · PARTIALLY FIXED: 0 · BLOCKED (reconciliation-নিজস্ব): runtime rows carried।
- **নিয়ম পালিত:** শুধু পুরোনো রিপোর্টে FIXED লেখা থাকাকে VERIFIED FIXED ধরা হয়নি — প্রতিটির জন্য বর্তমান Source + আজকের executed evidence দেখা হয়েছে।

**Fix Pass নোট (এই Audit প্যাকেজের অংশ, পরবর্তী ধাপে):** N-01 → `scripts/i18n_key_check.py` + `scripts/contrast_check.py` repo-তে ফিরিয়ে আনা হয়েছে (আজকের executed output এই ফোল্ডারের `evidence/`-তে); N-02 → `scripts/layout_verify/merge_audit_results.py` + `scripts/windows-ui-audit/functional/checkmap.json` যোগ + synthetic-merge test; N-03 → README সংশোধন। UI/Backend কোনো ফাইল স্পর্ষ হয়নি; fix pass-এর পর 557/557+2/2 পুনঃনিশ্চিত।

## 27. New Findings

এই Audit-এ প্রথমবার ধরা পড়া (পূর্ণ field: `UI_UX_FINDINGS.csv`):

- **N-01 (Medium, Evidence/tooling):** Reports 05/08/FINAL-এর উল্লিখিত `scripts/i18n_key_check.py`, `scripts/contrast_check.py` গাছে ও git history-তেই নেই — পুরোনো evidence পুনঃপ্রস্তুত করা যেত না। (Fix pass: দুটি স্ক্রিপ্ট যোগ করা হয়েছে, আজকের output সহ।)
- **N-02 (Medium, Automation tooling):** `merge_audit_results.py` + `checkmap.json` অনুপস্থিত — Windows run-এর 84-check result matrix-এ মেশানো যেত না। (Fix pass: যোগ + পরীক্ষিত।)
- **N-03 (Low, Docs):** `windows-ui-audit/README.md` script-এ নেই এমন ৫টি আচরণ দাবি করে। (Fix pass: সংশোধিত।)
- **N-04 (Low, UX/UIA):** Interface page-এর language/font dropdown-এ দৃশ্যমান caption নেই; `settings.interface.language` key অব্যবহৃত; UIA fallback name ৩ dropdown-এর জন্য এক। **Fix করা হয়নি** (layout reflow + Windows visual verification লাগবে) — Recommendation রেকর্ড হয়েছে।
- **N-05 (Informational):** embedded manifest নেই (DPI runtime-only); InnoSetup wizard English-only; dead id `kMenuOpen` dispatcher-এ।
- **N-06 (Informational):** Brief-এর "85 checks" → বাস্তবে 84; `UI_UX_FINDINGS_FULLY_APPLIED.md` নেই।
- **N-07 (Blocker-class):** সব Windows-runtime evidence আজও BLOCKED — এটাই প্রধান বাকি-কাজ।

## 28. Severity Summary

স্ক্রিপ্ট-হিসাব (`UI_UX_FINDINGS.csv` থেকে, 35 findings):

| Severity | Findings |
|---|---:|
| Critical | 4 |
| High | 6 |
| Medium | 15 |
| Low | 7 |
| Informational | 2 |
| Blocker-class | 1 |

সব Critical/High/Medium ঐতিহাসিক finding **সমাধান-যাচাইসহ** তালিকাভুক্ত (VERIFIED FIXED); বর্তমানে **খোলা Functional/Visual defect = 0** (যা runtime ছাড়া যাচাইযোগ্য); খোলা সমস্যা = tooling/docs/informational (N-01..N-06) + runtime-evidence blocker (N-07)।

## 29. Improvement Recommendations

(Recommendation মাত্র — এই Mission-এ implement করা হয়নি)

1. **Windows Runtime Evidence Campaign** (সর্বোচ্চ মূল্য): MSVC build → `Invoke-SubornoUiAudit.ps1` (84 checks) → `Manual-Test-Protocol.md` (35 steps) → screenshots সহ matrix merge — এই Audit-এর fix pass করা merge pipeline দিয়ে। এতে 95 BLOCKED row বাস্তব PASS/FAIL-এ যাবে।
2. **N-04 fix (UI change হিসেবে, আলাদা অনুমোদনে):** Interface page-এ language/font dropdown-এর caption row + `settings.interface.language` key ব্যবহার; UIA name-ও caption থেকে নেওয়া।
3. **Embedded manifest** (`dpiAware PerMonitorV2`, `requestedExecutionLevel asInvoker`) — MSVC build change + CI-তে যাচাই।
4. **Installer localization** — InnoSetup বাংলা .isl available হলে যোগ।
5. **Narrator সেশন** — Manual protocol §1-এর 8 step + Accessibility Insights FastPass, evidence সহ।
6. **Help Window/কেন্দ্রীয় help surface** (ঐচ্ছিক) — বর্তমানে help বিক্ষিপ্ত (hint text, About)।

## 30. Remaining Blockers

| Blocker | কী আটকায় | উন্মুক্তির পথ |
|---|---|---|
| Windows desktop + MSVC runtime | 95 BLOCKED rows, 20 screenshot slots, Narrator, DPI, installer | Windows 10/11 machine বা CI runner; harness + manual protocol প্রস্তুত |
| বাস্তব MSVC compile confirmation (fix commits `993968c`/`5724528`/`c1462f4` + BUG-05) | "compiles on MSVC" চূড়ান্ত দাবি | `ci.yml windows-msvc` job সবুজ হলেই বন্ধ হবে |
| UIA live tree / Narrator | Accessibility compliance দাবি | Manual protocol §1 |
| Khipro trailing virama (known limitation) | inventory 401/402 | engine-স্তরের নকশা-সিদ্ধান্ত (spec অনুযায়ী bug-vs-design নথিভুক্ত) |

## 31. Evidence Index

সম্পূর্ণ সূচি: `EVIDENCE_INDEX.md` (EV-001…EV-008), `evidence/i18n_key_check_f8f33d2.txt`, `evidence/contrast_check_f8f33d2.txt`, ডেটা ফাইল: `UI_UX_FUNCTIONAL_VISUAL_MATRIX.csv`, `UI_INVENTORY.csv`, `ACCESSIBILITY_AUDIT.csv`, `SCREENSHOT_EVIDENCE_INDEX.csv` (20 slots — সব BLOCKED), `UI_UX_FINDINGS.csv`, `execution-summary.json`। কোনো Screenshot নেই — **কোনোটাই fabricate করা হয়নি; সব slot BLOCKED।**

## 32. Final UI/UX Verdict

**A. Visual Quality** — Source-স্তরে সুসংগত ও পেশাদার মানের সিস্টেম (একক token model, AA contrast recompute PASS, shared geometry, ellipsis/clamp সর্বত্র)। কিন্তু বাস্তব পর্দায় দেখা যায়নি → চূড়ান্ত দৃশ্যমান মানের দাবি **runtime evidence ছাড়া সম্ভব নয়**। Rating: **INSUFFICIENT EVIDENCE (runtime) — source-level design: strong**।

**B. Functional Quality** — সব Control-এর backend mapping, staged model, persistence, rollback, immediate actions — সম্পূর্ণ traced + 557/557+2/2 executed PASS; 84-check harness প্রস্তুত। বাস্তব click/toggle **BLOCKED**। Rating: **ENGINE/LOGIC VERIFIED — WINDOWS RUNTIME BLOCKED**।

**C. Accessibility Quality** — UIA providers 6/6 surface, semantic AutomationId, 4 pattern, keyboard models সম্পূর্ণ, HC palette — Source-স্তরে ব্যাপক ও সঠিক; Narrator/live UIA/focus-visibility **BLOCKED**। Rating: **IMPLEMENTATION COMPLETE — RUNTIME VERIFICATION BLOCKED**।

**D. Overall UX Quality** — ব্যবহারকারীর পথগুলো (start, switch, settings, language, tray, error) নকশায় সংক্ষিপ্ত ও feedback-সমৃদ্ধ; discoverability-তে একটি Low finding (N-04); বাস্তব ব্যবহারযোগ্যতার প্রমাণ runtime-এ বাকি। Rating: **NEEDS IMPROVEMENT (evidence-gated)**।

### Final Overall Verdict: **NEEDS IMPROVEMENT**

- কারণ: (১) সব গুরুত্বপূর্ণ গুণ-স্তর Logic/Engine-এ প্রমাণসহ সবুজ, কিন্তু (২) Windows Runtime স্তরে **কোনো বাস্তব evidence নেই** (95 BLOCKED, 0 screenshot), এবং (৩) tooling/docs-স্তরের কিছু ফাঁক (N-01..N-03 — এই Audit-এর fix pass-এ বন্ধ) পাওয়া গেছে।
- এটি Formal Certification নয়; Audit Findings-এর ভিত্তিতে বর্তমান অবস্থার মূল্যায়ন। **কোনো BLOCKED item PASS করা হয়নি; কোনো Finding লুকানো হয়নি।**
- Production-এর দরজা পরিষ্কারভাবে জানা: Section 30-এর ৪ blocker — বিশেষত Windows Runtime Evidence Campaign + MSVC CI confirmation।
