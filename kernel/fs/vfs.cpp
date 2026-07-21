#include "vfs.hpp"

// Static file table
static VFSNode s_files[MAX_VFS_FILES];

static bool streql(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        ++a; ++b;
    }
    return *a == *b;
}

static void strncpy_local(char* dst, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n - 1 && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static const char sysinfo_content[] = 
    "NovaOS Version 1.0 (x86-64 UEFI)\n"
    "Author: Parth Chittalwar\n"
    "Subsystems: GDT, IDT, PMM, Heap, PIC, PIT, Keyboard, Scheduler, VFS, Shell\n";

static const char readme_content[] = 
    "Welcome to NovaOS!\n"
    "This is a custom freestanding 64-bit kernel booted via Limine.\n"
    "Type 'help' in the shell to explore available commands.\n";

static const char credits_content[] = 
    "NovaOS Architecture:\n"
    "- Bootloader: Limine 8.x\n"
    "- Architecture: x86-64 Long Mode\n"
    "- Executable Format: ELF64\n"
    "- Memory: Higher-half kernel mapping with PMM bitmap & Heap allocator\n";

void vfs_init() {
    for (size_t i = 0; i < MAX_VFS_FILES; ++i) s_files[i].used = false;

    vfs_create_file("sysinfo.txt", sysinfo_content, sizeof(sysinfo_content) - 1);
    vfs_create_file("readme.txt",  readme_content,  sizeof(readme_content) - 1);
    vfs_create_file("credits.txt", credits_content, sizeof(credits_content) - 1);
}

bool vfs_create_file(const char* name, const char* data, size_t size) {
    for (size_t i = 0; i < MAX_VFS_FILES; ++i) {
        if (!s_files[i].used) {
            strncpy_local(s_files[i].name, name, MAX_FILENAME);
            s_files[i].data = data;
            s_files[i].size = size;
            s_files[i].used = true;
            return true;
        }
    }
    return false;
}

const VFSNode* vfs_open(const char* filename) {
    for (size_t i = 0; i < MAX_VFS_FILES; ++i) {
        if (s_files[i].used && streql(s_files[i].name, filename)) {
            return &s_files[i];
        }
    }
    return nullptr;
}

size_t vfs_list(const VFSNode** out_nodes, size_t max_out) {
    size_t count = 0;
    for (size_t i = 0; i < MAX_VFS_FILES && count < max_out; ++i) {
        if (s_files[i].used) {
            out_nodes[count++] = &s_files[i];
        }
    }
    return count;
}
