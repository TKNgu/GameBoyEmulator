#include "Instruction.h"
#include "Bus.h"
#include "CPU.h"

int instruction_size = 0;
Instruction instructions[0b100];

int NOP(CPU *cpu, u8 opcode) { return 0; }

int LDR16IMM16(CPU *cpu, u8 opcode) {
  u16 value;
  if (CPUReadIMM16(cpu, &value)) {
    return 1;
  }
  CPUWriteR16(cpu, opcode, value);
  return 0;
}

int LDR16MEMA(CPU *cpu, u8 opcode) {
  u16 r16 = CPUReadR16(cpu, opcode);
  return BusWrite(cpu->bus, r16, cpu->registers.A);
}

int LDAR16MEM(CPU *cpu, u8 opcode) {
  u16 r16 = CPUReadR16(cpu, opcode);
  return BusWrite(cpu->bus, r16, cpu->registers.A);
}

int LDIMM16SP(CPU *cpu, u8 opcode) {
  u16 imm16;
  if (CPUReadIMM16(cpu, &imm16)) {
    return 1;
  }
  if (BusWrite(cpu->bus, imm16++, cpu->registers.P)) {
    return 1;
  }
  if (BusWrite(cpu->bus, imm16, cpu->registers.S)) {
    return 1;
  }
  return 0;
}

int INCR16(CPU *cpu, u8 opcode) {
  u16 r16 = CPUReadR16(cpu, opcode);
  CPUWriteR16(cpu, opcode, r16 + 1);
  return 0;
}

int DECR16(CPU *cpu, u8 opcode) {
  u16 r16 = CPUReadR16(cpu, opcode);
  CPUWriteR16(cpu, opcode, r16 - 1);
  return 0;
}

int ADDHLR16(CPU *cpu, u8 opcode) {
  u32 a = cpu->registers.HL;
  u32 b = CPUReadR16(cpu, opcode);

  NFLAGOF;
  (a & 0xff) + (b & 0xfff) >= 0x1000 ? HFLAGON : HFLAGOF;
  a + b >= 0x10000 ? CFLAGON : CFLAGOF;
  cpu->registers.HL += b;
  return 0;
}

int INCR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (CPUReadR8(cpu, (opcode & 0b00111000) >> 3, &r8)) {
    return 1;
  }
  u8 tmp = r8 + 1;
  tmp ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  (r8 & 0xf) + 1 >= 0x10 ? HFLAGON : HFLAGOF;
  return CPUWriteR8(cpu, (opcode & 0b00111000) >> 3, tmp);
}

int DECR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (CPUReadR8(cpu, (opcode & 0b00111000) >> 3, &r8)) {
    return 1;
  }
  u8 tmp = r8 - 1;
  tmp ? ZFLAGOF : ZFLAGON;
  r8 ? NFLAGOF : NFLAGON;
  (r8 & 0xf) ? HFLAGOF : HFLAGON;
  return CPUWriteR8(cpu, (opcode & 0b00111000) >> 3, tmp);
}

int LDR8IMM8(CPU *cpu, u8 opcode) {
  u8 imm8;
  if (CPUReadIMM(cpu, &imm8)) {
    return 1;
  }
  return CPUWriteR8(cpu, (opcode & 0b00111000) >> 3, imm8);
}

int RLCA(CPU *cpu, u8 opcode) {
  u8 tmp = cpu->registers.A;
  u8 c = (tmp >> 7) & 1;
  tmp = (tmp << 1) | c;
  tmp ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  HFLAGOF;
  c ? CFLAGON : CFLAGOF;
  cpu->registers.A = tmp;
  return 0;
}

int RRCA(CPU *cpu, u8 opcode) {
  u8 tmp = cpu->registers.A;
  u8 c = tmp & 1;
  tmp >>= 1;
  tmp |= (c << 7);
  tmp ? CFLAGOF : CFLAGON;
  NFLAGOF;
  HFLAGOF;
  c ? CFLAGON : CFLAGOF;
  cpu->registers.A = tmp;
  return 0;
}

int RLA(CPU *cpu, u8 opcode) {
  u8 tmp = cpu->registers.A;
  u8 c = (tmp >> 7) & 1;
  tmp <<= 1;
  if (CFLAG) {
    tmp += 1;
  }
  tmp ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  HFLAGOF;
  c ? CFLAGON : CFLAGOF;
  cpu->registers.A = tmp;
  return 0;
}

int RRA(CPU *cpu, u8 opcode) {
  u8 tmp = cpu->registers.A;
  u8 c = tmp & 1;
  tmp >>= 1;
  if (CFLAG) {
    tmp += 0b10000000;
  }
  tmp ? CFLAGOF : CFLAGON;
  NFLAGOF;
  HFLAGOF;
  c ? CFLAGON : CFLAGOF;
  cpu->registers.A = tmp;
  return 0;
}

int DAA(CPU *cpu, u8 opcode) {
  if (NFLAG) {
    if (CFLAG) {
      cpu->registers.A -= 0x60;
    }
    if (HFLAG) {
      cpu->registers.A -= 0x06;
    }
  } else {
    if (CFLAG || cpu->registers.A > 0x99) {
      cpu->registers.A += 0x60;
      CFLAGON;
    }
    if (HFLAG || (cpu->registers.A & 0x0f) > 0x09) {
      cpu->registers.A += 0x06;
    }
  }
  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  HFLAGOF;
  return 0;
}

int CPL(CPU *cpu, u8 opcode) {
  cpu->registers.A = ~cpu->registers.A;
  NFLAGON;
  HFLAGON;
  return 0;
}

int SCF(CPU *cpu, u8 opcode) {
  cpu->registers.F &= 0b10010000;
  cpu->registers.F |= 0b00010000;
  return 0;
}

int CCF(CPU *cpu, u8 opcode) {
  cpu->registers.F &= 0b10010000;
  CFLAG ? (cpu->registers.F &= 0b11100000) : (cpu->registers.F |= 0b00010000);
  return 0;
}

int JRIMM8(CPU *cpu, u8 opcode) {
  u8 imm8;
  if (CPUReadIMM(cpu, &imm8)) {
    return 1;
  }
  cpu->registers.PC += imm8;
  return 0;
}

int JRCondIMM8(CPU *cpu, u8 opcode) {
  u8 imm8;
  if (CPUReadIMM(cpu, &imm8)) {
    return 1;
  }
  if (CPUCond(cpu, opcode)) {
    cpu->registers.PC += imm8;
  }
  return 0;
}

int Stop(CPU *cpu, u8 opcode) { return 1; }

int LDR8R8(CPU *cpu, u8 opcode) {
  u8 tmp;
  if (CPUReadR8(cpu, opcode & 0b00000111, &tmp)) {
    return 1;
  }
  if (CPUWriteR8(cpu, (opcode & 0b00111000) >> 3, tmp)) {
    return 1;
  }
  return 0;
}

int HALT(CPU *cpu, u8 opcode) { return 1; }

int ADDAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }

  u16 r16 = r8;
  u16 a16 = cpu->registers.A;
  u16 sum = r16 + a16;
  cpu->registers.A = sum & 0xff;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  (r16 & 0x0f) + (a16 & 0x0f) >= 0x10 ? HFLAGON : HFLAGOF;
  r16 + a16 >= 0x100 ? CFLAGON : CFLAGOF;
  return 0;
}

int ADCAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }

  u16 c16 = CFLAG ? 1 : 0;
  u16 r16 = r8;
  u16 a16 = cpu->registers.A;
  u16 sum = r16 + a16 + c16;
  cpu->registers.A = sum & 0xff;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  (r16 & 0x0f) + (a16 & 0x0f) + c16 >= 0x10 ? HFLAGON : HFLAGOF;
  r16 + a16 + c16 >= 0x100 ? CFLAGON : CFLAGOF;
  return 0;
}

int SUBAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }

  u16 r16 = r8;
  u16 a16 = cpu->registers.A;
  u16 sub = a16 - r16;
  cpu->registers.A = sub & 0xff;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGON;
  (a16 & 0x0f) < (r16 & 0x0f) ? HFLAGON : HFLAGOF;
  a16 < r16 ? CFLAGON : CFLAGOF;
  return 0;
}

int SBCAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }

  u16 c16 = CFLAG ? 1 : 0;
  u16 r16 = r8;
  u16 a16 = cpu->registers.A;
  u16 sub = a16 - r16 - c16;
  cpu->registers.A = sub & 0xff;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  (a16 & 0x0f) < ((r16 & 0x0f) + c16) ? HFLAGON : HFLAGOF;
  a16 < (r16 + c16) ? CFLAGON : CFLAGOF;
  return 0;
}

int ANDAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }
  cpu->registers.A &= r8;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  HFLAGON;
  CFLAGOF;
  return 0;
}

int XORAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }
  cpu->registers.A ^= r8;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  HFLAGOF;
  CFLAGOF;
  return 0;
}

int ORAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }
  cpu->registers.A |= r8;

  cpu->registers.A ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  HFLAGOF;
  CFLAGOF;
  return 0;
}

int CPAR8(CPU *cpu, u8 opcode) {
  u8 r8;
  if (opcode & 0b01000000) {
    if (CPUReadIMM(cpu, &r8)) {
      return 1;
    }
  } else {
    if (CPUReadR8(cpu, opcode & 0b111, &r8)) {
      return 1;
    }
  }

  u16 r16 = r8;
  u16 a16 = cpu->registers.A;
  u16 sub = a16 - r16;

  (sub & 0xff) ? ZFLAGOF : ZFLAGON;
  NFLAGON;
  (a16 & 0x0f) < (r16 & 0x0f) ? HFLAGON : HFLAGOF;
  a16 < r16 ? CFLAGON : CFLAGOF;
  return 0;
}

int RET(CPU *cpu, u8 opcode) {}

void InstructionInit() {
  // Block 0
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00000000,
      .Exec = NOP,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11001111,
      .value = 0b00000001,
      .Exec = LDR16IMM16,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11001111,
      .value = 0b00000010,
      .Exec = LDR16MEMA,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11001111,
      .value = 0b00001010,
      .Exec = LDAR16MEM,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00001000,
      .Exec = LDIMM16SP,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11001111,
      .value = 0b00000001,
      .Exec = INCR16,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11001111,
      .value = 0b00001011,
      .Exec = INCR16,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11001111,
      .value = 0b00001001,
      .Exec = ADDHLR16,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11000111,
      .value = 0b00000100,
      .Exec = INCR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11000111,
      .value = 0b00000101,
      .Exec = DECR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11000111,
      .value = 0b00000110,
      .Exec = LDR8IMM8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00000111,
      .Exec = RLCA,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00001111,
      .Exec = RRCA,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00010111,
      .Exec = RLA,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00011111,
      .Exec = RRA,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00100111,
      .Exec = DAA,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00101111,
      .Exec = CPL,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00110111,
      .Exec = SCF,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00111111,
      .Exec = CCF,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00011000,
      .Exec = JRIMM8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11100111,
      .value = 0b00100000,
      .Exec = JRCondIMM8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b00010000,
      .Exec = Stop,
  };

  // Block 1
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b01110110,
      .Exec = HALT,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11000000,
      .value = 0b01000000,
      .Exec = LDR8R8,
  };

  // Block 2
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10000000,
      .Exec = ADDAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10001000,
      .Exec = ADCAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10010000,
      .Exec = SUBAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10011000,
      .Exec = SBCAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10100000,
      .Exec = ANDAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10101000,
      .Exec = XORAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10110000,
      .Exec = ORAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111000,
      .value = 0b10111000,
      .Exec = CPAR8,
  };

  // Block 3
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11001110,
      .Exec = ADDAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11001110,
      .Exec = ADCAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11010110,
      .Exec = SUBAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11011110,
      .Exec = SBCAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11100110,
      .Exec = ANDAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11101110,
      .Exec = XORAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11110110,
      .Exec = ORAR8,
  };
  instructions[instruction_size++] = (Instruction){
      .mask = 0b11111111,
      .value = 0b11111110,
      .Exec = CPAR8,
  };
}
