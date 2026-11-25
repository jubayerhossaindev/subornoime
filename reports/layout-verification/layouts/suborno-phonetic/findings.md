# Findings — Suborno

- সব mandated টাইপিং টেস্ট engine-level-এ PASS (brief 15/15, corpus 49/49, inventory 67/67, paragraph 3/3)।
- Stateful PhoneticInputSession: pending control token (যেমন :danda) পরে close হয়; composition suite এটি যাচাই করে (ENG-COMP-01)।
- :nukta: control শুধু consonant-run context-এ ় যোগ করে (phonetic_parser.cpp:355-360); standalone অবস্থায় literal থাকে — এটি ডিজাইন অনুযায়ী সঠিক (নুকতা একটি combining mark)।
- BUG-06/F-3/F-4 fix commit c1462f4; সংশ্লিষ্ট regression টেস্ট এই রানে OK।
- Windows runtime: BLOCKED (সব WIN-xxx আইটেম)।
