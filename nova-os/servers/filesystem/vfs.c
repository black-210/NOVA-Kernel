/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: servers/filesystem/vfs.c
 * Description: Virtual File System Abstraction Layer.
 * Mounts rootfs, devfs, and novafs into a unified hierarchical namespace.
 * License: Apache-2.0
 */

#include "vfs.h"
#include "kprintf.h"
#include "heap.h"

extern void novafs_init(void);
extern int  novafs_create_file(const char *name, const void *initial_data, size_t size);
extern size_t novafs_read_file(int inode_id, uint64_t offset, size_t size, uint8_t *out_buffer);

#define MAX_VFS_NODES 64
static vfs_node_t vfs_table[MAX_VFS_NODES];
static uint32_t vfs_count = 0;

static size_t vfs_default_read(vfs_node_t *node, uint64_t offset, size_t size, uint8_t *buffer) {
    if (!node || !buffer) return 0;
    return novafs_read_file((int)node->inode_num, offset, size, buffer);
}

void vfs_init(void) {
    kmemset(vfs_table, 0, sizeof(vfs_table));
    vfs_count = 0;

    novafs_init();

    // Create root directory "/"
    vfs_node_t *root = &vfs_table[vfs_count++];
    kstrncpy(root->name, "/", sizeof(root->name));
    root->flags = FS_DIRECTORY;
    root->inode_num = 0;
    root->permissions = 0755;

    // Populate initial system files into NOVA-FS
    const char *motd = "NOVA OS v1.0-LTS [Autonomous Microkernel Architecture]\nAll subsystems verified. Capability sandboxing enforced.\n";
    int motd_inode = novafs_create_file("motd", motd, kstrlen(motd));

    vfs_node_t *motd_node = &vfs_table[vfs_count++];
    kstrncpy(motd_node->name, "/etc/motd", sizeof(motd_node->name));
    motd_node->flags = FS_FILE;
    motd_node->inode_num = motd_inode >= 0 ? (uint32_t)motd_inode : 0;
    motd_node->size = (uint32_t)kstrlen(motd);
    motd_node->permissions = 0644;
    motd_node->read = vfs_default_read;

    kprintf("[VFS] Unified Virtual File System mounted (/ [rootfs], /etc/motd [novafs])\n");
}

vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path) return NULL;
    for (uint32_t i = 0; i < vfs_count; i++) {
        if (kstrcmp(vfs_table[i].name, path) == 0) {
            return &vfs_table[i];
        }
    }
    return NULL;
}

size_t vfs_read(vfs_node_t *node, uint64_t offset, size_t size, uint8_t *buf) {
    if (!node || !node->read) return 0;
    return node->read(node, offset, size, buf);
}
