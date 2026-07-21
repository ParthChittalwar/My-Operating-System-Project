#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// Kernel Heap — NovaOS Milestone 3
//
// First-fit free-list allocator backed by physical pages from
// the PMM, accessed through the HHDM.
//
// Layout of a heap block:
//   [HeapHeader | user data ...]
//
// The header is hidden before the pointer returned to the caller.
// ============================================================

// Initialise the heap with an initial pool of physical pages.
// Must be called after pmm_init().
void heap_init();

// Allocate at least `size` bytes.  Returns nullptr on failure.
void* kmalloc(size_t size);

// Free memory returned by kmalloc.
void  kfree(void* ptr);

// Zeroing allocator (like calloc(1, size)).
void* kzalloc(size_t size);
