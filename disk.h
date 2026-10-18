#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stddef.h>

// ATA IDE Ports (Primary Bus)
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMM_STAT    0x1F7

// CCFS Attributes
constexpr uint8_t CCFS_ATTR_FILE = 0x00;
constexpr uint8_t CCFS_ATTR_DIR  = 0x01;

#pragma pack(push, 1)
struct DiskFileEntry {
    char name[24];
    uint32_t start_sector;
    uint32_t byte_size;
    uint8_t flags;       // 0x00 = File, 0x01 = Directory
    uint8_t parent_id;   // 0 = Root (;)
    uint16_t reserved;
    uint32_t used;       // 1 = Active, 0 = Empty
};

struct DiskDirectorySector {
    DiskFileEntry files[16]; // 16 entries * 32 bytes = 512 bytes
};
#pragma pack(pop)

// Port I/O Wrappers
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

inline void ata_wait_busy() {
    while (inb(ATA_PRIMARY_COMM_STAT) & 0x80);
}

inline void ata_wait_ready() {
    while (!(inb(ATA_PRIMARY_COMM_STAT) & 0x08));
}

inline void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_busy();
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMM_STAT, 0x20); // Read Command

    ata_wait_busy();
    ata_wait_ready();

    uint16_t* ptr = reinterpret_cast<uint16_t*>(buffer);
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PRIMARY_DATA);
    }
}

inline void ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    ata_wait_busy();
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMM_STAT, 0x30); // Write Command

    ata_wait_busy();
    ata_wait_ready();

    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(buffer);
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_DATA, ptr[i]);
    }
}

#endif