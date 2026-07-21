#pragma once
#include "task.hpp"
#include <stdint.h>

// ============================================================
// Round-Robin Scheduler — NovaOS Milestone 5
// ============================================================

static constexpr uint32_t MAX_TASKS   = 16;
static constexpr uint32_t SCHED_HZ    = 100; // scheduler PIT frequency
static constexpr uint32_t SCHED_SLICE = 2;   // ticks per time-slice

// Initialise the scheduler.  Must be called after heap_init().
// The calling context becomes task 0 ("idle").
void sched_init();

// Create a new kernel task.  Returns task id, or 0xFFFFFFFF on failure.
// entry_fn : C function to run as the task's main loop
// name     : human-readable label (max 23 chars)
uint32_t sched_create_task(void (*entry_fn)(), const char* name);

// Called from the PIT IRQ handler each tick.
// Decides whether to switch tasks and performs the context switch.
extern "C" void sched_tick();

// Current running task id.
uint32_t sched_current_id();

// Number of tasks alive (READY or RUNNING).
uint32_t sched_task_count();

// Assembly: save old RSP, switch to new RSP, restore regs, ret.
// Defined in context_switch.asm.
extern "C" void context_switch(uint64_t* old_rsp, uint64_t new_rsp);
