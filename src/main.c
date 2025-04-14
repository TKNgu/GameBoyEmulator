#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "CPU.h"
#include "MMU.h"

int main() {
    struct CPU *cpu = (struct CPU *)malloc(sizeof(struct CPU));
    if (!cpu) {
        return 0;
    }
    if (InitCPU(cpu)) {
        goto RELEASE_CPU;
    }
    struct MMU *mmu = (struct MMU *)malloc(sizeof(struct MMU));
    if (!mmu) {
        goto RELEASE_CPU;
    }
    if (InitMMU(mmu)) {
        goto RELEASE_MMU;
    }

    bool isRunning = true;
    while (isRunning) {
        if (CPUTick(cpu, mmu)) {
            isRunning = false;
        }
    }

    ReleaseMMU(mmu);

RELEASE_MMU:
    free(mmu);

RELEASE_CPU:
    free(cpu);
    return 0;
}
