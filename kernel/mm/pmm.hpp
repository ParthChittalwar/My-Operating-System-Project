#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// Physical Memory Manager (PMM) — NovaOS Milestone 3
//
// Bitmap allocator: one bit per 4 KB physical page.
//   0 = in use / reserved
//   1 = free
//
// Coverage: 128 KB bitmap × 8 bits/byte × 4 KB/page = 4 GB
//
// All functions that return addresses return PHYSICAL addresses.
// To access physical memory from kernel code, add hhdm_offset.
// ============================================================

// Limine HHDM offset — virtual address where physical 0 is mapped.
// Set during pmm_init(); read via pmm_hhdm_offset().
uint64_t pmm_hhdm_offset();

// Convert a physical address to its HHDM-mapped virtual address.
static inline void* phys_to_virt(uint64_t phys) {
    extern uint64_t g_hhdm_offset;
    return reinterpret_cast<void*>(phys + g_hhdm_offset);
}

static inline uint64_t virt_to_phys(void* virt) {
    extern uint64_t g_hhdm_offset;
    return reinterpret_cast<uint64_t>(virt) - g_hhdm_offset;
}

// Initialise the PMM from Limine's memory map response.
// hhdm_off  : value of limine_hhdm_response::offset
// entries   : pointer to limine_memmap_entry array
// count     : number of entries
void pmm_init(uint64_t hhdm_off, void* entries, uint64_t count);

// Allocate one 4 KB physical page.  Returns physical address, or 0 on OOM.
void* pmm_alloc();

// Free a physical page previously returned by pmm_alloc().
void  pmm_free(void* phys_addr);

// Diagnostics
uint64_t pmm_free_pages();
uint64_t pmm_total_pages();
uint64_t pmm_used_bytes();   // roughly (total - free) * 4096
