#ifndef FS_H
#define FS_H
#include <stdint.h>

#define FS_MAGIC 0x43444653  /* "CDFS" in little-endian bytes */
#define FS_MAX_ENTRIES 21
#define FS_NAME_LEN 16

struct fs_entry {
    char name[FS_NAME_LEN];
    uint32_t start_lba;
    uint32_t length;
} __attribute__((packed));

struct fs_header {
    uint32_t magic;
    uint32_t entry_count;
    struct fs_entry entries[FS_MAX_ENTRIES];
} __attribute__((packed));

int fs_mount(void);
int fs_find(const char *name, struct fs_entry *out);
int fs_list(void (*callback)(const struct fs_entry *));
int fs_read_file(const struct fs_entry *entry, uint8_t *buffer, uint32_t max_len);

#endif
