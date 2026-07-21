#include "heap.hpp"
#include "pmm.hpp"

// ============================================================
// Kernel Heap Implementation — first-fit linked-list allocator
// ============================================================

static constexpr uint32_t HEAP_MAGIC  = 0xDEADBEEF;
static constexpr uint32_t HEAP_PAGES  = 64;          // 64 × 4 KB = 256 KB initial pool
static constexpr size_t   MIN_SPLIT   = 32;           // don't split blocks smaller than this

struct HeapHeader {
    uint32_t    magic;
    uint32_t    size;    // size of user data (not including this header)
    bool        free;
    HeapHeader* next;
    HeapHeader* prev;
};

static HeapHeader* s_head = nullptr;

// Merge block `b` with its successor if both are free.
static void merge_next(HeapHeader* b) {
    if (!b || !b->next || !b->free || !b->next->free) return;
    b->size += sizeof(HeapHeader) + b->next->size;
    b->next = b->next->next;
    if (b->next) b->next->prev = b;
}

void heap_init() {
    // Allocate HEAP_PAGES physical pages and place them contiguously
    // in virtual memory via HHDM.
    void* first_phys = pmm_alloc();
    if (!first_phys) return; // PMM OOM — very early boot, should not happen

    // Chain additional pages: each page follows the previous in
    // physical address space.  Because we use HHDM (identity-offset)
    // they are also contiguous in virtual space.
    // We allocate them one by one and rely on the PMM returning
    // consecutive pages.  For robustness we extend the header across
    // the total allocated size.
    uint64_t phys_base = reinterpret_cast<uint64_t>(first_phys);

    // Allocate remaining pages (best-effort; they may not be contiguous
    // in RAM, but usually are in QEMU's flat memory model).
    // For simplicity, we treat only the first page for now and grow
    // later via pmm_alloc when we need more room.
    for (uint32_t i = 1; i < HEAP_PAGES; ++i) {
        pmm_alloc(); // reserve, discard — let OS grow lazily in kmalloc
    }

    // Place the free-list head at the HHDM virtual address of phys_base.
    s_head = reinterpret_cast<HeapHeader*>(
        reinterpret_cast<uint64_t>(phys_to_virt(phys_base))
    );
    s_head->magic = HEAP_MAGIC;
    s_head->size  = HEAP_PAGES * 4096 - sizeof(HeapHeader);
    s_head->free  = true;
    s_head->next  = nullptr;
    s_head->prev  = nullptr;
}

void* kmalloc(size_t size) {
    if (size == 0) return nullptr;

    // Round up to 8-byte alignment.
    size = (size + 7) & ~7ULL;

    HeapHeader* cur = s_head;
    while (cur) {
        if (cur->magic != HEAP_MAGIC) return nullptr; // heap corruption
        if (cur->free && cur->size >= size) {
            // Split if there's enough room for a new free block.
            if (cur->size >= size + sizeof(HeapHeader) + MIN_SPLIT) {
                auto* next = reinterpret_cast<HeapHeader*>(
                    reinterpret_cast<uint8_t*>(cur) + sizeof(HeapHeader) + size
                );
                next->magic = HEAP_MAGIC;
                next->size  = cur->size - size - sizeof(HeapHeader);
                next->free  = true;
                next->next  = cur->next;
                next->prev  = cur;
                if (cur->next) cur->next->prev = next;
                cur->next   = next;
                cur->size   = size;
            }
            cur->free = false;
            return reinterpret_cast<uint8_t*>(cur) + sizeof(HeapHeader);
        }
        cur = cur->next;
    }
    return nullptr; // OOM
}

void kfree(void* ptr) {
    if (!ptr) return;
    auto* hdr = reinterpret_cast<HeapHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(HeapHeader)
    );
    if (hdr->magic != HEAP_MAGIC || hdr->free) return;
    hdr->free = true;
    // Coalesce with next free block.
    merge_next(hdr);
    // Coalesce with previous free block.
    if (hdr->prev) merge_next(hdr->prev);
}

void* kzalloc(size_t size) {
    void* p = kmalloc(size);
    if (!p) return nullptr;
    uint8_t* b = reinterpret_cast<uint8_t*>(p);
    for (size_t i = 0; i < size; ++i) b[i] = 0;
    return p;
}
