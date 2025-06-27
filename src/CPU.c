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
int (*cpuInstructions[NUM_INSTRUCTION])(struct CPU *, struct MMU *);
size_t tickCount;

int CPUReadI8(struct MMU *mmu, uint16_t addr, uint8_t *value) {
    tickCount += 4;
    return MMURead8(mmu, addr, value);
}

int CPUWriteI8(struct MMU *mmu, uint16_t addr, uint8_t value) {
    tickCount += 4;
    return MMUWrite8(mmu, addr, value);
}

int CPUReadNextI8(struct CPU *cpu, struct MMU *mmu, uint8_t *i8) {
    if (CPUReadI8(mmu, cpu->pc, i8)) {
        return 1;
    }
    cpu->pc = (cpu->pc + 1) & 0xffff;
    return 0;
}

int CPUReadNextI16(struct CPU *cpu, struct MMU *mmu, uint16_t *i16) {
    uint8_t low;
    if (CPUReadI8(mmu, cpu->pc, &low)) {
        return 1;
    }
    cpu->pc = (cpu->pc + 1) & 0xffff;
    uint8_t hight;
    if (CPUReadI8(mmu, cpu->pc, &hight)) {
        return 1;
    }
    cpu->pc = (cpu->pc + 1) & 0xffff;
    *i16 = hight;
    *i16 = ((*i16) << 8) | low;
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
    tickCount += 4;
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
    return 0;
}

int LD_R16_IMM16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16 = DecodeR16(cpu);
    return CPUReadNextI16(cpu, mmu, r16);
}

int LD_R16MEM_A(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16mem = DecodeR16Mem(cpu);
    return CPUWriteI8(mmu, *r16mem, cpu->a);
}

int LD_A_R16MEM(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16mem = DecodeR16Mem(cpu);
    return CPUReadI8(mmu, *r16mem, &cpu->a);
}

int LD_IMM16_SP(struct CPU *cpu, struct MMU *mmu) {
    uint16_t imm16;
    if (CPUReadNextI16(cpu, mmu, &imm16)) {
        return 1;
    }
    if (CPUWriteI8(mmu, imm16, cpu->s)) {
        return 1;
    }
    if (CPUWriteI8(mmu, imm16++, cpu->p)) {
        return 1;
    }
    return 0;
}

int INC_R16(struct CPU *cpu, struct MMU *mmu) {
    tickCount += 4;
    uint16_t *r16 = DecodeR16(cpu);
    uint16_t value = *r16;
    value = (value + 1) & 0xffff;
    *r16 = value;
    return 0;
}

int DEC_R16(struct CPU *cpu, struct MMU *mmu) {
    tickCount += 4;
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
        CPUReadI8(mmu, cpu->hl, &value);
        value = RegisterINC(cpu, value);
        CPUWriteI8(mmu, cpu->hl, value);
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
        if (CPUReadI8(mmu, cpu->hl, &value)) {
            return 1;
        }
        value = RegisterDEC(cpu, value);
        if (CPUWriteI8(mmu, cpu->hl, value)) {
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
        if (CPUWriteI8(mmu, cpu->hl, value)) {
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
    tickCount += 4;
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
        if (CPUReadI8(mmu, cpu->hl, &value)) {
            return 1;
        }
    } else {
        uint8_t *src = DecodeR8Source(cpu);
        value = *src;
    }

    if (((opcode >> 3) & 0x07) == 0x06) {
        if (CPUWriteI8(mmu, cpu->hl, value)) {
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
        if (CPUReadI8(mmu, cpu->hl, &value)) {
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
    if (CPUReadI8(mmu, cpu->sp, value)) {
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
    tickCount += 4;
    return POPW(cpu, mmu, &cpu->pc);
}

int RETi(struct CPU *cpu, struct MMU *mmu) {
    return RET(cpu, mmu);
}

int RET_COND(struct CPU *cpu, struct MMU *mmu) {
    tickCount += 4;
    return DecodeCond(cpu) ? RET(cpu, mmu) : 0;
}

int JP_IMM16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t tmp;
    if (CPUReadNextI16(cpu, mmu, &tmp)) {
        return 1;
    }
    tickCount += 4;
    cpu->pc = tmp;
    return 0;
}

int JP_COND_IMM16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t tmp;
    return DecodeCond(cpu) ? JP_IMM16(cpu, mmu)
                           : CPUReadNextI16(cpu, mmu, &tmp);
}

int JP_HL(struct CPU *cpu, struct MMU *mmu) {
    cpu->pc = cpu->hl;
    return 0;
}

int PUSHB(struct CPU *cpu, struct MMU *mmu, uint8_t value) {
    tickCount += 4;
    cpu->sp = (cpu->sp - 1) & 0xffff;
    return CPUWriteI8(mmu, cpu->sp, value);
}

int PUSHW(struct CPU *cpu, struct MMU *mmu, uint16_t value) {
    if (PUSHB(cpu, mmu, value >> 8)) {
        return 1;
    }
    if (PUSHB(cpu, mmu, value & 0xff)) {
        return 1;
    }
    return 0;
}

int RST(struct CPU *cpu, struct MMU *mmu, uint16_t target) {
    if (PUSHW(cpu, mmu, cpu->pc)) {
        return 1;
    }
    cpu->pc = target;
    return 0;
}

int CALL_IMM16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t i16;
    if (CPUReadNextI16(cpu, mmu, &i16)) {
        return 1;
    }
    tickCount += 4;
    cpu->sp--;
    return RST(cpu, mmu, i16);
}

int CALL_COND_IMM16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t tmp;
    return DecodeCond(cpu) ? CALL_IMM16(cpu, mmu)
                           : CPUReadNextI16(cpu, mmu, &tmp);
}

int RST_TGT3(struct CPU *cpu, struct MMU *mmu) {
    tickCount += 4;
    static uint16_t address[] = {0x00, 0x08, 0x10, 0x18,
                                 0x20, 0x28, 0x30, 0x38};
    uint8_t index = (opcode & 0x38) >> 3;
    return RST(cpu, mmu, address[index]);
}

uint16_t *DecodeR16skt(struct CPU *cpu) {
    switch (opcode & 0x30) {
    case 0x00:
        return &cpu->bc;
        break;
    case 0x10:
        return &cpu->de;
        break;
    case 0x20:
        return &cpu->hl;
        break;
    default:
        return &cpu->af;
        break;
    }
}

int POP_R16stk(struct CPU *cpu, struct MMU *mmu) {
    uint16_t *r16 = DecodeR16skt(cpu);
    if (r16 == &cpu->af) {
        uint8_t value;
        if (POPB(cpu, mmu, &value)) {
            return 1;
        }
        cpu->f = value & 0xf0;
        /* Restore flags from memory (low 4 bits are ignored) */
        cpu->f_z = cpu->fz;
        cpu->f_n = cpu->fn;
        cpu->f_h = cpu->fh;
        cpu->f_c = cpu->fc;

        if (POPB(cpu, mmu, &cpu->a)) {
            return 1;
        }
        return 0;
    } else {
        return POPW(cpu, mmu, r16);
    }
}

int PUSH_R16stk(struct CPU *cpu, struct MMU *mmu) {
    tickCount += 4;
    uint16_t *r16 = DecodeR16skt(cpu);
    if (r16 == &cpu->af) {
        if (PUSHB(cpu, mmu, cpu->a)) {
            return 1;
        }

        cpu->fc = cpu->f_c;
        cpu->fh = cpu->f_h;
        cpu->fn = cpu->f_n;
        cpu->fz = cpu->f_z;
        if (PUSHB(cpu, mmu, cpu->f)) {
            return 1;
        }
        return 0;
    } else {
        return PUSHW(cpu, mmu, *r16);
    }
}

uint8_t cbOpcode;
int (*cpuCBInstructions[NUM_INSTRUCTION])(struct CPU *, struct MMU *);

int CB(struct CPU *cpu, struct MMU *mmu) {
    if (CPUReadNextI8(cpu, mmu, &cbOpcode)) {
        return 1;
    }
    tickCount += 4;
    return cpuCBInstructions[cbOpcode](cpu, mmu);
}

int LDH_imm8_A(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    uint16_t addr = 0xff00 | value;
    return CPUWriteI8(mmu, addr, cpu->a);
}

int LDH_MemC_A(struct CPU *cpu, struct MMU *mmu) {
    uint16_t addr = 0xff00 | cpu->c;
    return CPUWriteI8(mmu, addr, cpu->a);
}

int ADD_SP_si8(struct CPU *cpu, struct MMU *mmu, uint16_t *value) {
    uint8_t tmp;
    if (CPUReadNextI8(cpu, mmu, &tmp)) {
        return 1;
    }
    tickCount += 4;
    int8_t i8 = (int8_t)tmp;
    int32_t r = (uint32_t)cpu->sp;
    *value = (uint16_t)(r += i8);

    cpu->f_z = false;
    cpu->f_n = false;
    cpu->f_h = (cpu->sp ^ i8 ^ r) & 0x10;
    cpu->f_c = (cpu->sp ^ i8 ^ r) & 0x100;
    return 0;
}

int ADD_SP_Imm8(struct CPU *cpu, struct MMU *mmu) {
    tickCount += 4;
    return ADD_SP_si8(cpu, mmu, &cpu->sp);
}

int LD_Imm16_A(struct CPU *cpu, struct MMU *mmu) {
    uint16_t addr;
    if (CPUReadNextI16(cpu, mmu, &addr)) {
        return 1;
    }
    CPUWriteI8(mmu, addr, cpu->a);
    return 0;
}

int LDH_A_imm8(struct CPU *cpu, struct MMU *mmu) {
    uint8_t value;
    if (CPUReadNextI8(cpu, mmu, &value)) {
        return 1;
    }
    uint16_t addr = 0xff00 | value;
    return CPUReadI8(mmu, addr, &cpu->a);
}

int LDH_A_MemC(struct CPU *cpu, struct MMU *mmu) {
    uint16_t addr = 0xff00 | cpu->c;
    return CPUReadI8(mmu, addr, &cpu->a);
}

int LD_HL_SP_ADD_Imm8(struct CPU *cpu, struct MMU *mmu) {
    return ADD_SP_si8(cpu, mmu, &cpu->hl);
}

int LD_A_imm16(struct CPU *cpu, struct MMU *mmu) {
    uint16_t addr;
    if (CPUReadNextI16(cpu, mmu, &addr)) {
        return 1;
    }
    return CPUReadI8(mmu, addr, &cpu->a);
}

int LD_SP_HL(struct CPU *cpu, struct MMU *mmu) {
    cpu->sp = cpu->hl;
    return 0;
}

int DI(struct CPU *cpu, struct MMU *mmu) {
    return 0;
}

int EI(struct CPU *cpu, struct MMU *mmu) {
    return 0;
}

void InitInstructionFixed() {
    cpuInstructions[0x00] = NOP;
    cpuInstructions[0x08] = LD_IMM16_SP;
    cpuInstructions[0x07] = RLCA;
    cpuInstructions[0x0f] = RRCA;
    cpuInstructions[0x17] = RLA;
    cpuInstructions[0x1f] = RRA;
    cpuInstructions[0x27] = DAA;
    cpuInstructions[0x2f] = CPL;
    cpuInstructions[0x37] = SCF;
    cpuInstructions[0x3f] = CCF;
    cpuInstructions[0x18] = JR_IMM8;
    cpuInstructions[0x10] = STOP;

    cpuInstructions[0x76] = HALT;

    cpuInstructions[0xc6] = ADD_A_IMM8;
    cpuInstructions[0xce] = ADC_A_IMM8;
    cpuInstructions[0xd6] = SUB_A_IMM8;
    cpuInstructions[0xde] = SBC_A_IMM8;
    cpuInstructions[0xe6] = AND_A_IMM8;
    cpuInstructions[0xee] = XOR_A_IMM8;
    cpuInstructions[0xf6] = OR_A_IMM8;
    cpuInstructions[0xfe] = CP_A_IMM8;

    cpuInstructions[0xc9] = RET;
    cpuInstructions[0xd9] = RETi;

    cpuInstructions[0xc3] = JP_IMM16;
    cpuInstructions[0xe9] = JP_HL;

    cpuInstructions[0xcd] = CALL_IMM16;

    cpuInstructions[0xcb] = CB;

    cpuInstructions[0xe0] = LDH_imm8_A;
    cpuInstructions[0xe2] = LDH_MemC_A;
    cpuInstructions[0xe8] = ADD_SP_Imm8;
    cpuInstructions[0xea] = LD_Imm16_A;

    cpuInstructions[0xf0] = LDH_A_imm8;
    cpuInstructions[0xf2] = LDH_A_MemC;
    cpuInstructions[0xf3] = DI;
    cpuInstructions[0xf8] = LD_HL_SP_ADD_Imm8;
    cpuInstructions[0xf9] = LD_SP_HL;
    cpuInstructions[0xfa] = LD_A_imm16;
    cpuInstructions[0xfb] = EI;
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
                cpuInstructions[index] = LD_R16_IMM16;
                continue;
            }
            if (tmp == 0x20) {
                cpuInstructions[index] = LD_R16MEM_A;
                continue;
            }
            if (tmp == 0xa0) {
                cpuInstructions[index] = LD_A_R16MEM;
                continue;
            }
            if (tmp == 0x03) {
                cpuInstructions[index] = INC_R16;
                continue;
            }
            if (tmp == 0x0b) {
                cpuInstructions[index] = DEC_R16;
                continue;
            }
            if (tmp == 0x09) {
                cpuInstructions[index] = ADD_HL_R16;
                continue;
            }
            tmp = index & 0x07;
            if (tmp == 0x04) {
                cpuInstructions[index] = INC_R8;
                continue;
            }
            if (tmp == 0x05) {
                cpuInstructions[index] = DEC_R8;
                continue;
            }
            if (tmp == 0x06) {
                cpuInstructions[index] = LD_R8_IMM8;
                continue;
            }
            if (tmp == 0x20) {
                cpuInstructions[index] = JR_COND_IMM8;
                continue;
            }
        } else if (blockId == BLOCK_1) {
            tmp = index & 0xc0;
            if (tmp == 0x40) {
                cpuInstructions[index] = LD_R8_R8;
                continue;
            }
        } else if (blockId == BLOCK_2) {
            cpuInstructions[index] = Opcode_A_R8;
            continue;
        } else if (blockId == BLOCK_3) {
            tmp = index & 0xe7;
            if (tmp == 0xc0) {
                cpuInstructions[index] = RET_COND;
                continue;
            }
            if (tmp == 0xc2) {
                cpuInstructions[index] = JP_COND_IMM16;
                continue;
            }
            if (tmp == 0xc4) {
                cpuInstructions[index] = CALL_COND_IMM16;
                continue;
            }
            if (tmp == 0xc7) {
                cpuInstructions[index] = RST_TGT3;
                continue;
            }
            tmp = index & 0xcf;
            if (tmp == 0xc1) {
                cpuInstructions[index] = POP_R16stk;
                continue;
            }
            if (tmp == 0xc5) {
                cpuInstructions[index] = PUSH_R16stk;
                continue;
            }
        }
    }

    InitInstructionFixed();
}

uint8_t *DecodeCBOperand(struct CPU *cpu) {
    switch (cbOpcode & 0x07) {
    case 0x00:
        return &cpu->b;
        break;
    case 0x01:
        return &cpu->c;
        break;
    case 0x02:
        return &cpu->d;
        break;
    case 0x03:
        return &cpu->e;
        break;
    case 0x04:
        return &cpu->h;
        break;
    case 0x05:
        return &cpu->l;
        break;
    case 0x06: // HL
        return NULL;
        break;
    default: // case 0x07:
        return &cpu->a;
    }
}

void CBRLCSetFlags(struct CPU *cpu, uint8_t *v) {
    uint8_t c = *v >> 7;
    *v = (*v << 1) | c;
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;
}

void CBRRCSetFlags(struct CPU *cpu, uint8_t *v) {
    uint8_t c = *v & 1;
    *v = (*v >> 1) | (c << 7);
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;
}

void CBRLSetFlags(struct CPU *cpu, uint8_t *v) {
    bool new_c = *v >> 7;
    *v = (*v << 1) | (uint8_t)cpu->f_c;
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = new_c;
}

void CBRRSetFlags(struct CPU *cpu, uint8_t *v) {
    bool new_c = *v & 1;
    uint8_t old_c = cpu->f_c;
    *v = (*v >> 1) | (old_c << 7);
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = new_c;
}

void CBSLASetFlags(struct CPU *cpu, uint8_t *v) {
    bool c = *v >> 7;
    *v = *v << 1;
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;
}

void CBSRASetFlags(struct CPU *cpu, uint8_t *v) {
    bool c = *v & 1;
    *v = (*v >> 1) | (*v & 0x80);
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;
}

void CBSWAPSetFlags(struct CPU *cpu, uint8_t *v) {
    *v = ((*v << 4) | (*v >> 4)) & 0xff;
    cpu->f_z = (*v == 0);
    cpu->f_h = false;
    cpu->f_c = false;
}

void CBSRLSetFlags(struct CPU *cpu, uint8_t *v) {
    bool c = *v & 1;
    *v = *v >> 1;
    cpu->f_z = (*v == 0);
    cpu->f_n = false;
    cpu->f_h = false;
    cpu->f_c = c;
}

int CPU_CB_R8(struct CPU *cpu, struct MMU *mmu) {
    void (*cbInstruction)(struct CPU *, uint8_t *) = NULL;
    switch (cbOpcode & 0xf8) {
    case 0x00:
        cbInstruction = CBRLCSetFlags;
        break;
    case 0x08:
        cbInstruction = CBRRCSetFlags;
        break;
    case 0x10:
        cbInstruction = CBRLSetFlags;
        break;
    case 0x18:
        cbInstruction = CBRRSetFlags;
        break;
    case 0x20:
        cbInstruction = CBSLASetFlags;
        break;
    case 0x28:
        cbInstruction = CBSRASetFlags;
        break;
    case 0x30:
        cbInstruction = CBSWAPSetFlags;
        break;
    default: // 0x38
        cbInstruction = CBSRLSetFlags;
        break;
    }
    uint8_t *r8 = DecodeCBOperand(cpu);
    if (r8) {
        cbInstruction(cpu, r8);
        return 0;
    } else {
        uint8_t value;
        if (CPUReadI8(mmu, cpu->hl, &value)) {
            return 1;
        }
        cbInstruction(cpu, &value);
        return CPUWriteI8(mmu, cpu->hl, value);
    }
}

void BitSetFlags(struct CPU *cpu, uint8_t *v, uint8_t bit) {
    bool set = *v & (1U << bit);
    cpu->f_z = !set;
    cpu->f_n = false;
    cpu->f_h = true;
}

void ResSetFlags(struct CPU *, uint8_t *v, uint8_t bit) {
    *v = *v & ~(1U << bit);
}

void SetSetFlags(struct CPU *, uint8_t *v, uint8_t bit) {
    *v = *v | (1U << bit);
}

int CPU_CB_BIT(struct CPU *cpu, struct MMU *mmu) {
    void (*cbInstructionBit)(struct CPU *, uint8_t *, uint8_t) = NULL;
    switch (cbOpcode & 0xc0) {
    case 0x40:
        cbInstructionBit = BitSetFlags;
        break;
    case 0xb8:
        cbInstructionBit = ResSetFlags;
        break;
    default: // 0xc0:
        cbInstructionBit = SetSetFlags;
        break;
    }
    uint8_t bit = (cbOpcode >> 3) & 0x07;
    uint8_t *r8 = DecodeCBOperand(cpu);
    if (r8) {
        cbInstructionBit(cpu, r8, bit);
        return 0;
    } else {
        uint8_t value;
        if (CPUReadI8(mmu, cpu->hl, &value)) {
            return 1;
        }
        cbInstructionBit(cpu, &value, bit);
        return CPUWriteI8(mmu, cpu->hl, value);
    }
}

void InitCBInstruction() {
    for (size_t indexOpcodeCB = 0; indexOpcodeCB < 0x100; indexOpcodeCB++) {
        if ((indexOpcodeCB & 0xc0) == 0x00) {
            cpuCBInstructions[indexOpcodeCB] = CPU_CB_R8;
        } else {
            cpuCBInstructions[indexOpcodeCB] = CPU_CB_BIT;
        }
    }
}

int InitCPU(struct CPU *cpu) {
    InitInstruction();
    InitCBInstruction();
    cpu->pc = 0x00;
    return 0;
}

int CPUTick(struct CPU *cpu, struct MMU *mmu, size_t *numTick) {
    tickCount = 0;
    if (CPUReadI8(mmu, cpu->pc, &opcode)) {
        return 1;
    }
    cpu->pc++;
    int ret = cpuInstructions[opcode](cpu, mmu);
    *numTick = tickCount;
    return ret;
}
