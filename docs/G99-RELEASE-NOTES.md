## العربية

نسخة **تجريبية غير رسمية** لمعمارية arm64، تُثبت باسم **MobileGlues G99 Test** بجانب التطبيق الرسمي. افتح الإعدادات، امنح الوصول إلى مجلد MG الصحيح، وفعّل **Helio G99 Test**، ثم أعد تشغيل اللعبة واختر العارض التجريبي في اللانشر.

- يتطلب التفعيل الفعلي اكتشاف Mali-G57 مع Helio G99 أو معرّف عائلة MT6789؛ لا يكفي اسم GPU وحده. قد تشترك مشتقات المعالج في MT6789.
- يفضّل GLES الأصلي بدل ANGLE، ويوقف FSR1 في الوضع التجريبي، ويضبط كاش GLSL بين 64 و128 MiB. تبقى الإعدادات اليدوية محفوظة وتعود عند إيقاف الخيار وإعادة تشغيل اللعبة.
- يجمع كتابة كاش الشادر كل 64 عنصرًا أو بعد 5 ثوانٍ عند عملية الكاش التالية، بدل 16 عنصرًا. قد يُعاد توليد آخر 63 ترجمة عند إغلاق العملية قسرًا؛ لا يمس ذلك بيانات اللعبة.
- لا كسر سرعة، ولا تجاوز للحماية الحرارية، ولا حذف مزامنة أو تجاهل أخطاء إضافي أو تزييف امتدادات.
- **لم تُقَس زيادة FPS ولم يُجرَ اختبار على جهاز Helio G99 فعلي**. قد لا يتحسن الأداء أو قد يتراجع؛ أوقف التجربة عند حدوث خلل. قارن المشهد والإعدادات والحرارة نفسها، واستعمل اختبار multi-draw الموجود لاختيار الترتيب المقاس على جهازك.
- **توليد إطارات AI غير منفّذ**. تظهر حالة توضيحية فقط، لا مفتاح وهمي. FSR1 ليس AI frame generation.
- راجع `G99-SIGNING.txt`: إذا استُخدم مفتاح التجربة المؤقت فلن تقبل هذه النسخة تحديثًا من نسخة مستقبلية بتوقيع مختلف؛ يلزم حينها إعادة تثبيت التطبيق التجريبي. لم يُستخدم مفتاح MobileGL-Dev الرسمي، ولا يُنشر أي مفتاح خاص.
- التطبيق الرسمي والتجريبي قد يقرآن مجلد MG نفسه؛ لا تعدّل الإعدادات من التطبيقين بالتزامن. هذا ليس تعديلًا للانشر.

## English

Unofficial, opt-in arm64 experiment; separate application ID `com.fcl.plugin.mobileglues.g99test`. Native GLES, FSR1 disabled, bounded 64–128 MiB shader cache and 64-entry / five-second-on-cache-operation persistence batches. Detection requires both Mali-G57 and a G99 / MT6789-family SoC identifier. Stored manual settings are not rewritten. No measured FPS benefit is claimed; no physical-device test was performed. No AI frame generation is implemented.

See the bundled source commits, LGPL notices, SHA256 and signing certificate. A one-release experimental signer may require reinstalling future differently signed test builds. This is not an official MobileGL-Dev build. The source repository contains the renderer changes and a reproducible overlay on the pinned upstream plugin, so a separate plugin fork is not required.
