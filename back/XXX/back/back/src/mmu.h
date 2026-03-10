#ifndef mmu_h
#define mmu_h

#include <stdint.h>

#include "gb.h"
#include "cart.h"

typedef struct {
    struct gb *gb;
    struct gb_cart *cart;
} MMU;

extern MMU mmu;

uint8_t gbMemoryRead(MMU *mmu, uint16_t addr);
void gbMemoryWrite(MMU *mmu, uint16_t addr, uint8_t value);

#endif
