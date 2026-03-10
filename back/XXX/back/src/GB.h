#ifndef GB_h
#define GB_h

#include <stdbool.h>

#include "Memory.h"
#include "CPU.h"

typedef struct {
    bool isRunning;
    Memory memory;
    CPU cpu;
} GB;

void InitGB(GB *gb);
void GBStep(GB *gb);

#endif
