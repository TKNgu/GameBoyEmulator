#include "MMU.h"

#define MEMORY_SIZE 0x10000

int InitMMU(struct MMU *mmu) {
    return 0;
}

void ReleaseMMU(struct MMU *mmu) {
    return;
}

int MMURead8(struct MMU *mmu, uint16_t address, uint8_t *value) {
    return 0;
}

int MMUWrite8(struct MMU *mmu, uint16_t address, uint8_t value) {
    return 0;
}

int MMURead16(struct MMU *mmu, uint16_t address, uint16_t *value) {
    uint8_t low;
    if (MMURead8(mmu, address, &low)) {
        return 1;
    }
    uint8_t hight;
    if (MMURead8(mmu, ++address, &hight)) {
        return 1;
    }
    *value = hight;
    *value = ((*value) << 8) | low;
    return 0;
}

int MMUWrite16(struct MMU *mmu, uint16_t address, uint16_t value) {
    if (MMUWrite8(mmu, address, (uint8_t)(value))) {
        return 1;
    }
    if (MMUWrite8(mmu, ++address, (uint8_t)(value >> 8))) {
        return 1;
    }
    return 0;
}
