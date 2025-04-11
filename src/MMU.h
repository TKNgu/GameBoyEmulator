#ifndef MMU_h
#define MMU_h

#include <stdint.h>

struct MMU { };

int InitMMU(struct MMU *mmu);
void ReleaseMMU(struct MMU *mmu);

int MMURead(struct MMU *mmu, uint16_t address, uint8_t *value);
int MMUWrite(struct MMU *mmu, uint16_t address, uint8_t value);

#endif
