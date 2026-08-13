#ifndef ATA_H
#define ATA_H
#include <stdint.h>

/* Returns 0 on success, non-zero on error */
int ata_read_sector(uint32_t lba, uint8_t *buffer);
int ata_write_sector(uint32_t lba, const uint8_t *buffer);
int ata_init(void);

#endif
