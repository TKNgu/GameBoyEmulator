#include "MMU.h"

#define MEMORY_SIZE 0x10000

int InitMMU(struct MMU *mmu) {
    return 0;
}

void ReleaseMMU(struct MMU *mmu) {
    return;
}

int MMURead(struct MMU *mmu, uint16_t address, uint8_t *value) {
    return 0;
}

int MMUWrite(struct MMU *mmu, uint16_t address, uint8_t value) {
    return 0;
}
