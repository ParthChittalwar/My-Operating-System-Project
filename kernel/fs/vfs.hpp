#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// Virtual File System (VFS) & In-Memory RAM Disk — M6
// ============================================================

static constexpr size_t MAX_VFS_FILES = 16;
static constexpr size_t MAX_FILENAME  = 32;

struct VFSNode {
    char        name[MAX_FILENAME];
    const char* data;
    size_t      size;
    bool        used;
};

// Initialise the Virtual File System with built-in system files.
void vfs_init();

// Find a file by name. Returns nullptr if not found.
const VFSNode* vfs_open(const char* filename);

// List all files in the VFS. Returns count.
size_t vfs_list(const VFSNode** out_nodes, size_t max_out);

// Register a new file in RAM disk.
bool vfs_create_file(const char* name, const char* data, size_t size);
