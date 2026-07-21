; ============================================================
; context_switch.asm — Cooperative & Preemptive Context Switch
; NovaOS Milestone 5 — x86-64 NASM
;
; Function signature:
;   extern "C" void context_switch(uint64_t* old_rsp, uint64_t new_rsp);
;
; Parameters (SysV AMD64 ABI):
;   rdi = old_rsp (pointer to uint64_t where current RSP will be saved)
;   rsi = new_rsp (value of RSP to switch to)
; ============================================================

bits 64
section .text

global context_switch

context_switch:
    ; 1. Save callee-saved registers of current task onto its stack
    push    rbx
    push    rbp
    push    r12
    push    r13
    push    r14
    push    r15

    ; 2. Save current stack pointer into *old_rsp
    mov     [rdi], rsp

    ; 3. Switch stack pointer to new_rsp
    mov     rsp, rsi

    ; 4. Restore callee-saved registers from new task's stack
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    pop     rbx

    ; 5. Return to the instruction address saved on the new task's stack
    ret
