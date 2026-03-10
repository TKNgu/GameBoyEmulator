#ifndef Memory_h
#define Memory_h

#include <stdint.h>

typedef struct {
} Memory;

uint8_t MemoryRead(Memory *memory, uint16_t address);
void MemoryWrite(Memory *memory, uint16_t address, uint8_t value);

#endif
