# NOVA OS Kernel Core: Deep Dive & Implementation Manual
**Target Architecture:** `x86_64 Long Mode` | **Kernel Type:** `High-Assurance Microkernel`

---

## 1. Boot Transition: 32-bit Protected Mode to 64-bit Long Mode (`boot/boot.asm`)

The entry point of NOVA OS is executed by the bootloader at physical address `0x00100000` (1MB mark). The execution flow follows standard CPU hardware state transitions:

```text
BIOS / UEFI -> Multiboot Loader -> _start (32-bit Protected Mode)
       |
       +--> Verify CPUID capability & x86_64 Long Mode support
       +--> Identity-map first 1GB RAM in boot_pml4, boot_pdpt, boot_pd (2MB Huge Pages)
       +--> Enable PAE in CR4 (Physical Address Extension bit 5)
       +--> Enable Long Mode (LME bit 8) & No-Execute (NXE bit 11) in EFER MSR (0xC0000080)
       +--> Enable Paging & Protection (CR0.PG | CR0.PE)
       +--> Load 64-bit Global Descriptor Table (lgdt [gdt64])
       +--> Far jump to 64-bit Long Mode: jmp gdt64.code:long_mode_start
       +--> Reload Segment Registers (DS, ES, FS, GS, SS = 0x10)
       +--> Initialize Kernel Stack (stack_top = 16KB)
       +--> Call kmain() (Ring 0 C Entry Point)
```

### Critical Architectural Detail: The NXE Bit in EFER MSR
In modern x86_64 processors, page table entry bit 63 is reserved unless the No-Execute Enable (NXE) bit is set in the Extended Feature Enable Register (`EFER`, MSR `0xC0000080`). If a kernel attempts to map a page with bit 63 set without enabling `NXE`, the CPU immediately triggers a Page Fault with error code `0xA` (Reserved Bit Violation). 
In NOVA OS, `boot.asm` explicitly writes:
```nasm
mov ecx, 0xC0000080
rdmsr
or eax, (1 << 8) | (1 << 11) ; LME (Long Mode Enable) | NXE (No-Execute Enable)
wrmsr
```
This enables hardware enforcement of non-executable data pages before the first C instruction executes.

---

## 2. Global Descriptor Table (GDT) & Task State Segment (TSS) (`kernel/arch/x86_64/gdt.c`)

In 64-bit mode, memory segmentation is largely disabled in favor of flat 64-bit paging. However, the GDT remains mandatory for:
1. Setting the processor privilege ring (`Ring 0` vs `Ring 3`).
2. Defining code execution characteristics (64-bit Long Mode vs 32-bit compatibility mode).
3. Supplying the Task State Segment (`TSS`), which holds the privileged stack pointer (`RSP0`).

### NOVA OS GDT Layout:
* `0x00`: Null Descriptor.
* `0x08`: Kernel 64-bit Code (Access `0x9A`: Present, Ring 0, Executable, Readable; Granularity `0x20` Long Mode).
* `0x10`: Kernel Data (Access `0x92`: Present, Ring 0, Writable).
* `0x18`: User Data (Access `0xF2`, with RPL=3 selector `0x1B`: Present, Ring 3, Writable).
* `0x20`: User 64-bit Code (Access `0xFA`, with RPL=3 selector `0x23`: Present, Ring 3, Executable, Readable).
* `0x28`: 64-bit TSS Descriptor (16 bytes, pointing to `kernel_tss`).

When an unprivileged Ring 3 application executes a system call or triggers a hardware interrupt, the x86 CPU automatically switches the stack to `kernel_tss.rsp0`, guaranteeing that kernel execution never relies on an untrusted userspace stack.

---

## 3. Interrupt Handling & Exception Diagnostics (`kernel/arch/x86_64/idt.c`)

NOVA OS programs a 256-entry Interrupt Descriptor Table (`IDT`):
* **Vectors 0..31:** Dedicated CPU Exceptions.
  * Vectors `0` to `31` map directly to assembly ISR stubs (`interrupts.asm`) that push error codes, save all general-purpose registers (`RAX`, `RBX`, `RCX`, `RDX`, `RSI`, `RDI`, `RBP`, `R8-R15`), and call the C handler `isr_common_handler()`.
  * **Page Fault Handler (Vector 14):** Automatically reads `CR2` (the faulting memory address) and decodes the error code (identifying whether the fault was due to a non-present page, protection violation, write attempt, user-mode origin, or instruction fetch).
* **Vectors 32..47:** Hardware IRQs 0..15 remapped through the 8259 PIC (`pic.c`).
  * IRQ 0 (Timer) is bound to the preemptive scheduler tick (`pit_tick_handler()` and `sched_tick()`).
* **Vector 128 (0x80):** Ring 3 System Call Gate with descriptor flags `0xEE` (Present, Ring 3 Accessible, 64-bit Interrupt Gate).

---

## 4. Physical Memory Manager (PMM) (`kernel/mm/pmm.c`)

NOVA OS uses a high-performance **Page Frame Bitmap Allocator**:
* Tracks physical memory in discrete 4096-byte (4KB) frames.
* For a standard 512MB RAM configuration:
  * Total Frames: `512 * 1024 * 1024 / 4096 = 131,072 frames`.
  * Bitmap Size: `131,072 / 8 = 16,384 bytes` (exactly 4 pages / 16KB).
* The bitmap is positioned immediately following the kernel binary in memory.
* High-speed bit manipulation functions (`bitmap_set`, `bitmap_clear`, `bitmap_test`) allocate frames in constant time.

---

## 5. Virtual Memory Manager (VMM) & W^X Policy (`kernel/mm/vmm.c`)

The VMM implements x86_64 4-Level Paging:
1. **PML4** (Page Map Level 4): Bits `[47:39]` of virtual address.
2. **PDPT** (Page Directory Pointer Table): Bits `[38:30]`.
3. **PD** (Page Directory): Bits `[29:21]`.
4. **PT** (Page Table): Bits `[20:12]`.

### Mathematical Enforcement of W^X (Write XOR Execute):
When `vmm_map_page(pml4, virt, phys, flags)` is invoked, it verifies the security invariant:
```c
if ((flags & PAGE_WRITABLE) && !(flags & PAGE_NX)) {
    flags |= PAGE_NX; // Force No-Execute bit on all writable pages
}
```
This guarantees that an attacker cannot write shellcode into a buffer and subsequently redirect execution to that buffer.

---

## 6. Hardened Kernel Heap Allocator (`kernel/mm/heap.c`)

The kernel heap (`kmalloc`, `kzalloc`, `kfree`) operates on an isolated 8MB pool (`0x01000000..0x01800000`) protected by three layers of defensive checks:
1. **Magic Verification:** Every allocated block begins with `HEAP_MAGIC_ALLOCATED` (`0x4E4F5641414C4C43`). Every free block begins with `HEAP_MAGIC_FREE` (`0x4E4F564146524545`).
2. **Canary Cookie:** A 64-bit secret constant (`0xDEADC0DEBEEFCAFE`) is placed in the block header. If an out-of-bounds heap write corrupts this value, `kfree()` halts execution immediately before memory corruption can propagate.
3. **Buffer Poisoning:** Upon deallocation, the entire user payload is overwritten with `0x55` to immediately expose and crash any attempts at Use-After-Free.

---

## 7. Preemptive Multi-Level Priority Scheduler (`kernel/sched/scheduler.c`)

NOVA OS features an autonomous priority-based preemptive scheduler:
* **Priority Levels:**
  * Priority 0: Microkernel internal tasks.
  * Priority 1: User-Space Root Manager (`init-server`).
  * Priority 2: System Servers (Storage, Network, Compositor).
  * Priority 3: Background Batch & Idle Task (`idle`).
* **Time Quantum:** Calibrated to the 8254 PIT (1000 Hz). Higher-priority tasks receive up to 40ms of CPU time per quantum; lower-priority tasks receive 10ms.
* **Context Switch (`sched/context.asm`):** Fast assembly routine that saves callee-saved registers into the outgoing `cpu_context_t` struct and restores registers from the incoming task struct before updating `RSP0` in the TSS.

---

## 8. Capability Access Control Engine (`kernel/security/capabilities.c`)

Every task structure contains a 256-slot **Capability Table**:
* Object types: `CAP_TYPE_MEMORY`, `CAP_TYPE_IPC_ENDPOINT`, `CAP_TYPE_THREAD`, `CAP_TYPE_PROCESS`, `CAP_TYPE_DEVICE_PORT`.
* Rights bitmask: `CAP_RIGHT_READ`, `CAP_RIGHT_WRITE`, `CAP_RIGHT_EXECUTE`, `CAP_RIGHT_GRANT`, `CAP_RIGHT_REVOKE`, `CAP_RIGHT_IPC_SEND`, `CAP_RIGHT_IPC_RECV`, etc.
* **Rights Attenuation Principle:** A process can only delegate a capability with equal or fewer rights than it holds (`delegated_rights = parent.rights & requested_rights`). If a child requests rights the parent does not hold, the operation is blocked.
