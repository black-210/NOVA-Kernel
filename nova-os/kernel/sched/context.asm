; NOVA OS - Next-Generation Microkernel Operating System
; File: kernel/sched/context.asm
; Description: Preemptive thread context switching routine.
; Signature: void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx)
; License: Apache-2.0

BITS 64

global context_switch

context_switch:
    ; RDI = old_ctx pointer
    ; RSI = new_ctx pointer

    ; 1. Save callee-saved registers into old_ctx if non-null
    test rdi, rdi
    jz .load_new

    ; Save stack pointer
    mov [rdi + 0],  rsp
    mov [rdi + 8],  r15
    mov [rdi + 16], r14
    mov [rdi + 24], r13
    mov [rdi + 32], r12
    mov [rdi + 40], rbx
    mov [rdi + 48], rbp

    pushfq
    pop rax
    mov [rdi + 56], rax ; Save RFLAGS

.load_new:
    ; 2. Restore callee-saved registers from new_ctx
    mov rsp, [rsi + 0]
    mov r15, [rsi + 8]
    mov r14, [rsi + 16]
    mov r13, [rsi + 24]
    mov r12, [rsi + 32]
    mov rbx, [rsi + 40]
    mov rbp, [rsi + 48]

    mov rax, [rsi + 56]
    push rax
    popfq               ; Restore RFLAGS

    ret

section .note.GNU-stack noalloc noexec nowrite progbits
