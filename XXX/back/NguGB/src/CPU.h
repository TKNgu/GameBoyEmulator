#ifndef CPU_h
#define CPU_h

#include "Bus.h"
#include "Common.h"

#define CPU_ERROR_READ_OPCODE 01
#define CPU_ERROR_OPCODE_NOT_IMPL 02
#define CPU_ERROR_INSTRUCTION_EXEC 03
#define CPU_STOP 04
#define CPU_HALT 04

#define ZFLAG (cpu->registers.F & 0b10000000)
#define NFLAG (cpu->registers.F & 0b01000000)
#define HFLAG (cpu->registers.F & 0b00100000)
#define CFLAG (cpu->registers.F & 0b00010000)

#define ZFLAGOF (cpu->registers.F &= 0b01110000)
#define ZFLAGON (cpu->registers.F |= 0b10000000)
#define NFLAGOF (cpu->registers.F &= 0b10110000)
#define NFLAGON (cpu->registers.F |= 0b01000000)
#define HFLAGOF (cpu->registers.F &= 0b11010000)
#define HFLAGON (cpu->registers.F |= 0b00100000)
#define CFLAGOF (cpu->registers.F &= 0b11100000)
#define CFLAGON (cpu->registers.F |= 0b00010000)

typedef struct {
  struct {
    union {
      u16 AF;
      struct {
        u8 F;
        u8 A;
      };
    };
    union {
      u16 BC;
      struct {
        u8 C;
        u8 B;
      };
    };
    union {
      u16 DE;
      struct {
        u8 E;
        u8 D;
      };
    };
    union {
      u16 HL;
      struct {
        u8 L;
        u8 H;
      };
    };
    union {
      u16 SP;
      struct {
        u8 P;
        u8 S;
      };
    };
    u16 PC;
  } registers;
  Bus bus;
} CPU;

void CPUInit(CPU *cpu);
int CPUStep(CPU *cpu);
void CPUShow(CPU cpu);

u16 CPUReadR16(CPU *cpu, u8 opcode);
void CPUWriteR16(CPU *cpu, u8 opcode, u16 value);
int CPUReadR8(CPU *cpu, u8 bit, u8 *r8);
int CPUWriteR8(CPU *cpu, u8 bit, u8 r8);
int CPUReadIMM(CPU *cpu, u8 *value);
int CPUReadIMM16(CPU *cpu, u16 *value);
int CPUCond(CPU *cpu, u8 opcode);

#endif
