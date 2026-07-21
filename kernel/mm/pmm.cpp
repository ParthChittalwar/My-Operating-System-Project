#include "pmm.hpp"
#include "../../limine.h"

// ============================================================
// PMM Implementation — bitmap allocator
// ============================================================

// 128 KB bitmap → 1 048 576 bits → 1 048 576 pages × 4 KB = 4 GB
static constexpr uint64_t BITMAP_QWORDS = 16384; // 16384 × 64 = 1 M bits
static uint64_t bitmap[BITMAP_QWORDS];

static uint64_t s_total_pages = 0;
static uint64_t s_free_pages  = 0;
uint64_t        g_hhdm_offset  = 0; // global, accessed via pmm.hpp inline helpers

// Bitmap helpers (0-indexed page number)
static inline void  bit_set  (uint64_t p) { bitmap[p/64] |=  (1ULL << (p % 64)); }
static inline void  bit_clear(uint64_t p) { bitmap[p/64] &= ~(1ULL << (p % 64)); }
static inline bool  bit_test (uint64_t p) { return bitmap[p/64] & (1ULL << (p % 64)); }

uint64_t pmm_hhdm_offset() { return g_hhdm_offset; }

void pmm_init(uint64_t hhdm_off, void* raw_entries, uint64_t count) {
    g_hhdm_offset = hhdm_off;

    // Start with all pages marked as used.
    for (uint64_t i = 0; i < BITMAP_QWORDS; ++i) bitmap[i] = 0;

    auto* entries = reinterpret_cast<limine_memmap_entry**>(raw_entries);

    // First pass: count total pages and find the highest address.
    for (uint64_t i = 0; i < count; ++i) {
        auto* e = entries[i];
        uint64_t last_page = (e->base + e->length) / 4096;
        if (last_page > s_total_pages) s_total_pages = last_page;
    }
    if (s_total_pages > BITMAP_QWORDS * 64) s_total_pages = BITMAP_QWORDS * 64;

    // Second pass: mark usable regions as free.
    for (uint64_t i = 0; i < count; ++i) {
        auto* e = entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t base  = (e->base + 0xFFF) & ~0xFFFULL; // align up to 4 KB
        uint64_t limit = (e->base + e->length) & ~0xFFFULL;
        for (uint64_t addr = base; addr < limit; addr += 4096) {
            uint64_t page = addr / 4096;
            if (page < BITMAP_QWORDS * 64) {
                bit_set(page);
                ++s_free_pages;
            }
        }
    }

    // Reserve the first 1 MB unconditionally (BIOS/firmware legacy area).
    for (uint64_t p = 0; p < 256; ++p) { // 256 pages × 4KB = 1 MB
        if (bit_test(p)) { bit_clear(p); --s_free_pages; }
    }
}

void* pmm_alloc() {
    // Linear scan for a free bit.
    for (uint64_t qi = 0; qi < BITMAP_QWORDS; ++qi) {
        if (bitmap[qi] == 0) continue; // no free pages in this qword
        // Find the lowest set bit.
        uint64_t bit = __builtin_ctzll(bitmap[qi]);
        uint64_t page = qi * 64 + bit;
        bit_clear(page);
        --s_free_pages;
        return reinterpret_cast<void*>(page * 4096);
    }
    return nullptr; // OOM
}

void pmm_free(void* phys_addr) {
    uint64_t page = reinterpret_cast<uint64_t>(phys_addr) / 4096;
    if (page >= BITMAP_QWORDS * 64) return;
    if (!bit_test(page)) {
        bit_set(page);
        ++s_free_pages;
    }
}

uint64_t pmm_free_pages()  { return s_free_pages; }
uint64_t pmm_total_pages() { return s_total_pages; }
uint64_t pmm_used_bytes()  { return (s_total_pages - s_free_pages) * 4096; }
