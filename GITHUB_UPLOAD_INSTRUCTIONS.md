# GitHub-এ Upload করার নির্দেশনা (Suborno IME — verification zip)

এই zip-এ **সম্পূর্ণ repository** আছে — সোর্স কোড + সব verification evidence +
Windows verification workflow + **.git history সহ** (branch
`ci/suborno-v19-windows-verification`, commit `6de4291`)।

## পদ্ধতি ১ — Git CLI দিয়ে (সুপারিশকৃত, branch ঠিক থাকে)

```bash
# 1. zip আনজিপ করুন
unzip subornoime_v21_verification.zip -d subornoime
cd subornoime

# 2. আপনার repo-টা remote হিসেবে যোগ করুন (origin আগেই সেট করা আছে —
#    নিজের URL-এ পয়েন্ট করে নিন)
git remote set-url origin https://github.com/jubayerhossaindev/subornoime.git

# 3. Verification branch-টা push করুন (main-এ কিছু যাবে না)
git push -u origin ci/suborno-v19-windows-verification

# 4. (ঐচ্ছিক) পুরো হিস্ট্রি main-এও রাখতে চাইলে:
git push origin ci/suborno-v19-windows-verification:main
```

Push করার সাথে সাথেই `.github/workflows/suborno-windows-verification.yml`
workflow টা **স্বয়ংক্রিয়ভাবে চালু হবে** (push trigger এই branch-এর জন্য
কনফিগার করা)। GitHub → **Actions** ট্যাবে গিয়ে "Windows Verification" run
দেখুন।

নিজে চালাতে চাইলে: Actions → Windows Verification → **Run workflow**
(workflow_dispatch সাপোর্ট করে)।

### Authentication

Push-এর সময় GitHub লগইন চাইবে:
- Personal Access Token ব্যবহার করুন (Settings → Developer settings →
  Personal access tokens → `repo` + `workflow` scope)
- অথবা `gh auth login` দিয়ে GitHub CLI লগইন করুন

## পদ্ধতি ২ — GitHub web upload (branch ছাড়া, সরাসরি main)

GitHub-এ repo খুলে → **Add file → Upload files** → zip-এর **ভেতরের ফাইলগুলো**
(zip নিজে নয়) drag করে commit করুন। তারপর Actions → Windows Verification →
**Run workflow** চাপুন (push trigger শুধু `ci/suborno-v19-windows-verification`
branch-এ কাজ করে, কিন্তু workflow_dispatch যেকোনো branch থেকে চলে)।

## এই zip-এ কী আছে

| Path | বিবরণ |
|---|---|
| `src/`, `include/`, `tests/`, `packaging/`, `scripts/` | সম্পূর্ণ v19 সোর্স + টেস্ট + ইনস্টলার |
| `.github/workflows/suborno-windows-verification.yml` | Windows verification pipeline (MSVC v143, /analyze, dumpbin ABI, installer test, ৯৮-check UIA audit) |
| `reports/LINUX_PART_III_VERIFICATION_REPORT.md` | Part III Linux verification — চূড়ান্ত রিপোর্ট |
| `reports/WINDOWS_GITHUB_VERIFICATION_REPORT.md` | Windows/GitHub verification — চূড়ান্ত রিপোর্ট |
| `reports/linux_verification/part_iii/` | ১২টি section রিপোর্ট + ১৫টি execution log (সব কমান্ড + exit code সহ) |
| `reports/windows_verification/` | ১৩টি রিপোর্ট + gui/ evidence tree + blocker প্রমাণ |
| `.git/` | সম্পূর্ণ commit history (v19 refactor → v20 → verification wave) |

## Verification ফলাফল এক নজরে

- **৬৮৬/৬৮৬ unit test** PASS (Release), ctest ৩/৩ suite
- **ASan+UBSan: ৩৪/৩৪ Part III test** PASS (leak detection on)
- **TSan: DiagnosticsTest ৭/৭** PASS
- **৪টা defect ধরা পড়ে ঠিক হয়েছে** (D-001 benchmark wiring, D-002 format,
  D-003 flaky timing fence, D-004 test count)
- Windows runtime (MSVC/TSF/installer/UIA) — workflow দিয়ে GitHub runner-এ
  চলবে; upload করার পর সেগুলোর evidence স্বয়ংক্রিয়ভাবে জমা হবে
