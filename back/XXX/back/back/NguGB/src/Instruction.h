#ifndef Instruction_h
#define Instruction_h

#include "CPU.h"

typedef struct {
  u8 mask;
  u8 value;
  int (*Exec)(CPU *cpu, u8 opcode);
} Instruction;

extern int instruction_size;
extern Instruction instructions[];

void InstructionInit();

#endif
