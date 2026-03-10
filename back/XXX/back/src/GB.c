#include "GB.h"

void InitGB(GB *gb) {
    gb->isRunning = true;
    InitCPU(&gb->cpu, &gb->memory);
}

void GBStep(GB *gb) {
    CPUStep(&gb->cpu);
}
