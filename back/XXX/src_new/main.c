#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include "CPU.h"
#include "MMU.h"
#include "Sleep.h"

#define CPU_HZ 4194304
#define CPU_X2HZ 8388608
#define TIME_CPU_TICK 238.4185791

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
    double paddingTime = 0;
    struct timespec start;
    struct timespec now;

    while (isRunning) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        size_t countTick = 0;
        CPUTick(cpu, mmu, &countTick);
        double runTime = paddingTime + countTick * TIME_CPU_TICK;
        int sleepTime = (int)runTime;
        paddingTime = runTime - sleepTime;
        NanoSleep(sleepTime);
        clock_gettime(CLOCK_MONOTONIC, &now);
        // Update lates
    }
    ReleaseMMU(mmu);

RELEASE_MMU:
    free(mmu);

RELEASE_CPU:
    free(cpu);
    return 0;
}
