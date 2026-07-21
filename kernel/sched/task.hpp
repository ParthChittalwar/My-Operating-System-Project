#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// Task Control Block (TCB) — NovaOS Milestone 5
// ============================================================

static constexpr size_t TASK_STACK_PAGES = 4; // 4 × 4KB = 16 KB per task

enum class TaskState : uint8_t {
    READY   = 0,
    RUNNING = 1,
    BLOCKED = 2,
    DEAD    = 3,
};

struct Task {
    uint64_t   rsp;          // saved stack pointer (when not running)
    uint8_t*   stack_base;   // bottom of allocated kernel stack
    TaskState  state;
    uint32_t   id;
    char name[24];
};
