# مواصفات NOVA OS ABI الرسمية (Kernel & Driver ABI v1.0.0-LTS)
## Frozen System Call & Interface Contract Specification

### 1. سياسة استقرار الواجهات (ABI Stability Policy)
تلتزم NOVA OS بسياسة استقرار صارمة تحمي المطورين وشركات العتاد من كسر التوافق العشوائي:
- **المعرف المميز:** `NOVA_ABI_SIGNATURE = 0x4E4F56414F530100ULL` ('NOVAOS\x01\x00').
- **الإصدار الحالي:** Major 1, Minor 0, Patch 0.
- **القاعدة الأساسية:** لا يجوز تغيير ترتيب أو أحجام أو محاذاة الحقول في البنى العامة المحددة في `abi/nova_abi.h`.
- **أي توسع مستقبلي:** يتم عبر إسناد أرقام استدعاءات نظام جديدة في فضاء الاستدعاءات الإضافي مع الاحتفاظ بالاستدعاءات القديمة لضمان التوافق الخلفي.

---

### 2. جدول استدعاءات النظام (Syscall Dispatch Table)

| رقم الاستدعاء | اسم الاستدعاء في ABI | المعاملات | الوصف الهندسي |
| :--- | :--- | :--- | :--- |
| `0` | `SYS_NOVA_NULL` | لا يوجد | اختبار استجابة النواة والتحقق من السياق |
| `1` | `SYS_NOVA_LOG` | `str_ptr`, `len` | طباعة آمنة ومفحوصة المؤشرات لسجلات تصحيح المستخدم |
| `2` | `SYS_NOVA_CAP_CREATE` | `type`, `rights`, `object_id` | إنشاء تذكرة قدرة جديدة في جدول صلاحيات العملية |
| `3` | `SYS_NOVA_CAP_DELEGATE` | `src_slot`, `dst_task_id`, `mask` | تفويض قدرة مع تخفيف الصلاحيات (Rights Attenuation) |
| `4` | `SYS_NOVA_CAP_REVOKE` | `slot` | إلغاء قدرة فورياً واسترداد الصلاحيات |
| `5` | `SYS_NOVA_IPC_CALL` | `endpoint_id`, `req_ptr`, `resp_ptr` | استدعاء IPC تزامني فائق السرعة مع تبادل رسائل 64 بايت |
| `6` | `SYS_NOVA_IPC_REPLY` | `endpoint_id`, `msg_ptr` | إرسال رد على رسالة سابقة في نقطة النهاية |
| `7` | `SYS_NOVA_IPC_WAIT` | `endpoint_id`, `out_msg_ptr` | انتظار ورود رسالة من نقطة نهاية محددة |
| `8` | `SYS_NOVA_TASK_CREATE` | `name_ptr`, `entry_ptr`, `prio` | إنشاء عملية جديدة في فضاء عنونة مستقل |
| `9` | `SYS_NOVA_TASK_TERMINATE` | `exit_code` | إنهاء العملية الحالية وتحرير مواردها بأمان |
| `10` | `SYS_NOVA_MEM_MAP` | `virt`, `phys`, `flags` | حجز صفحات ذاكرة مع فرض W^X وبت NX |
| `11` | `SYS_NOVA_MEM_UNMAP` | `virt` | تحرير حجز الصفحات وتطهير TLB |
| `12` | `SYS_NOVA_PORT_IO` | `port`, `val`, `is_write`, `width` | وصول مفحوص الصلاحيات لمنافذ الإدخال والإخراج |
| `13` | `SYS_NOVA_TIME_GET` | لا يوجد | قراءة عداد نبضات الموقت الموحد (PIT Ticks) |
| `14` | `SYS_NOVA_SYS_INFO` | `info_struct_ptr` | استعلام إحصائيات النظام والذاكرة وتبديل السياق |

---

### 3. بنية غلاف رسائل IPC (64 Bytes Cache-Line Aligned)
```c
typedef struct __attribute__((aligned(64))) {
    uint32_t sender_pid;        /* 4 bytes */
    uint32_t target_endpoint;  /* 4 bytes */
    uint32_t flags;            /* 4 bytes */
    uint32_t message_type;     /* 4 bytes */
    uint64_t token;            /* 8 bytes */
    uint64_t transferred_cap;  /* 8 bytes */
    uint64_t sequence_id;      /* 8 bytes */
    uint64_t data[3];          /* 24 bytes inline fast payload */
} nova_ipc_msg_t;
```
تضمن محاذاة 64 بايت عدم حدوث False Sharing في معالجات x86_64، وتسمح بنقل الرسائل بتعليمات MOV دقيقة وبزمن أقل من 5 نانوثانية.
