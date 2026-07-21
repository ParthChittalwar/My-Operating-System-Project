#include "scheduler.hpp"
#include "../mm/heap.hpp"
#include "../mm/pmm.hpp"

// ============================================================
// Scheduler Implementation — cooperative context switch via
// callee-saved register frame on the kernel stack.
// ============================================================

static Task   s_tasks[MAX_TASKS];
static uint32_t s_count   = 0;
static uint32_t s_current = 0;
static uint32_t s_ticks   = 0;

// memset-equivalent for stack zeroing without libc.
static void zero_mem(void* p, size_t n) {
    uint8_t* b = (uint8_t*)p;
    for (size_t i = 0; i < n; ++i) b[i] = 0;
}
static void copy_str(char* dst, const char* src, size_t max) {
    size_t i = 0;
    for (; i < max - 1 && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

// Each task's initial stack must look as if context_switch() was
// called with that task sitting inside its own first invocation.
// context_switch pops: r15, r14, r13, r12, rbp, rbx → ret → entry_fn
// So we pre-seed the stack with zeros for those 6 regs + entry_fn ptr.
static void setup_initial_stack(Task& t, void (*entry)()) {
    // Allocate kernel stack from heap.
    t.stack_base = (uint8_t*)kmalloc(TASK_STACK_PAGES * 4096);
    if (!t.stack_base) return;
    zero_mem(t.stack_base, TASK_STACK_PAGES * 4096);

    uint64_t* sp = (uint64_t*)(t.stack_base + TASK_STACK_PAGES * 4096);
    // Stack grows downward; push in reverse order.
    *--sp = (uint64_t)entry; // ret address  → entry function
    *--sp = 0;               // rbx
    *--sp = 0;               // rbp
    *--sp = 0;               // r12
    *--sp = 0;               // r13
    *--sp = 0;               // r14
    *--sp = 0;               // r15
    t.rsp = (uint64_t)sp;
}

void sched_init() {
    // Task 0 is the current (boot) context.
    Task& idle    = s_tasks[0];
    idle.id       = 0;
    idle.state    = TaskState::RUNNING;
    idle.stack_base = nullptr; // boot stack (allocated by firmware)
    idle.rsp      = 0;         // filled on first context_switch out
    copy_str((char*)idle.name, "idle", sizeof(idle.name));
    s_count   = 1;
    s_current = 0;
}

uint32_t sched_create_task(void (*entry_fn)(), const char* name) {
    if (s_count >= MAX_TASKS) return 0xFFFFFFFF;

    Task& t   = s_tasks[s_count];
    t.id      = s_count;
    t.state   = TaskState::READY;
    copy_str((char*)t.name, name, sizeof(t.name));
    setup_initial_stack(t, entry_fn);
    if (!t.stack_base) return 0xFFFFFFFF;

    return s_count++;
}

void sched_tick() {
    ++s_ticks;
    if (s_ticks % SCHED_SLICE != 0) return; // not yet time to switch

    if (s_count <= 1) return; // only one task — nothing to switch to

    // Mark current task as READY (unless it killed itself).
    if (s_tasks[s_current].state == TaskState::RUNNING)
        s_tasks[s_current].state = TaskState::READY;

    // Round-robin: find next READY task.
    uint32_t next = (s_current + 1) % s_count;
    for (uint32_t i = 0; i < s_count; ++i) {
        if (s_tasks[next].state == TaskState::READY) break;
        next = (next + 1) % s_count;
    }
    if (next == s_current) {
        s_tasks[s_current].state = TaskState::RUNNING;
        return; // no other READY task
    }

    uint32_t prev = s_current;
    s_current     = next;
    s_tasks[next].state = TaskState::RUNNING;

    context_switch(&s_tasks[prev].rsp, s_tasks[next].rsp);
    // Returns here when this task is scheduled again.
}

uint32_t sched_current_id() { return s_current; }
uint32_t sched_task_count()  { return s_count; }
