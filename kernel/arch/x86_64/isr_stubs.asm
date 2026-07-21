; ============================================================
; isr_stubs.asm — Interrupt Service Routine Entry Trampolines
; NovaOS Milestone 2, 4 & 5 — x86-64 NASM
; ============================================================

bits 64
section .text

global isr_common_stub
global isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3
global isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7
global isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11
global isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15
global isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19
global isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23
global isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27
global isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31
global irq_stub_0,  irq_stub_1

extern isr_handler
extern pit_tick_handler
extern keyboard_irq_handler
extern pic_send_eoi
extern sched_tick

%macro ISR_NO_ERRCODE 1
isr_stub_%1:
    push    qword 0
    push    qword %1
    jmp     isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
isr_stub_%1:
    push    qword %1
    jmp     isr_common_stub
%endmacro

ISR_NO_ERRCODE 0   ; #DE Divide Error
ISR_NO_ERRCODE 1   ; #DB Debug
ISR_NO_ERRCODE 2   ; NMI
ISR_NO_ERRCODE 3   ; #BP Breakpoint
ISR_NO_ERRCODE 4   ; #OF Overflow
ISR_NO_ERRCODE 5   ; #BR BOUND Range Exceeded
ISR_NO_ERRCODE 6   ; #UD Invalid Opcode
ISR_NO_ERRCODE 7   ; #NM Device Not Available
ISR_ERRCODE    8   ; #DF Double Fault
ISR_NO_ERRCODE 9   ; Coprocessor Segment Overrun
ISR_ERRCODE    10  ; #TS Invalid TSS
ISR_ERRCODE    11  ; #NP Segment Not Present
ISR_ERRCODE    12  ; #SS Stack-Segment Fault
ISR_ERRCODE    13  ; #GP General Protection Fault
ISR_ERRCODE    14  ; #PF Page Fault
ISR_NO_ERRCODE 15  ; Reserved
ISR_NO_ERRCODE 16  ; #MF x87 FPU Error
ISR_ERRCODE    17  ; #AC Alignment Check
ISR_NO_ERRCODE 18  ; #MC Machine Check
ISR_NO_ERRCODE 19  ; #XM SIMD Floating-Point
ISR_NO_ERRCODE 20  ; #VE Virtualization Exception
ISR_ERRCODE    21  ; #CP Control Protection
ISR_NO_ERRCODE 22  ; Reserved
ISR_NO_ERRCODE 23  ; Reserved
ISR_NO_ERRCODE 24  ; Reserved
ISR_NO_ERRCODE 25  ; Reserved
ISR_NO_ERRCODE 26  ; Reserved
ISR_NO_ERRCODE 27  ; Reserved
ISR_NO_ERRCODE 28  ; #HV Hypervisor Injection
ISR_NO_ERRCODE 29  ; #VC VMM Communication Exception
ISR_ERRCODE    30  ; #SX Security Exception
ISR_NO_ERRCODE 31  ; Reserved

; ============================================================
; Hardware IRQ Stubs
; ============================================================

; IRQ0: Timer (PIT)
irq_stub_0:
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    call    pit_tick_handler

    mov     rdi, 0              ; IRQ 0
    call    pic_send_eoi

    call    sched_tick          ; scheduler preemptive tick

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
    iretq

; IRQ1: Keyboard
irq_stub_1:
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    call    keyboard_irq_handler

    mov     rdi, 1              ; IRQ 1
    call    pic_send_eoi

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
    iretq

; Exception common handler stub
isr_common_stub:
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rdi, rsp
    call    isr_handler

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    add     rsp, 16
    iretq
