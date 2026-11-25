# BSTI/BCC-কে আনুষ্ঠানিক তথ্য চাওয়ার খসড়া (বাংলা)

> **ব্যবহারের নিয়ম**: এই খসড়া প্রস্তুত করা হয়েছে কিন্তু **পাঠানো হয়নি**।
> ব্যবহারকারীর অনুমতি ছাড়া কোনো email, form submission বা legal declaration
> করা হবে না (mission rule)। পাঠানোর আগে প্রতিষ্ঠানের সর্বশেষ official
> contact channel যাচাই করে নিতে হবে।

---

**প্রতি:**
মহাপরিচালক, বাংলাদেশ স্ট্যান্ডার্ডস এন্ড টেস্টিং ইনস্টিটিউশন (BSTI),
ঢাকা।
(কপি: মহাপরিচালক, বাংলাদেশ কম্পিউটার কাউন্সিল (BCC), আগারগাঁও, ঢাকা)

**বিষয়: SubornoIME — নেটিভ Windows বাংলা IME-এর জন্য BDS কনফরম্যান্স
মূল্যায়ন ও সার্টিফিকেশনের যোগ্যতা সংক্রান্ত সরকারি দিকনির্দেশনার আবেদন**

জনাব,

আমরা SubornoIME নামে একটি নেটিভ Windows বাংলা ইনপুট মেথড এডিটর (IME)
উন্নয়ন করছি — C++20, Win32, TSF, COM ভিত্তিক, MIT লাইসেন্সের অধীনে
বিতরণযোগ্য ডেস্কটপ সফটওয়্যার। আমাদের পণ্যটি সাতটি কীবোর্ড লেআউট সমর্থন
করে এবং সম্পূর্ণ ইউনিকোড-ভিত্তিক আউটপুট দেয় (UTF-8/UTF-16, NFC)।

আমরা নিম্নলিখিত জাতীয় মানসমূহের প্রতি পণ্যের সামঞ্জস্য যাচাই করতে চাই:
- **BDS 1738:2018** — Specification for Computer Bangla Keyboard (First Revision)
- **BDS 1520:2018** — Specification for Bangla Coded Character Set for Information Interchange
- **BDS 1935:2018** — Bangladesh Standard Code for Information Interchange (BSCII)
- **BDS 1834:2011** — Bangla Keypad for Mobile Phone

আমাদের প্রশ্নসমূহ:

1. ডাউনলোডযোগ্য ডেস্কটপ সফটওয়্যার (এই IME) কি BSTI-র certification mark
   কাঠামোর মধ্যে পড়ে? বাধ্যতামূলক পণ্যের তালিকায় (৩১৫ টি পণ্যের S.R.O.)
   সফটওয়্যার/IT পণ্যের অবস্থান কী?
2. BSTI কি সফটওয়্যার পণ্যের conformity assessment বা সার্টিফিকেশন করে?
   থাকলে কোন scheme-এ (mandatory CM / voluntary CM / test report / অন্য কোনো)?
3. BDS 1738:2018-এর scope কি physical keyboard/device-সীমিত, নাকি সফটওয়্যারে
   বাস্তবায়িত লেআউটও (layout implementation) এর অধীনে আসে?
4. BDS 1520:2018 (তৃতীয় পরিমার্জন) ও BDS 1935:2018-এর জন্য সফটওয়্যার
   conformance assessment কি সম্ভব? সম্ভব হলে কী ধরনের প্রমাণপত্র/test dossier
   গ্রহণযোগ্য?
5. স্বেচ্ছামূলক (voluntary) certification route কি এই ধরনের পণ্যের জন্য
   উন্মুক্ত? শর্তাবলী কী?
6. Official test procedure কী? কোন accredited/recognized laboratory বা
   assessment authority প্রযোজ্য?
7. আবেদনের জন্য কী কী document/form/fee লাগবে? (official form নম্বর ও ফি
   তালিকা সংযুক্ত করতে অনুরোধ করছি।)
8. সফটওয়্যারের ক্ষেত্রে "regular inspection or testing" (BSTI আইন ২০১৮,
   ধারা ১৬(২)(খ))-এর সমতুল্য কী ধরা হবে — build system, test report,
   সংস্করণ অডিট, না অন্য কিছু?
9. BDS 1738:2018-এর foreword-এ উল্লিখিত Bijoy keyboard সংক্রান্ত intellectual
   property rights-এর ভিত্তিতে, official mapping table-এর সফটওয়্যার
   বাস্তবায়নের জন্য কি মালিকের পক্ষ থেকে আলাদা license/permission প্রয়োজন?
   থাকলে সেটি কীভাবে গ্রহণ করতে হবে?
10. BDS 1834:2011 (mobile phone keypad) কি Windows ডেস্কটপ IME-এর ক্ষেত্রে
    অপ্রযোজ্য বলে BSTI-র লিখিত মতামত দেওয়া সম্ভব?
11. আমরা কি authorized full copy ক্রয়ের জন্য BDS e-store-এর (account +
    Sonali Payment Gateway) পথই ব্যবহার করব, নাকি conformance assessment-
    এর অংশ হিসেবে মানদণ্ড সরবরাহের অন্য ব্যবস্থা আছে?
12. BCC-র standards তালিকায় BDS 1520-এর download-টি "BDS 1520:2018" লেবেলে
    দেওয়া, কিন্তু সংযুক্ত document-এর cover-এ "(Second Revision)" এবং page
    header-এ "BDS 1520:2011" লেখা; অথচ BSTI Catalogue 2023-এ BDS 1520:2018-
    কে "(Third Revision)" লেখা আছে। অনুরোধ: (ক) বর্তমান authorized
    সংস্করণটি কোনটি, এবং (খ) Second Revision-এর Table 2-এর character
    allocation Third Revision-এ অপরিবর্তিত আছে কি না, লিখিতভাবে জানান।
13. bstibds.com ঠিকানাটি (BSTI-র service page-এ এখনও "e-Catalog & BDS Sale"
    হিসেবে সংযুক্ত) বর্তমানে একটি license-checking application হোস্ট করছে।
    কি ec.eservice.bsti.gov.bd-ই BDS ক্রয়ের একমাত্র official চ্যানেল?

আমাদের পণ্যের বর্তমান প্রযুক্তিগত অবস্থা (সংক্ষেপ, ২০২৬-১১-২৪): ইউনিকোড
বাংলা ব্লকের সম্পূর্ণ repertoire যাচাই (128/128 code point), official
`NormalizationTest.txt` (UAX #15) বাংলা লাইনের NFC কনফরম্যান্স (7/7),
579-রows মাল্টি-লেআউট engine প্রমাণ, ASan/UBSan-clean টেস্ট স্যুট (591+2
PASS), এবং official BDS 1520 document থেকে তৈরি 94-row allocation corpus
engine-বিপরীতে যাচাইকৃত (94/94)। সম্পূর্ণ evidence dossier প্রস্তুত আছে এবং
assessment-এর জন্য জমা দিতে প্রস্তুত।

আপনার লিখিত দিকনির্দেশনার জন্য আগাম ধন্যবাদ।

বিনীত,
[আবেদনকারীর নাম / প্রতিষ্ঠান]
[ঠিকানা, ইমেইল, ফোন]
[তারিখ]

---

**সংযুক্তি তালিকা (পাঠানোর আগে প্রস্তুত করতে হবে)**: product brief,
BDS_MASTER_COMPLIANCE_MATRIX.csv, test summary, license inventory.
