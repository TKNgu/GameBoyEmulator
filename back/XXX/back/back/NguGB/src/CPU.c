#include "CPU.h"
#include <stdio.h>

#include "Bus.h"
#include "Instruction.h"

void CPUInit(CPU *cpu) {
  cpu->registers.PC = 0x100;
  cpu->registers.A = 0x01;
}

int CPUStep(CPU *cpu) {
  u8 opcode;
  if (BusRead(cpu->bus, cpu->registers.PC++, &opcode)) {
    return CPU_ERROR_READ_OPCODE;
  }
  if (opcode == 0b00010000) {
    return CPU_STOP;
  }
  if (opcode == 0b01110110) {
    return CPU_HALT;
  }

  for (size_t index = 0; index < instruction_size; index++) {
    Instruction instruction = instructions[index];
    if ((opcode & instruction.mask) == instruction.value) {
      if (instruction.Exec(cpu, opcode)) {
        return CPU_ERROR_INSTRUCTION_EXEC;
      }
      return 0;
    }
  }
  printf("Opcode 0x%x\n", opcode);
  return CPU_ERROR_OPCODE_NOT_IMPL;
}

void CPUShow(CPU cpu) {
  printf("Cpu:\n");
  printf("\tAF: 0x%x\n", cpu.registers.AF);
  printf("\tBC: 0x%x\n", cpu.registers.BC);
  printf("\tDE: 0x%x\n", cpu.registers.DE);
  printf("\tHL: 0x%x\n", cpu.registers.HL);
  printf("\tSP: 0x%x\n", cpu.registers.SP);
  printf("\tPC: 0x%x\n", cpu.registers.PC);
}

u16 CPUReadR16(CPU *cpu, u8 opcode) {
  switch (opcode & 0b00110000) {
  case 0b00000000:
    return cpu->registers.BC;
  case 0b00010000:
    return cpu->registers.DE;
  case 0b00100000:
    return cpu->registers.HL;
  default: // 0b00110000:
    return cpu->registers.SP;
  }
}

void CPUWriteR16(CPU *cpu, u8 opcode, u16 value) {
  switch (opcode & 0b00110000) {
  case 0b00000000:
    cpu->registers.BC = value;
    break;
  case 0b00010000:
    cpu->registers.DE = value;
    break;
  case 0b00100000:
    cpu->registers.HL = value;
    break;
  case 0b00110000:
    cpu->registers.SP = value;
    break;
  }
}

int CPUReadR8(CPU *cpu, u8 bit, u8 *r8) {
  switch (bit) {
  case 0b000:
    *r8 = cpu->registers.B;
    break;
  case 0b001:
    *r8 = cpu->registers.C;
    break;
  case 0b010:
    *r8 = cpu->registers.D;
    break;
  case 0b011:
    *r8 = cpu->registers.E;
    break;
  case 0b100:
    *r8 = cpu->registers.H;
    break;
  case 0b101:
    *r8 = cpu->registers.L;
    break;
  case 0b110:
    return BusRead(cpu->bus, cpu->registers.HL, r8);
  case 0b111:
    *r8 = cpu->registers.A;
    break;
  }
  return 0;
}

int CPUWriteR8(CPU *cpu, u8 bit, u8 r8) {
  switch (bit) {
  case 0b000:
    cpu->registers.B = r8;
    break;
  case 0b001:
    cpu->registers.C = r8;
    break;
  case 0b010:
    cpu->registers.D = r8;
    break;
  case 0b011:
    cpu->registers.E = r8;
    break;
  case 0b100:
    cpu->registers.H = r8;
    break;
  case 0b101:
    cpu->registers.L = r8;
    break;
  case 0b110:
    return BusWrite(cpu->bus, cpu->registers.HL, r8);
  case 0b111:
    cpu->registers.A = r8;
    break;
  }
  return 0;
}

int CPUReadIMM(CPU *cpu, u8 *value) {
  return BusRead(cpu->bus, cpu->registers.PC++, value);
}

int CPUReadIMM16(CPU *cpu, u16 *value) {
  u8 low;
  if (CPUReadIMM(cpu, &low)) {
    return 1;
  }
  u8 hight;
  if (CPUReadIMM(cpu, &hight)) {
    return 1;
  }
  *value = (hight << 8) + low;
  return 0;
}

int CPUCond(CPU *cpu, u8 opcode) {
  switch (opcode & 0b00011000) {
  case 0b00000000:
    return !ZFLAG;
  case 0b00001000:
    return ZFLAG;
  case 0b00010000:
    return !CFLAG;
  default:
    return CFLAG;
  }
}
