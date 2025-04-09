#include <stdbool.h>
#include <stdio.h>

#include "CPU.h"

int MemoryRead(uint16_t address, uint8_t *value);
int MemoryWrite(uint16_t address, uint8_t value);

int main() {
  if (InitCPU(MemoryRead, MemoryWrite)) {
    printf("Error Init CPU\n");
  }

  while (!CPUTick()) {
  }

  return 0;
}

int MemoryRead(uint16_t address, uint8_t *value) { return 0; }

int MemoryWrite(uint16_t address, uint8_t value) { return 0; }
