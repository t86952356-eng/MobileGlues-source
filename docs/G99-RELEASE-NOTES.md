# MobileGlues G99 Test 3 — experimental, guided by the Test 2 phone log

## العربية

- سجل الهاتف أثبت تفعيل تجربة G99 ومرشح الحالة في Test 2، لكن المستخدم لم يلاحظ تحسنًا. لا نقول إن تخطي أوامر الحالة دليل على FPS أعلى.
- السجل أظهر GLES 3.2 على Mali-G57 MC2/MT6789، وغياب GL_EXT_multi_draw_indirect، واختيار indirect لقوائم العناصر. في هذا المسار يبني المحرك أوامر على CPU ويرفعها إلى buffer ثم ينفذ كل رسم على حدة: ليس دمجًا حقيقيًا.
- **التغيير:** CPU-command upload elision. عندما تأتي قائمة الرسم من CPU، ولا يتوفر MDI الحقيقي، ويوجد EBO وسياق معروف ودعم مباشر صالح، تمرر الأوامر بالترتيب إلى glDrawElements/glDrawElementsBaseVertex دون بناء ورفع الأوامر المؤقتة. عدد الرسومات نفسه؛ ليست تقنية جديدة لتقليل تظليل GPU، والـbasevertex المباشر موجود أصلًا كخلفية منفصلة. الجديد هو تجاوز الجسر الزائد داخل المسار المختار وإتاحة مقارنة مستقلة.
- أوامر indirect القادمة من التطبيق أو GPU تبقى كما هي؛ لا قراءتها إلى CPU أو حذفها. تبقى primitive restart وحدود التحقق ومسارات fallback. الأوضاع ذات MDI الحقيقي لا تُستبدل. يُخبر عداد benchmark أن المسار تغير فلا ينسب نتيجة الإرسال المباشر إلى indirect.
- مفتاح جديد **تجاوز رفع أوامر CPU**. أبقِ Helio G99 Test مفعلاً، وقارن هذا المفتاح وحده Off ثم On مع إعادة تشغيل اللعبة في المشهد نفسه. لا حاجة لإعادة ضبط إعدادات Test 2 التي أظهر السجل أنها مفعلة بالفعل.
- سجل جديد كل 600 عرض: `[G99 Render]` بعدادات الرسم و`bypass_batches/bypass_subdraws/avoided_cmd_bytes`؛ و`[G99 Wait]` لأزمنة CPU لاستدعاءات swap/map/read/finish وعينة 1 من 64 رسمًا. ليست أزمنة GPU ولا قياسًا سببيًا للاختناق؛ swap يشمل vsync والانتظار. العدادات تحصي أنواع الرسم المغلفة فقط وليست أثرًا كاملًا لكل GL API.
- لا خفض دقة أو جودة أو precision، ولا frame generation أو overclock أو إزالة مزامنة. لا قياسات FPS على هاتف المطور ولا ضمان تحسن.
- **معرّف مستقل:** com.fcl.plugin.mobileglues.g99test3، الإصدار 2.0.0-g99-test.3 / code 2003، arm64-v8a. يمكن تثبيته بجانب Test 2 دون حذفه. اختر عارض Test 3 الصحيح. تطبيقات MG تشترك في MG/config.json؛ انسخه احتياطيًا ولا تعدله من تطبيقين معًا. التوقيع غير رسمي ومؤقت لهذه الدفعة.
- شغّل عالمًا فعليًا نحو دقيقة وأرسل MG/latest.log مع FPS ونسخة Minecraft والـmods ودقة اللعب. إذا كان bypass_batches=0 فهذه التقنية لم تخدم المشهد؛ لا تُنسب إليها زيادة ولا فشل تقني وهي غير مستخدمة. سجل أزمنة الانتظار وحده لا يثبت أن البطء GPU-bound.

## English

Test 3 avoids the CPU-built indirect-command upload for eligible CPU draw lists on the logged G99 configuration without real MDI. It preserves individual draw order, count, index offset and base vertex; does not merge draws, change shaders or read GPU command buffers. Adds an independent OFF/ON switch and per-context render/wait diagnostics. Timings are CPU wall time, not GPU durations or measured FPS gains. Separate app ID allows side-by-side installation with Test 2; MG configuration remains shared. Unofficial prerelease, no phone FPS promise.
