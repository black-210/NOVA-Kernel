# وثيقة العمارة الهندسية لنظام التشغيل NOVA OS
## الهوية الهندسية لنواة NOVA Microkernel (الإصدار v1.0-LTS)

### 1. الفلسفة المعمارية ومقارنة البدائل
تم تصميم نواة **NOVA Kernel** كبنية **نواة دقيقة (Microkernel)** صارمة العزل، تعالج جذرياً الاختلالات الهيكلية المتوارثة في نواة Linux الأحادية (Monolithic):

```
+-------------------------------------------------------------------------+
|                         USER APPLICATIONS / DESKTOP                     |
|         (Wayland Clients, GNOME / KDE Apps, POSIX Subsystem)           |
+-------------------------------------------------------------------------+
       |                                                    |
       v (IPC Messages)                                     v (IPC Messages)
+-------------------------------------------------------------------------+
|                  USER-SPACE ISOLATED SYSTEM SERVERS                     |
|  +--------------------+  +--------------------+  +-------------------+  |
|  |   VFS / NOVA-FS    |  |    Network Stack   |  |   NOVA Glass      |  |
|  | (Extents, Journal) |  | (Stateful Firewall)|  | (Wayland Bridge)  |  |
|  +--------------------+  +--------------------+  +-------------------+  |
|  +--------------------+  +--------------------+                         |
|  | Device Mgr (devmgr)|  | User-Space Drivers |                         |
|  | (Crash Supervision)|  | (PCI, Storage, Net)|                         |
|  +--------------------+  +--------------------+                         |
+-------------------------------------------------------------------------+
       |                                                    |
       v (Fast Synchronous Rendezvous Syscalls - Ring 3 -> Ring 0)        |
+-------------------------------------------------------------------------+
|                       NOVA MICROKERNEL (Ring 0)                         |
|  * Unforgeable Capability Token Engine (Zero Ambient Authority)         |
|  * 4-Level x86_64 Paging with Hardened W^X & NX Enforcement             |
|  * Physical Memory Manager (Bitmap Frame Allocator)                     |
|  * Preemptive Priority Round-Robin Scheduler & Context Switching        |
|  * Sub-Microsecond Synchronous Rendezvous IPC (64-byte Cacheline)       |
|  * Hardware IDT / GDT / PIC / PIT Core Architecture Hooks               |
+-------------------------------------------------------------------------+
```

---

### 2. معالجة عيوب Linux الهندسية في NOVA OS

| الخاصية الهندسية | نواة Linux (Monolithic) | نواة NOVA OS (Microkernel) | الأثر التقني |
| :--- | :--- | :--- | :--- |
| **استقرار ABI للتعريفات** | غير مستقر عمداً (No stable in-kernel ABI)؛ يتطلب إعادة بناء التعريفات مع كل إصدار | Kernel ABI و Driver ABI مجمد وموثق ومفحوص باختبارات تراجع | ثبات التعريفات الخارجية عبر الإصدارات دون كسر التوافق |
| **عزل التعريفات والأجهزة** | 70%+ من كود Linux تعريفات تعمل في Ring 0، وأي عطل يسقط النظام بكامله (Kernel Panic) | التعريفات تعمل في عمليات معزولة في User-Space تحت إشراف `devmgr` | تعطل تعريف كرت الشبكة أو الصوت لا يسقط النظام، ويعاد تشغيله تلقائياً |
| **سلامة الذاكرة وسياسة W^X** | تاريخ حافل بثغرات ROP وPrivilege Escalation بسبب مساحات الذاكرة المشتركة | فرض صارم لقاعدة W^X في VMM؛ لا يمكن لصفحة أن تكون Writable و Executable معاً | منع تنفيذ شفرات الهجوم في الذاكرة القابلة للكتابة |
| **نموذج الصلاحيات** | صلاحية الجذر (Root) الشاملة؛ وصول المخترق لـ root يمنحه السيطرة الكاملة | نموذج قدرات دقيق (Capabilities)؛ لا توجد سلطة مطلقة، وحق التفويض مشروط | تقييد أي عملية فقط بالموارد الممنوحة لها بنظام تفويض مشفر |
| **الاعتماديات المتبادلة** | تشابك هائل بين أنظمة الملفات والشبكات وإدارة الذاكرة (Spaghetti Dependencies) | تواصل حصري عبر رسائل IPC نمطية ذات 64 بايت محددة العقود | تدقيق أمني مستقل لكل خدمة دون الحاجة لفهم كامل النظام |
