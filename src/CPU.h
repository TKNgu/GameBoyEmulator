#ifndef CPU_H
#define CPU_H

#include <stdint.h>

int InitCPU(int (*memoryRead)(uint16_t, uint8_t *), int (*memoryWrite)(uint16_t, uint8_t));
int CPUTick();

#endif
