/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/vfs.h
 * Description: Virtual File System (VFS) abstractions, inodes, and mount points.
 * License: Apache-2.0
 */

#ifndef NOVA_VFS_H
#define NOVA_VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05

struct vfs_node;

typedef size_t (*vfs_read_fn)(struct vfs_node *node, uint64_t offset, size_t size, uint8_t *buffer);
typedef size_t (*vfs_write_fn)(struct vfs_node *node, uint64_t offset, size_t size, const uint8_t *buffer);
typedef struct vfs_node *(*vfs_finddir_fn)(struct vfs_node *node, const char *name);

typedef struct vfs_node {
    char            name[64];
    uint32_t        flags;
    uint32_t        inode_num;
    uint32_t        size;
    uint32_t        permissions;
    uint32_t        uid;
    uint32_t        gid;
    vfs_read_fn     read;
    vfs_write_fn    write;
    vfs_finddir_fn  finddir;
    struct vfs_node *ptr; /* Implementation-specific pointer */
} vfs_node_t;

void vfs_init(void);
vfs_node_t *vfs_resolve_path(const char *path);
size_t vfs_read(vfs_node_t *node, uint64_t offset, size_t size, uint8_t *buf);
size_t vfs_write(vfs_node_t *node, uint64_t offset, size_t size, const uint8_t *buf);

#endif /* NOVA_VFS_H */
