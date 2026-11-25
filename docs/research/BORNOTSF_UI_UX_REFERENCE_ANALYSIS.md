# BornoTSF UI/UX Reference Analysis

Date: 2026-11-21
Researcher: Principal Windows Desktop UI/UX study for SubornoIME

## 1. Source Research

### 1.1 Original repository

- URL: https://github.com/JayedAhsan/BornoTSF (inspected via web fetch)
- Purpose: open-source Bangla Input Method Editor for Windows built on the
  Text Services Framework; runs as a native COM DLL (`BornoTSF.dll`)
  registered into the OS input architecture.
- README states it deliberately works **without a background executable or
  heavy GUI**: no topbar, no settings dialog, no on-screen keyboard, no
  theme engine. The only user-facing UI is a **system tray menu** ("Quick
  options are available in the system tray menu") that opens keymap
  reference pages for phonetic/fixed layouts.
- Technology: C++ (MSVC, Visual Studio 2022, MSBuild x64+x86), Windows
  10/11 SDK, Boost (regex for layout parsing). Top-level structure:
  `assets/`, `composition/`, `core/`, `display/`, `infra/`,
  `layoutParsers/`, `storage/` plus `DllMain.cpp`, `Register.cpp`,
  `BornoTSF.def`, `BornoTSF.rc`. No submodules.
- Layouts: Borno Phonetic, Avro Phonetic, Khipro (compositional), National
  (Jatiyo), Probhat (fixed layouts marked "Tested (WIP)").
- License: **GNU GPL v3.0**.

### 1.2 Critical finding: repository ≠ screenshots

The supplied screenshots show a full desktop application: a floating
desktop "topbar" widget, a sidebar settings dialog with theme/opacity
sliders, an on-screen keyboard, a license manager, a SyncType voice-typing
QR dialog, and an About dialog branded "Codepotro". **None of these UI
surfaces exist in the JayedAhsan/BornoTSF repository** — they belong to the
full retail "Borno" application distributed by Codepotro (version
2026.6.18 in screenshots), whose application shell is not in that
repository. The repository contains only the TSF engine and its
candidate/composition display layer (`display/`).

Consequence (per the task's own rule): the screenshots are treated as the
primary visual specification; the repository was inspected independently
to document architecture, behavior vocabulary (layout names, tray menu
concepts) and licensing. No GPL code was needed or used.

### 1.3 Architecture findings relevant to SubornoIME

- Same integration model as SubornoIME: TSF COM DLL registered via
  regsvr32; engine + display separated from any shell UI.
- Borno's retail shell (screenshots) is a custom-drawn Qt-style dark UI
  (uniform #2C2C2C surfaces, rounded corners, pill-shaped selection
  highlights, white Segoe UI text). Custom-drawn controls rather than
  native light-theme Win32 controls.
- Behavior vocabulary shared with SubornoIME: phonetic layouts (Borno/
  Avro phonetic), fixed layouts (National/Probhat), numpad→Bangla digits,
  Caps Lock ignore, candidate selection with number keys, tray menu.

## 2. Reference ZIP Analysis

- URL given: https://tmpfiles.org/wcAVb0iF3dIZ/born-images.zip
  (landing page returned HTML; the real download link found inside the
  page — `https://tmpfiles.org/dl/1795294456.031f3671f4f4c454/wcAVb0iF3dIZ/born-images.zip`
  — was followed, as instructed).
- Download verification: 6,018,014 bytes; `file` reports
  "Zip archive data, made by v3.1". Valid ZIP confirmed. Original archive
  preserved unmodified at `work/reference/born-images.zip` (outside the
  repository); extracted once to an isolated directory
  `work/reference/images/born-images/`. No image was modified.

### 2.1 Image inventory (all 24 images)

| # | File | Dimensions (px) | Content |
|---|---|---|---|
| 1 | Screenshot_1.png | 522×414 | Installer wizard — Welcome page (shield logo, "Welcome to the Borno - A FREE Bangla IME Setup Wizard", Next/Cancel) |
| 2 | Screenshot_2.png | 534×436 | Installer — License Agreement page (EULA text, accept/decline radios, Back/Next disabled/Cancel) |
| 3 | Screenshot_3.png | 532×430 | Installer — Installing page (green progress bar, "Creating shortcuts...", Cancel) |
| 4 | Screenshot_4.png | 529×426 | Installer — Finish page ("Launch Borno" + "Visit Documentation" checkboxes, Finish) |
| 5 | Screenshot_5.png | 1080×728 | Desktop with floating **topbar pill** ("English" mode) highlighted by red annotation box; blue mode icon, wrench, ✕ |
| 6 | Screenshot_6.png | 1021×727 | Topbar pill ("Borno" mode) + **layout context menu**: Borno (radio•), Avro, Khipro, National ›, Encoding ›, Keyboard Layout Editor |
| 7 | Screenshot_7.png | 1086×722 | Topbar pill + **wrench menu**: Settings…, On-Screen Keyboard, Quick Theme ›, Suggestion Bar ›, Learning…, — Documentation, License Manager…, About Borno… |
| 8 | Screenshot_8.png | 736×625 | **Settings dialog — General page**: sidebar (General•/Interface/Topbar/Input), "Behavior": Remember Last Input Layout ✓, Start with Windows ✓; "Start as:" = Last State; "Switching Mode:" = Per Application/Window; Save/Cancel |
| 9 | Screenshot_9.png | 739×615 | Settings — **Interface page**: App Theme: Borno; Candidate Window: Show Candidate Window ✓, Enable Next Word Prediction ✓; Orientation: Vertical; Fixed Height ✓; Use OS Default Bangla Font ✓; Enable Sliding Effect ✓; Follow Mouse Pointer ✓; Remember Topbar Position ✓; Save/Cancel |
| 10 | Screenshot_10.png | 747×609 | Interface page with **App Theme dropdown open**: ✓ Borno, Borno Air, Borno Blue, Borno Classic, Borno Cream, Borno Glass, Borno Light, Borno Mist, Borno Pearl, Borno Silk |
| 11 | Screenshot_11.png | 753×618 | Settings — **Topbar page**: Topbar Visibility; Enable Idle Dimming ✓; Idle Opacity (%) slider = 50; Idle Time (Seconds) slider = 10; Save/Cancel |
| 12 | Screenshot_12.png | 738×616 | Settings — **Input page**: Layout Switching Shortcut: F10 (Default); Disable Encoding Shortcut ☐; Map Numpad to Bangla Numbers ✓; Change locale to dummy bn_BD… ✓; Ignore Caps Lock state in Bangla layouts ☐; Candidate Selection: Select candidate using number keys ✓; Save/Cancel |
| 13 | Screenshot_13.png | 743×616 | Input page with **shortcut dropdown open**: F1, F2, F3, F4, ✓F10 (Default), F12, Ctrl + Space, Alt + Shift, Win + Space |
| 14 | Screenshot_14.png | 882×649 | **On-Screen Keyboard** window: title "Borno On-Screen Keyboard", —/+/✕ controls, ENG / বাংলা toggle tabs, full QWERTY + Ctrl/Space/Ctrl, dark keys |
| 15 | Screenshot_15.png | 558×379 | Topbar wrench menu + **Quick Theme submenu** (radio): • Borno Default, Borno Air, Borno Blue, Borno Classic, Borno Cream, Borno Glass, Borno Light, Borno Mist, Borno Pearl, Borno Silk |
| 16 | Screenshot_16.png | 575×306 | Topbar wrench menu + **Suggestion Bar submenu**: ✓ Show Suggestion Bar, ✓ Next Word Prediction |
| 17 | Screenshot_17.png | 593×307 | Topbar wrench menu + **Learning… submenu**: Clear Learnt Words, Clear Candidate Choices |
| 18 | Screenshot_18.png | 523×437 | **License Manager dialog**: PC ID field + Copy ID, Enter Activation Key textarea, Register & Activate / Get License Key, "Status: Unregistered" |
| 19 | Screenshot_19.png | 549×439 | **About Borno dialog**: logo, "Borno", "A FREE Bangla IME", Version 2026.6.18, FREE, codepotro logo, © Codepotro. All Rights Reserved, EULA + Open Source Licenses buttons, "Check for Updates" pill |
| 20 | Screenshot_20.png | 528×431 | Same About dialog **with red annotation arrows** marking logo, title/version block, codepotro logo, © line, EULA + Open Source Licenses buttons (annotation of what to reproduce) |
| 21 | Screenshot_21.png | 375×543 | **SyncType - Voice Typing** dialog: "Open Borno SyncType and scan QR or enter IP: 104.4.2.2" + QR code |
| 22 | Screenshot_22.png | 240×136 | Topbar **✕ menu**: Switch to Tray, Exit Borno |
| 23 | Screenshot_23.png | 222×183 | System tray area with **Borno tray icon** highlighted (red arrow) |
| 24 | Screenshot_24.png | 383×310 | **Tray context menu**: Show Topbar, SyncType, Keyboard Layout ›, Encoding ›, Keyboard Layout Editor, — Suggestion Bar ›, Settings…, On-Screen Keyboard, License Manager…, — Exit |

### 2.2 Classification

- **Different windows of one application version** (retail Borno 2026.6.18,
  dark shell): 5–24. **Installer wizard pages** (Inno Setup, light theme):
  1–4. Image 20 is the same About dialog as 19 with annotation arrows
  (not a separate state). Images 10/13/15/16/17/22/24 are **open-menu
  states** of the windows in 9/12/7/24. No image shows a typing/candidate
  window, so the candidate-list visual language is NOT specified by the
  reference.

## 3. Measured visual system (programmatic pixel sampling)

Measured with PIL (`scripts/measure_reference.py`,
`measure_reference2.py`); "measured" = dominant pixel value of the region.

| Property | Value | Source |
|---|---|---|
| Window/page background (settings, OSK, About, menus) | `#2C2C2C` | measured (S8 100%, S19 95%, S14 86%) |
| Topbar pill background | `#2B2B2B` | measured (S5 86%) |
| Selected pill / active tab / button surface | `#383838` | measured (S8 sidebar 93%, S14 tab 84%) |
| OSK key surface / key border | `#4F4F4F` / `#3B3B3B` | measured (S14 97%) |
| Slider track / handle | `#272727` / `#FFFFFF` | measured (S11) |
| Primary text | `#FFFFFF` | measured (titles, labels, checkbox text) |
| Inactive tab text | `#8C8C8C` | measured (S14 বাংলা tab) |
| Accent (topbar mode icon) | `#5886FE` blue | measured (S5) |
| Installer progress bar green | `#6FC67E` | measured (S3) |
| Installer page background | `#FEFEFF` (light) | measured (S2) |
| Borders | no strong outlines; separators only in menus (subtle darker/lighter lines) | measured |
| Corner radius | dialogs/buttons ~8 px, topbar pill ~10 px, menus ~4 px | visual estimate (alpha channel unusable — screenshots contain desktop background) |
| Typography | Segoe UI (white), title bold ~16 px, body ~14 px, sidebar ~14 px, OSK keys ~14 px | visual identification (Windows shell font) |
| Topbar pill size | ~170×30 px incl. icon/text/wrench/✕, top-center of screen | measured (S5: x≈553–723, y≈8–38) |
| Settings window | ~736×620 px, sidebar ~185 px wide, content panel inset with rounded outline, Save/Cancel bottom-right | measured (S8/S9/S11/S12) |

Inferred (not directly measurable): hover states (no hover captured),
focus indicators (not captured), exact paddings inside menus (~12 px left
item inset), menu item height (~28 px).

## 4. Feature-by-feature reality check against SubornoIME

Features visible in the reference with **no backing functionality in
SubornoIME** (a fake control would violate the no-dead-UI rule):

| Reference feature | SubornoIME equivalent | Decision |
|---|---|---|
| App Theme dropdown (10 themes) / Quick Theme menu | none (settings.ui.theme_id exists but no theme engine renders it) | NOT IMPLEMENTED — no renderer |
| Next Word Prediction / Suggestion Bar | none | NOT IMPLEMENTED |
| On-Screen Keyboard | none | NOT IMPLEMENTED (large standalone feature) |
| License Manager / PC ID / activation | none (MIT-licensed free app) | NOT IMPLEMENTED — inapplicable |
| SyncType voice typing (QR/IP) | none | NOT IMPLEMENTED — no service |
| Learning… (clear learnt words) | none | NOT IMPLEMENTED — no learner |
| Encoding menu / Keyboard Layout Editor | none | NOT IMPLEMENTED |
| Check for Updates | none (no update server) | NOT IMPLEMENTED — would be fake |
| Orientation: Vertical/Fixed Height/Sliding Effect/Follow Mouse | candidate window is vertical-only fixed behavior | NOT IMPLEMENTED (no alternative exists to switch to) |
| Layout Switching Shortcut dropdown | no shortcut engine | NOT IMPLEMENTED |
| Remember Last Input Layout / Start as / Switching Mode | Suborno always restores its saved layout (= "Last State" semantics, not switchable) | NOT IMPLEMENTED as a control — single real behavior |
| **Floating topbar widget** | tray + settings window exist; topbar is implementable with real actions | **IMPLEMENTED** (new feature) |
| **Dark sidebar settings dialog + Save/Cancel** | settings window exists (immediate save) | **IMPLEMENTED** (restyled + staged save) |
| **About dialog** | About section inside settings window | **IMPLEMENTED** (dedicated dialog) |
| **Tray menu** | exists | **RESTYLED** to reference order |
| **Installer wizard** | exists (Inno Setup, LicenseFile + Launch checkbox already match reference pattern) | verified, minor label alignment |

## 5. Licensing assessment

- BornoTSF repository: GPL-3.0. SubornoIME: **MIT**. GPL→MIT transfer of
  source is not permitted; therefore **no source code, no logic, no
  assets were copied** from BornoTSF or from retail Borno. The
  implementation below is a clean-room Win32 implementation written from
  the measured visual specification in this document. Screenshots were
  used as factual visual references (colors, layout, behavior), which is
  permissible; ideas/layout are not copyrightable expression.
- The string "Borno" appears in SubornoIME only as a **layout name**
  ("Borno Phonetic") inherited from the established layout family naming
  (Avro/Borno phonetic are layout standards) — not branding.

Sources consulted: GitHub repository page (README, structure, license
badge), supplied reference ZIP images (all 24, individually).
