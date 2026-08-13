#include "fs.h"
#include "ata.h"
#include "string.h"

static struct fs_header header;
static int mounted = 0;

int fs_mount(void)
{
    uint8_t sector[512];

    if (ata_read_sector(0, sector) != 0) {
        mounted = 0;
        return -1;
    }

    /* sector is raw bytes on disk; cast to header layout, then copy
       into our persistent global (sector[] is stack-local and about to
       go out of scope) */
    struct fs_header *h = (struct fs_header *)sector;

    if (h->magic != FS_MAGIC) {
        mounted = 0;
        return -2; /* not formatted / bad magic */
    }

    header.magic = h->magic;
    header.entry_count = h->entry_count;
    for (uint32_t i = 0; i < header.entry_count && i < FS_MAX_ENTRIES; i++) {
        header.entries[i] = h->entries[i];
    }

    mounted = 1;
    return 0;
}

int fs_find(const char *name, struct fs_entry *out)
{
    if (!mounted) return -1;

    for (uint32_t i = 0; i < header.entry_count && i < FS_MAX_ENTRIES; i++) {
        if (k_strncasecmp(header.entries[i].name, name, FS_NAME_LEN) == 0) {
            *out = header.entries[i];
            return 0;
        }
    }
    return -1;
}

int fs_list(void (*callback)(const struct fs_entry *))
{
    if (!mounted) return -1;

    for (uint32_t i = 0; i < header.entry_count && i < FS_MAX_ENTRIES; i++) {
        callback(&header.entries[i]);
    }
    return 0;
}

int fs_read_file(const struct fs_entry *entry, uint8_t *buffer, uint32_t max_len)
{
    uint32_t sectors_needed = (entry->length + 511) / 512;
    uint32_t bytes_copied = 0;

    for (uint32_t i = 0; i < sectors_needed; i++) {
        uint8_t sector[512];
        if (ata_read_sector(entry->start_lba + i, sector) != 0) {
            return -1;
        }

        uint32_t remaining = entry->length - bytes_copied;
        uint32_t chunk = (remaining < 512) ? remaining : 512;
        if (bytes_copied + chunk > max_len) {
            chunk = max_len - bytes_copied;
        }

        for (uint32_t j = 0; j < chunk; j++) {
            buffer[bytes_copied + j] = sector[j];
        }
        bytes_copied += chunk;

        if (bytes_copied >= max_len) break;
    }

    return (int)bytes_copied;
}
