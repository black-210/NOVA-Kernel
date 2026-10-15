# خارطة الطريق الهندسية لتشغيل بيئات GNOME وKDE Plasma على NOVA OS
## الدراسة الفنية ومراحل تكامل واجهات سطح المكتب

### 1. تحليل الاعتماديات الفعلية لبيئات GNOME وKDE
لا يمكن افتراض أن GNOME Shell أو KDE Plasma تعمل مباشرة على نواة دقيقة جديدة دون بناء الطبقات الهندسية التي تتطلبها هذه المشاريع:

1. **بروتوكول العرض (Display Protocol):**
   - تعتمد GNOME وKDE بالكامل على **Wayland** (مع دعم اختياري لـ XWayland للتطبيقات القديمة).
   - تحتاج إلى Display Server / Compositor متوافق.
2. **بروتوكول التواصل بين الخدمات (IPC / Messaging):**
   - تعتمد بيئات سطح المكتب على **D-Bus** لتنسيق الحالات، والتنبيهات، والتحكم في الطاقة، والتكامل بين التطبيقات.
3. **طبقة الوسائط المتعددة والصوت (Audio & Media):**
   - تعتمد على **PipeWire** لمعالجة الصوت ومشاركة الشاشات في Wayland.
4. **محركات الرسوميات وتجريد العتاد (Graphics Stack):**
   - تعتمد على **Mesa 3D** لتوفير برامج تشغيل OpenGL وVulkan، وواجهات Direct Rendering Manager (DRM / KMS).
5. **مكتبة C القياسية (Libc & POSIX APIs):**
   - تتطلب توفير واجهات مثل `fork`, `execve`, `epoll`, `pthread`, `socket`, `mmap` أو بدائل معيارية متطابقة.

---

### 2. خطة التنفيذ المرحلية في NOVA OS

```
[المرحلة 1: منجز]  NOVA Glass Compositor الأصلي المزدوج (Double-buffered Surface Manager)
       |
[المرحلة 2: منجز]  Wayland Protocol Surface Bridge في سيرفر الرسوميات (servers/graphics/compositor.c)
       |
[المرحلة 3: قادم]  تطوير مكتبة libposix-shim لترجمة استدعاءات POSIX إلى رسائل IPC السريعة لنواة NOVA
       |
[المرحلة 4: قادم]  بناء خادم D-Bus متوافق يعتمد على آلية Fast IPC Rendezvous لنواة NOVA
       |
[المرحلة 5: قادم]  تصريف خادم Wayland Compositor (KWin لـ KDE و Mutter لـ GNOME) مع تعريفات Mesa DRM
       |
[المرحلة 6: الهدف] تشغيل الجلسة الكاملة لـ GNOME Shell وKDE Plasma فوق نواة NOVA المستقلة!
```

**تنبيه الشفافية الهندسية:**
حتى اكتمال طبقة توافق POSIX ومكتبات Wayland وMesa، يوفر نظام NOVA OS حالياً واجهة سطح مكتب تفاعلية أصيلة (NOVA Glass UI) لإدارة العمليات ومراقبة النواة وتصفح نظام الملفات وتشغيل الطرفية، دون الادعاء الزائف باكتمال تشغيل GNOME أو KDE قبل إثبات ذلك باختبارات فعلية.
