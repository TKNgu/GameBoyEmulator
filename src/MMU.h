#ifndef MMU_h
#define MMU_h

#include <stdint.h>

struct MMU { };

int InitMMU(struct MMU *mmu);
void ReleaseMMU(struct MMU *mmu);

int MMURead8(struct MMU *mmu, uint16_t address, uint8_t *value);
int MMUWrite8(struct MMU *mmu, uint16_t address, uint8_t value);

int MMURead16(struct MMU *mmu, uint16_t address, uint16_t *value);
int MMUWrite16(struct MMU *mmu, uint16_t address, uint16_t value);

#endif
