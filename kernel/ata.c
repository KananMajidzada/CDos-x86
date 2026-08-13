#include "ata.h"
#include "io.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

#define STATUS_BSY      0x80
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01

static int ata_poll_ready(void)
{
    /* wait for BSY to clear */
    int timeout = 1000000;
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS);
        if (!(status & STATUS_BSY)) {
            if (status & STATUS_ERR) return -1;
            if (status & STATUS_DRQ) return 0;
        }
    }
    return -1; /* timeout */
}

int ata_init(void)
{
    /* select master drive, LBA mode */
    outb(ATA_DRIVE_HEAD, 0xE0);
    io_wait();
    return 0;
}

int ata_read_sector(uint32_t lba, uint8_t *buffer)
{
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (ata_poll_ready() != 0) return -1;

    for (int i = 0; i < 256; i++) {
        uint16_t data;
        __asm__ volatile ("inw %1, %0" : "=a"(data) : "Nd"((uint16_t)ATA_DATA));
        buffer[i * 2]     = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)((data >> 8) & 0xFF);
    }

    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t *buffer)
{
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (ata_poll_ready() != 0) return -1;

    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"((uint16_t)ATA_DATA));
    }

    /* flush cache */
    outb(ATA_COMMAND, 0xE7);
    ata_poll_ready();

    return 0;
}
