#include "CPU.h"
#include <stdio.h>

#define NUM_INSTRUCTION 0x100

#define BLOCK_MASK 0b11000000
#define BLOCK_0 0b00000000
#define BLOCK_1 0b01000000
#define BLOCK_2 0b10000000
#define BLOCK_3 0b11000000

uint8_t opcode;
int (*cpuInstruction[NUM_INSTRUCTION])(struct CPU *, struct MMU *);

uint8_t cbOpcode;
int (*cpuCBInstruction[NUM_INSTRUCTION])(struct CPU *, struct MMU *);

int NOP(struct CPU *, struct MMU *) {
    printf("NOP\n");
    return 0;
}

void InitInstruction() {
    printf("Init Instruction\n");
    uint8_t blockId;
    for (size_t index = 0; index < NUM_INSTRUCTION; index++) {
        blockId = index & BLOCK_MASK;
        if (blockId == BLOCK_0) {
            if (index = 0b00000000) {
                cpuInstruction[index] = NOP;
                continue;
            }

        } else if (blockId == BLOCK_1) {

        } else if (blockId == BLOCK_2) {

        } else if (blockId == BLOCK_3) {
        }
    }
}

int InitCPU(struct CPU *cpu) {
    InitInstruction();
    cpu->pc = 0x00;
    return 0;
}

int CPUTick(struct CPU *cpu, struct MMU *mmu) {
    if (MMURead(mmu, cpu->pc, &opcode)) {
        return 1;
    }
    cpu->pc++;
    return cpuInstruction[opcode](cpu, mmu);
}
