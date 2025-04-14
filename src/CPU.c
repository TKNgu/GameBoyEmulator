#include "CPU.h"

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

#include "MMU.h"

#define NUM_INSTRUCTION 0x100

#define BLOCK_MASK 0xc0
#define BLOCK_0 0x00
#define BLOCK_1 0x40
#define BLOCK_2 0x80
#define BLOCK_3 0xc0

uint8_t opcode;
int (*cpuInstruction[NUM_INSTRUCTION])(struct CPU *, struct MMU *);

int CPUReadNextI8(struct CPU *cpu, struct MMU *mmu, uint8_t *i8) {
    if (MMURead8(mmu, cpu->pc, i8)) {
        return 1;
    }
    cpu->pc = (cpu->pc + 1) & 0xffff;
    return 0;
}

int CPUReadNextI16(struct CPU *cpu, struct MMU *mmu, uint16_t *i16) {
    if (MMURead16(mmu, cpu->pc, i16)) {
        return 1;
    }
    cpu->pc = (cpu->pc + 2) & 0xffff;
    return 0;
}

uint8_t *DecodeR8(struct CPU *cpu) {
    switch (opcode & 0x38) {
    case 0x00:
        return &cpu->b;
    case 0x08:
        return &cpu->c;
    case 0x10:
        return &cpu->d;
    case 0x18:
        return &cpu->e;
    case 0x20:
        return &cpu->h;
    case 0x28:
        return &cpu->l;
    case 0x30:
        return NULL; // Update late [HL]
    default:         // 0x38:
        return &cpu->a;
    }
}

uint8_t *DecodeR8Source(struct CPU *cpu) {
    switch (opcode & 0x07) {
    case 0x00:
        return &cpu->b;
    case 0x01:
        return &cpu->c;
    case 0x02:
        return &cpu->d;
    case 0x03:
        return &cpu->e;
    case 0x04:
        return &cpu->h;
    case 0x05:
        return &cpu->l;
    case 0x06:
        return NULL; // Update late [HL]
    default:         // 0x07:
        return &cpu->a;
    }
}

uint16_t *DecodeR16(struct CPU *cpu) {
    switch (opcode & 0x30) {
    case 0x00:
        return &cpu->bc;
    case 0x10:
        return &cpu->de;
    case 0x20:
        return &cpu->hl;
    default: // 0x30
        return &cpu->sp;
    }
}

uint16_t *DecodeR16Mem(struct CPU *cpu) {
    static uint16_t r16mem;
    switch (opcode & 0x30) {
    case 0x00:
        return &cpu->bc;
    case 0x10:
        return &cpu->de;
    case 0x20:
        r16mem = cpu->hl;
        cpu->hl++;
        return &r16mem;
    default: // 0x30
        r16mem = cpu->hl;
        cpu->hl--;
        return &r16mem;
    }
}

void RegisterHLAdd16(struct CPU *cpu, uint16_t value) {
    uint32_t wa = (uint32_t)cpu->hl;
    uint32_t wb = (uint32_t)value;
    uint32_t r = wa + wb;

    cpu->f_n = false;
    cpu->f_c = r & 0x10000;
    cpu->f_h = (wa ^ wb ^ r) & 0x1000;
    cpu->hl = (uint16_t)r;
}

uint8_t RegisterINC(struct CPU *cpu, uint8_t value) {
    uint8_t r = (value + 1) & 0xff;
    cpu->f_z = (r == 0);
    cpu->f_n = false;
    cpu->f_h = ((value & 0xf) == 0xf);
    return r;
}

uint8_t RegisterDEC(struct CPU *cpu, uint8_t value) {
    uint8_t r = (value - 1) & 0xff;
    cpu->f_z = (r == 0);
    cpu->f_n = true;
    cpu->f_h = ((value & 0xf) == 0);
    return r;
}

int NOP(struct CPU *, struct MMU *) {
    printf("NOP\n");
    return 0;
}

int LD_R16_IMM16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16 = DecodeR16(cpu);
    return CPUReadNextI16(cpu, mmu, r16);
}

int LD_R16MEM_A(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16mem = DecodeR16Mem(cpu);
    return MMUWrite8(mmu, *r16mem, cpu->a);
}

int LD_A_R16MEM(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16mem = DecodeR16Mem(cpu);
    return MMURead8(mmu, *r16mem, &cpu->a);
}

int LD_IMM16_SP(struct CPU *cpu, struct MMU *mmu) {
    uint16_t imm16;
    if (CPUReadNextI16(cpu, mmu, &imm16)) {
        return 1;
    }
    return MMUWrite16(mmu, imm16, cpu->sp);
}

int INC_R16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16 = DecodeR16(cpu);
    uint16_t value = *r16;
    value = (value + 1) & 0xffff;
    *r16 = value;
    return 0;
}

int DEC_R16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16 = DecodeR16(cpu);
    uint16_t value = *r16;
    value = (value - 1) & 0xffff;
    *r16 = value;
    return 0;
}

int ADD_HL_R16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16 = DecodeR16(cpu);
    RegisterHLAdd16(cpu, *r16);
    return 0;
}

int INC_R8(struct CPU *cpu, struct MMU *mmu) {
    if (((opcode >> 3) & 0x07) == 0x6) {
        uint8_t value;
        MMURead8(mmu, cpu->hl, &value);
        value = RegisterINC(cpu, value);
        MMUWrite8(mmu, cpu->hl, value);
    } else {
        uint8_t *r8 = DecodeR8(cpu);
        uint8_t value = RegisterINC(cpu, *r8);
        *r8 = value;
    }
    return 0;
}

int DEC_R8(struct CPU *cpu, struct MMU *mmu) {
    if (((opcode >> 3) & 0x07) == 0x6) {
        uint8_t value;
        if (MMURead8(mmu, cpu->hl, &value)) {
            return 1;
        }
        value = RegisterDEC(cpu, value);
        if (MMUWrite8(mmu, cpu->hl, value)) {
            return 1;
        }
    } else {
        uint8_t *r8 = DecodeR8(cpu);
        uint8_t value = RegisterDEC(cpu, *r8);
        *r8 = value;
    }
    return 0;
}

int LD_R8_IMM8(struct CPU *cpu, struct MMU *mmu) {
    if (((opcode >> 3) & 0x07) == 0x06) {
        uint8_t value;
        if (CPUReadNextI8(cpu, mmu, &value)) {
            return 1;
        }
        if (MMUWrite8(mmu, cpu->hl, value)) {
            return 1;
        }
    } else {
        uint8_t *r8 = DecodeR8(cpu);
        if (CPUReadNextI8(cpu, mmu, r8)) {
            return 1;
        }
    }
    return 0;
}

int RLCA(struct CPU *cpu, struct MMU *) {
    uint8_t a = cpu->a;
    uint8_t c;

    c = a >> 7;
    a = (a << 1) & 0xff;
    a |= c;

    cpu->a = a;
    cpu->f_z = false;
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;

    return 0;
}

int RRCA(struct CPU *cpu, struct MMU *) {
    uint8_t a = cpu->a;
    uint8_t c;

    c = a & 1;
    a = a >> 1;
    a |= (c << 7);

    cpu->a = a;
    cpu->f_z = false;
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;

    return 0;
}

int RLA(struct CPU *cpu, struct MMU *) {
    uint8_t a = cpu->a;
    uint8_t c = cpu->f_c;

    uint8_t tmp = a >> 7;
    a = (a << 1) & 0xff;
    a |= c;

    cpu->a = a;
    cpu->f_z = false;
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = tmp;

    return 0;
}

int RRA(struct CPU *cpu, struct MMU *) {
    uint8_t a = cpu->a;
    uint8_t c = cpu->f_c;

    uint8_t tmp = a & 1;
    a = a >> 1;
    a |= (c << 7);

    cpu->a = a;
    cpu->f_z = false;
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = tmp;

    return 0;
}

int DAA(struct CPU *cpu, struct MMU *) {
    uint8_t a = cpu->a;
    uint8_t adj = 0;

    if (cpu->f_h) {
        adj |= 0x06;
    }

    if (cpu->f_c) {
        adj |= 0x60;
    }

    if (cpu->f_n) {
        a -= adj;
    } else {
        if ((a & 0xf) > 0x09) {
            adj |= 0x06;
        }

        if (a > 0x99) {
            adj |= 0x60;
        }

        a += adj;
    };

    cpu->a = a;
    cpu->f_z = (a == 0);
    cpu->f_c = ((adj & 0x60) != 0);
    cpu->f_h = false;

    return 0;
}

int CPL(struct CPU *cpu, struct MMU *) {
    cpu->a = ~cpu->a;
    cpu->f_n = true;
    cpu->f_h = true;

    return 0;
}

int SCF(struct CPU *cpu, struct MMU *) {
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = true;

    return 0;
}

int CCF(struct CPU *cpu, struct MMU *) {
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = !cpu->f_c;

    return 0;
}

int JR_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t i8;
    if (CPUReadNextI8(cpu, mmu, &i8)) {
        return 1;
    }
    uint16_t pc = cpu->pc;
    pc += (int8_t)(i8);
    cpu->pc = pc;
    return 0;
}

bool DecodeCond(struct CPU *cpu) {
    switch (opcode & 0x18) {
    case 0x00:
        return !cpu->f_z;
    case 0x08:
        return cpu->f_z;
    case 0x10:
        return !cpu->f_c;
    default: // 0x18:
        return cpu->f_c;
    }
}

int JR_COND_IMM8(struct CPU *cpu, struct MMU *mmu) {
    if (DecodeCond(cpu)) {
        return JR_IMM8(cpu, mmu);
    } else {
        uint8_t tmp;
        return CPUReadNextI8(cpu, mmu, &tmp);
    }
}

int STOP(struct CPU *cpu, struct MMU *mmu) {
    return 0;
}

int HALT(struct CPU *cpu, struct MMU *mmu) {
    return 0;
}

int LD_R8_R8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if ((opcode & 0x07) == 0x06) {
        if (MMURead8(mmu, cpu->hl, &value)) {
            return 1;
        }
    } else {
        uint8_t *src = DecodeR8Source(cpu);
        value = *src;
    }

    if (((opcode >> 3) & 0x07) == 0x06) {
        if (MMUWrite8(mmu, cpu->hl, value)) {
            return 1;
        }
    } else {
        uint8_t *dst = DecodeR8(cpu);
        *dst = value;
    }
    return 0;
}

uint8_t ADD_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint16_t al = a;
    uint16_t bl = b;

    uint16_t r = al + bl;

    cpu->f_z = !(r & 0xff);
    cpu->f_n = false;
    cpu->f_h = (a ^ b ^ r) & 0x10;
    cpu->f_c = r & 0x100;

    return r;
}

uint8_t ADC_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint16_t al = a;
    uint16_t bl = b;
    uint16_t c = cpu->f_c;

    uint16_t r = al + bl + c;

    cpu->f_z = !(r & 0xff);
    cpu->f_n = false;
    cpu->f_h = (a ^ b ^ r) & 0x10;
    cpu->f_c = r & 0x100;

    return r;
}

uint8_t SUB_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint16_t al = a;
    uint16_t bl = b;

    uint16_t r = al - bl;

    cpu->f_z = !(r & 0xff);
    cpu->f_n = true;
    cpu->f_h = (a ^ b ^ r) & 0x10;
    cpu->f_c = r & 0x100;

    return r;
}

uint8_t SBC_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint16_t al = a;
    uint16_t bl = b;
    uint16_t c = cpu->f_c;

    uint16_t r = al - bl - c;

    cpu->f_z = !(r & 0xff);
    cpu->f_n = true;
    cpu->f_h = (a ^ b ^ r) & 0x10;
    cpu->f_c = r & 0x100;

    return r;
}

uint8_t AND_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint8_t r = a & b;

    cpu->f_z = (r == 0);
    cpu->f_n = false;
    cpu->f_h = true;
    cpu->f_c = false;

    return r;
}

uint8_t XOR_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint8_t r = a ^ b;

    cpu->f_z = (r == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = false;

    return r;
}

uint8_t OR_SET_FLAGS(struct CPU *cpu, uint8_t a, uint8_t b) {
    uint8_t r = a | b;

    cpu->f_z = (r == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = false;

    return r;
}

int Opcode_A_R8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t (*core)(struct CPU *, uint8_t, uint8_t);
    switch (opcode & 0xf8) {
    case 0x80:
        core = ADD_SET_FLAGS;
        break;
    case 0x88:
        core = ADC_SET_FLAGS;
        break;
    case 0x90:
        core = SUB_SET_FLAGS;
        break;
    case 0x98:
        core = SBC_SET_FLAGS;
        break;
    case 0xa0:
        core = AND_SET_FLAGS;
        break;
    case 0xa8:
        core = XOR_SET_FLAGS;
        break;
    case 0xb0:
        core = OR_SET_FLAGS;
        break;
    default:
        core = SUB_SET_FLAGS;
        break;
    }
    uint8_t *r8 = DecodeR8Source(cpu);
    uint8_t value;
    if (r8) {
        value = *r8;
    } else {
        if (MMURead8(mmu, cpu->hl, &value)) {
            return 1;
        }
    }
    if ((opcode & 0xf8) != 0xb8) {
        cpu->a = core(cpu, cpu->a, value);
    } else {
        core(cpu, cpu->a, value);
    }
    return 0;
}

int ADD_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = ADD_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int ADC_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = ADC_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int SUB_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = SUB_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int SBC_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = SBC_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int AND_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = AND_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int XOR_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = XOR_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int OR_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    cpu->a = OR_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int CP_A_IMM8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    SUB_SET_FLAGS(cpu, cpu->a, value);
    return 0;
}

int POPB(struct CPU *cpu, struct MMU *mmu, uint8_t *value) {
    if (MMURead8(mmu, cpu->sp, value)) {
        return 1;
    }
    cpu->sp = (cpu->sp + 1) & 0xffff;
    return 0;
}

int POPW(struct CPU *cpu, struct MMU *mmu, uint16_t *value) {
    uint8_t low;
    if (POPB(cpu, mmu, &low)) {
        return 1;
    }
    uint8_t high;
    if (POPB(cpu, mmu, &high)) {
        return 1;
    }
    *value = high;
    *value = low | ((*value) << 8);
    return 0;
}

int RET(struct CPU *cpu, struct MMU *mmu) {
    return POPW(cpu, mmu, &cpu->pc);
}

int RETi(struct CPU *cpu, struct MMU *mmu) {
    return RET(cpu, mmu);
}

int RET_COND(struct CPU *cpu, struct MMU *mmu) {
    if (DecodeCond(cpu)) {
        return RET(cpu, mmu);
    }
    return 0;
}

int JP_IMM16(struct CPU *cpu, struct MMU *mmu) {
    return CPUReadNextI16(cpu, mmu, &cpu->pc);
}

int JP_COND_IMM16(struct CPU *cpu, struct MMU *mmu) {
    if (DecodeCond(cpu)) {
        return JP_IMM16(cpu, mmu);
    } else {
        uint16_t tmp;
        return CPUReadNextI16(cpu, mmu, &tmp);
    }
}

void InitInstructionFixed() {
    cpuInstruction[0x00] = NOP;
    cpuInstruction[0x08] = LD_IMM16_SP;
    cpuInstruction[0x07] = RLCA;
    cpuInstruction[0x0f] = RRCA;
    cpuInstruction[0x17] = RLA;
    cpuInstruction[0x1f] = RRA;
    cpuInstruction[0x27] = DAA;
    cpuInstruction[0x2f] = CPL;
    cpuInstruction[0x37] = SCF;
    cpuInstruction[0x3f] = CCF;
    cpuInstruction[0x18] = JR_IMM8;
    cpuInstruction[0x10] = STOP;

    cpuInstruction[0xc6] = ADD_A_IMM8;
    cpuInstruction[0xce] = ADC_A_IMM8;
    cpuInstruction[0xd6] = SUB_A_IMM8;
    cpuInstruction[0xde] = SBC_A_IMM8;
    cpuInstruction[0xe6] = AND_A_IMM8;
    cpuInstruction[0xee] = XOR_A_IMM8;
    cpuInstruction[0xf6] = OR_A_IMM8;
    cpuInstruction[0xfe] = CP_A_IMM8;

    cpuInstruction[0xc9] = RET;
    cpuInstruction[0xd9] = RETi;
}

void InitInstruction() {
    printf("Init Instruction\n");
    uint8_t blockId;
    for (uint16_t index = 0; index < NUM_INSTRUCTION; index++) {
        blockId = index & BLOCK_MASK;
        uint8_t tmp;
        if (blockId == BLOCK_0) {
            tmp = index & 0x0f;
            if (tmp == 0x01) {
                cpuInstruction[index] = LD_R16_IMM16;
                continue;
            }
            if (tmp == 0x20) {
                cpuInstruction[index] = LD_R16MEM_A;
                continue;
            }
            if (tmp == 0xa0) {
                cpuInstruction[index] = LD_A_R16MEM;
                continue;
            }
            if (tmp == 0x03) {
                cpuInstruction[index] = INC_R16;
                continue;
            }
            if (tmp == 0x0b) {
                cpuInstruction[index] = DEC_R16;
                continue;
            }
            if (tmp == 0x09) {
                cpuInstruction[index] = ADD_HL_R16;
                continue;
            }
            tmp = index & 0x07;
            if (tmp == 0x04) {
                cpuInstruction[index] = INC_R8;
                continue;
            }
            if (tmp == 0x05) {
                cpuInstruction[index] = DEC_R8;
                continue;
            }
            if (tmp == 0x06) {
                cpuInstruction[index] = LD_R8_IMM8;
                continue;
            }
            if (tmp == 0x20) {
                cpuInstruction[index] = JR_COND_IMM8;
                continue;
            }
        } else if (blockId == BLOCK_1) {
            if (index == 0x76) {
                cpuInstruction[index] = HALT;
                continue;
            }
            tmp = index & 0xc0;
            if (tmp == 0x40) {
                cpuInstruction[index] = LD_R8_R8;
                continue;
            }
        } else if (blockId == BLOCK_2) {
            cpuInstruction[index] = Opcode_A_R8;
            continue;
        } else if (blockId == BLOCK_3) {
            tmp = index & 0xe7;
            if (tmp == 0xc0) {
                cpuInstruction[index] = RET_COND;
                continue;
            }
            if (tmp == 0xc2) {
                cpuInstruction[index] = JP_COND_IMM16;
                continue;
            }
        }
    }

    InitInstructionFixed();
}

int InitCPU(struct CPU *cpu) {
    InitInstruction();
    cpu->pc = 0x00;
    return 0;
}

int CPUTick(struct CPU *cpu, struct MMU *mmu) {
    if (MMURead8(mmu, cpu->pc, &opcode)) {
        return 1;
    }
    cpu->pc++;
    return cpuInstruction[opcode](cpu, mmu);
}
