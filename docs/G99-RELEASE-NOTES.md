# MobileGlues G99 Test 2 — experimental

## العربية

- نسخة جديدة بمعمارية arm64 من التطبيق التجريبي، وليست تعديلًا للانشر.
- التقنية الجديدة: **Driver State Filtering** لتسع دوال GL، في جدول GLES نفسه حتى تشمل أوامر المحرك الداخلية واستعادتها. يحفظ كل سياق حالته مستقلًا، ولا يخزن السياقات غير المتتبعة أو القيم الفاسدة، ولا يحذف رسمًا أو مزامنة أو يغير دقة الشادر.
- يمكن أن تقل كلفة CPU عند كثرة أوامر الحالة المكررة. **لا توجد زيادة FPS مقاسة على هاتف G99، ولا ضمان بفرق ملحوظ إذا كان الاختناق GPU-bound**. عدادات الأوامر ليست قياس FPS.
- إعدادات G99 الجديدة مفعلة افتراضيًا للإعدادات الجديدة بعد التحقق من G99/MT6789 + Mali-G57. للإعدادات القديمة اضغط **تطبيق افتراضيات G99** بعد نسخ MG/config.json احتياطيًا، ثم أعد تشغيل اللعبة.
- الافتراضيات: GLES أصلي، عدم إخفاء الأخطاء، GL تلقائي، كاش 64 MiB، CS/timer/DSA وFSR1 معطلة، وترتيب multi-draw الافتراضي المرشّح حسب الإمكانات. لا نقول إن ترتيبًا ثابتًا هو أسرع ترتيب لكل الألعاب؛ يمكن إعادة اختبار multi-draw على الهاتف.
- السجل MG/latest.log يعرض `[G99 FastPath]` وعدّادات submitted/forwarded/skipped بعد 120 إطارًا ثم كل 600. لا يوجد AI frame generation.
- **توقيع هذه الدفعة مختلف عن Test 1 عند استخدام مفتاح التجربة المؤقت؛ احذف التطبيق التجريبي القديم أولًا ثم ثبّت Test 2. لا تحذف التطبيق الرسمي أو مجلد MG.** إعدادات MG مشتركة؛ لا تعدّلها من تطبيقين معًا.
- لم يُجرَ اختبار جهاز فعلي. قارن العالم والدقة والحرارة والـmods نفسها، وأرسل السجل وFPS قبل/بعد بدل افتراض أن عدد الأوامر المتجاوزة يساوي ارتفاع FPS.

## English

Experimental ARM64 build, app version 2.0.0-g99-test.2 / code 2002. Adds per-context filtering of nine valid, identical driver-state assignments at the shared GLES dispatch boundary. No draw skipping, synchronization removal, precision reduction, AI generation or measured FPS claims. This targets CPU driver overhead, not GPU shader workload. New configs default to the G99 profile after hardware gating; use the confirmed Apply G99 defaults action for inherited configs. One-release signing may require uninstalling the old experimental app before installation; do not remove MG or the official app.
