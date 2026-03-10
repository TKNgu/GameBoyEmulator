#include "CPU.h"
#include <stdbool.h>
#include "string.h"

/*static void (*)(CPU *cpu) Opcodes[0xff];*/

void InitOpcodes();

void InitCPU(CPU *cpu, Memory *memory) {
    InitOpcodes();
    memset(&cpu->registers, 0x00, sizeof(cpu->registers));
    cpu->memory = memory;
}

void CPUStep(CPU *cpu) {
    uint8_t opcode = MemoryRead(cpu->memory, cpu->registers.PC);
    /*Opcodes[opcode](cpu);*/
}

InitOpcode() {
    /*for (size_t index = 0; index < 0xff; index++) {*/
    /*    Opcodes[index] = NotImpl;*/
    /*}*/
}
