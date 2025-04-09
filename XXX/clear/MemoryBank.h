#ifndef MemoryBank_h
#define MemoryBank_h

#include <stdint.h>
#include <stddef.h>

struct MemoryBank {
    uint8_t **memoryBank;
    size_t size;
};

int InitMemoryBank(struct MemoryBank *memoryBank, size_t size, size_t num);
void ReleaseMemoryBank(struct MemoryBank *memoryBank);
int MemoryBankRealloc(struct MemoryBank *memoryBank, size_t size, size_t num);

#endif
