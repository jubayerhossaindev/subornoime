# Findings — English

- Passthrough identity সব ক্ষেত্রে PASS (hello/world/SubornoIME/Hello 2026!/mixed/backspace)।
- Engine-স্তরের ফলব্যাক সেশন (LayoutInputSession) snapshot-এ preedit/candidates দেখায়, কিন্তু TSF সার্ভিস passthrough kind-এর ক্ষেত্রে কি কখনো intercept করে না (text_service.cpp:325,597,678) — বাস্তব Windows-এ হোস্ট অ্যাপ নিজেই কি গ্রহণ করে।
- বাংলা corpus/paragraph/inventory: NOT APPLICABLE (এটি বাংলা টাইপিং layout নয়)।
- Windows runtime: BLOCKED।
