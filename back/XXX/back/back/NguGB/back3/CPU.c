#include "CPU.h"
#include "Memory.h"
#include "Registers.h"
#include <stdint.h>
#include <stdio.h>

#define OPCODE_SIZE 0x0100
#define CB_OPCODE_SIZE 0x0100

struct {
  void (*opcodes[OPCODE_SIZE])();
  void (*cbOpcodes[OPCODE_SIZE])();
} cpu;

u8 *GetAddressR8(u8 bit) {
  static u8 *mapRegister8[8] = {
      &registers.B, &registers.C, &registers.D, &registers.E,
      &registers.H, &registers.L, NULL,         &registers.A,
  };
  if (bit == 6) {
    return ram + registers.HL;
  }
  return mapRegister8[bit];
}

u8 GetR8(u8 bit) { return *GetAddressR8(bit); }

void SetR8(u8 bit, u8 value) { *GetAddressR8(bit) = value; }

u16 *GetAddressR16(u8 bit) {
  static u16 *mapRegister16[4] = {&registers.BC, &registers.DE, &registers.HL,
                                  &registers.SP};
  return mapRegister16[bit];
}

u16 GetR16(u8 bit) { return *GetAddressR16(bit); }

void SetR16(u8 bit, u16 value) { *GetAddressR16(bit) = value; }

u16 *GetAddressR16STK(u8 bit) {
  static u16 *mapRegister16STK[4] = {&registers.BC, &registers.DE,
                                     &registers.HL, &registers.AF};
  return mapRegister16STK[bit];
}

u16 GetR16STK(u8 bit) { return *GetAddressR16STK(bit); }

void SetR16STK(u8 bit, u16 value) { *GetAddressR16STK(bit) = value; }

u16 GetR16Mem(u8 bit) {
  switch (bit) {
  case 0:
    return registers.BC;
  case 1:
    return registers.DE;
  case 2: {
    u16 tmp = registers.HL;
    registers.HL++;
    return tmp;
  }
  case 3: {
    u16 tmp = registers.HL;
    registers.HL--;
    return tmp;
  }
  }
  return 0;
}

int GetCOND(u8 bit) {
  switch (bit) {
  case 0:
    return !ZFLAG;
  case 1:
    return ZFLAG;
  case 2:
    return !CFLAG;
  case 3:
    return CFLAG;
  }
  return 0;
}

u8 GetB3(u8 bit) { return 0b1 << bit; }

u8 GetTGT3(u8 bit) { return bit >> 3; }

u8 GetIMM8(u16 address) { return ram[address + 1]; }

u16 GetIMM16(u16 address) {
  u8 lo = ram[address + 1];
  u8 hi = ram[address + 2];
  return lo + (hi << 8);
}

size_t indexOpcode;
u8 cb;

#define REGISTER16_OPCODE ((u16 *)(&registers))[(indexOpcode >> 4) & 0b11]

#define COND                                                                   \
  (0b1 << ((indexOpcode >> 3) & 0b11)) &                                       \
      ((ZFLAG ? 0b10 : 0b1) | (CFLAG ? 0b1000 : 0b100))

void NOP() {};

void LD_R16_IMM16() {
  u8 lo = ram[registers.PC++];
  u8 hi = ram[registers.PC++];
  REGISTER16_OPCODE = lo + (hi << 8);
}

void LD_R16Meme_A() { ram[REGISTER16_OPCODE] = registers.A; }

void LD_HLIMeme_A() {
  ram[registers.HL] = registers.A;
  registers.HL++;
}

void LD_HLDMeme_A() {
  ram[registers.HL] = registers.A;
  registers.HL--;
}

void LD_A_R16Meme() { registers.A = ram[REGISTER16_OPCODE]; }

void LD_A_HLIMeme() { registers.A = ram[registers.HL + 1]; }

void LD_A_HLDMeme() { registers.A = ram[registers.HL - 1]; }

void LD_IMM16_SP() {
  u8 lo = ram[registers.PC++];
  u8 hi = ram[registers.PC++];
  size_t address = lo + (hi << 8);
  ram[address] = registers.P;
  ram[address + 1] = registers.S;
}

void INC_R16() { REGISTER16_OPCODE++; }
void DEC_R16() { REGISTER16_OPCODE--; }

void ADD_HL_R16() {
  u16 sum = registers.HL + REGISTER16_OPCODE;
  NFLAGOF;
  sum < registers.HL || sum < REGISTER16_OPCODE ? CFLAGON : CFLAGOF;
  u16 sumH = sum & 0x0fff;
  sumH < (registers.HL & 0x0fff) || sumH < (REGISTER16_OPCODE & 0x0fff)
      ? HFLAGON
      : HFLAGOF;
}

void INC_R8() {
  u8 *r8 = GetAddressR8((indexOpcode >> 3) & 0b111);
  u8 tmp = ++(*r8);
  tmp ? ZFLAGOF : ZFLAGON;
  NFLAGON;
  tmp & 0x0011 ? HFLAGON : HFLAGOF;
}

void DEC_R8() {
  u8 *r8 = GetAddressR8((indexOpcode >> 3) & 0b111);
  u8 tmp = --(*r8);
  tmp ? ZFLAGOF : ZFLAGON;
  NFLAGON;
  (tmp & 0b11) == 0b11 ? HFLAGON : HFLAGOF;
}

void LD_R8_IMM8() {
  u8 *r8 = GetAddressR8((indexOpcode >> 3) & 0b111);
  *r8 = ram[registers.PC++];
}

void RLCA() {
  u8 tmp = registers.A;
  u8 carry = tmp & 0x80;
  registers.A = carry ? (tmp << 1) | 0x01 : (tmp << 1) & 0xfe;
  registers.F = carry ? 0x08 : 0x00;
}

void RRCA() {
  u8 tmp = registers.A;
  u8 carry = tmp & 0x01;
  registers.A = carry ? (tmp >> 1) | 0x80 : (tmp << 1) & 0x7f;
  registers.F = carry ? 0x08 : 0x00;
}

void RLA() {
  registers.F = registers.A & 0x80 ? 0x08 : 0x00;
  registers.A <<= 1;
}

void RRA() {
  registers.F = registers.A & 0x01 ? 0x08 : 0x00;
  registers.A >>= 1;
}

void DAA() {
  if (NFLAG) {
    if (CFLAG) {
      registers.A -= 0x60;
    }
    if (HFLAG) {
      registers.A -= 0x06;
    }
  } else {
    if (CFLAG || registers.A > 0x99) {
      registers.A += 0x60;
      CFLAGON;
    }
    if (HFLAG || (registers.A & 0x0f) > 0x09) {
      registers.A += 0x06;
    }
  }
  registers.A ? ZFLAGOF : ZFLAGON;
  HFLAGOF;
}

void CPL() {
  registers.A ^= 0xff;
  NFLAGON;
  HFLAGON;
}

void SCF() {
  registers.F &= 0b10010000;
  registers.F |= 0b00010000;
}

void CCF() {
  registers.F ^= 0b00010000;
  registers.F &= 0b10010000;
}

void JR_E8() {
  u8 tmp = ram[registers.PC++];
  registers.PC += (int8_t)(tmp);
}

void JR_COND_E8() {
  int8_t tmp = (int8_t)(ram[registers.PC++]);
  if ((COND)) {
    registers.PC += tmp;
  }
}

void STOP() {
  // TODO
}

void LD_R8_R8() {
  if (indexOpcode == 0x76) {
    return;
  }
  (*GetAddressR8((indexOpcode >> 3) & 0b111)) =
      (*GetAddressR8(indexOpcode & 0b111));
}

void ADD_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  u8 sum = registers.A + tmp;
  sum ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  u8 half = sum & 0x0f;
  half < (tmp & 0x0f) || half < (registers.A & 0x0f) ? HFLAGON : HFLAGOF;
  sum < tmp || sum < registers.A ? CFLAGON : CFLAGOF;
  registers.A = sum;
}

void ADC_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  u8 sum = registers.A + tmp + ZFLAG ? 1 : 0;
  sum ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  u8 half = sum & 0x0f;
  half < (tmp & 0x0f) || half < (registers.A & 0x0f) ? HFLAGON : HFLAGOF;
  sum < tmp || sum < registers.A ? CFLAGON : CFLAGOF;
  registers.A = sum;
}

void SUB_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  NFLAGON;
  (registers.A & 0x0f) > (tmp & 0x0f) ? HFLAGOF : HFLAGON;
  registers.A > tmp ? CFLAGOF : CFLAGON;
  registers.A -= tmp;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void SBC_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  NFLAGON;
  (registers.A & 0x0f) > (tmp & 0x0f) ? HFLAGOF : HFLAGON;
  registers.A > tmp ? CFLAGOF : CFLAGON;
  registers.A -= (tmp + (CFLAG ? 1 : 0));
  registers.A ? ZFLAGOF : ZFLAGON;
}

void AND_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  registers.A &= tmp;
  registers.F = 0b00100000;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void XOR_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  registers.A ^= tmp;
  registers.F = 0b00000000;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void OR_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  registers.A |= tmp;
  registers.F = 0b00000000;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void CP_A_R8() {
  u8 tmp = *GetAddressR8(indexOpcode & 0b111);
  u8 sub = registers.A - tmp;
  sub ? ZFLAGOF : ZFLAGON;
  NFLAGON;
  (registers.A & 0x0f) > (tmp & 0x0f) ? HFLAGOF : HFLAGON;
  registers.A > tmp ? CFLAGOF : CFLAGON;
}

void ADD_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  u8 sum = registers.A + tmp;
  sum ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  u8 half = sum & 0x0f;
  half < (tmp & 0x0f) || half < (registers.A & 0x0f) ? HFLAGON : HFLAGOF;
  sum < tmp || sum < registers.A ? CFLAGON : CFLAGOF;
}

void ADC_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  u8 sum = registers.A + tmp + ZFLAG ? 1 : 0;
  sum ? ZFLAGOF : ZFLAGON;
  NFLAGOF;
  u8 half = sum & 0x0f;
  half < (tmp & 0x0f) || half < (registers.A & 0x0f) ? HFLAGON : HFLAGOF;
  sum < tmp || sum < registers.A ? CFLAGON : CFLAGOF;
  registers.A = sum;
}

void SUB_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  NFLAGON;
  (registers.A & 0x0f) > (tmp & 0x0f) ? HFLAGOF : HFLAGON;
  registers.A > tmp ? CFLAGOF : CFLAGON;
  registers.A -= tmp;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void SBC_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  NFLAGON;
  (registers.A & 0x0f) > (tmp & 0x0f) ? HFLAGOF : HFLAGON;
  registers.A > tmp ? CFLAGOF : CFLAGON;
  registers.A -= (tmp + CFLAG ? 1 : 0);
  registers.A ? ZFLAGOF : ZFLAGON;
}

void AND_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  registers.A &= tmp;
  registers.F = 0b00100000;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void XOR_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  registers.A ^= tmp;
  registers.F = 0b00000000;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void OR_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  registers.A |= tmp;
  registers.F = 0b00000000;
  registers.A ? ZFLAGOF : ZFLAGON;
}

void CP_A_IMM8() {
  u8 tmp = ram[registers.PC++];
  NFLAGON;
  (registers.A & 0x0f) > (tmp & 0x0f) ? HFLAGOF : HFLAGON;
  registers.A > tmp ? CFLAGOF : CFLAGON;
  u8 sub = registers.A - tmp;
  sub ? ZFLAGOF : ZFLAGON;
}

void RET() {
  u8 lo = ram[registers.SP++];
  u8 hi = ram[registers.SP++];
  registers.PC = lo + (hi << 8);
}

void RETI() {
  u8 lo = ram[registers.SP++];
  u8 hi = ram[registers.SP++];
  registers.PC = lo + (hi << 8);
}

void RET_COND() {
  if (!(COND)) {
    return;
  }
  RET();
}

void JP() {
  u8 lo = ram[registers.PC++];
  u8 hi = ram[registers.PC];
  registers.PC = lo + (hi << 8);
}

void JP_HL() { registers.PC = registers.HL; }

void JP_COND() {
  if (!(COND)) {
    return;
  }
  JP();
}

void CALL() {
  u8 lo = ram[registers.PC++];
  u8 hi = ram[registers.PC++];
  ram[--registers.SP] = registers.PC >> 8;
  ram[--registers.SP] = registers.PC;
  registers.PC = lo + (hi << 8);
}

void CALL_COND() {
  if (!(COND)) {
    return;
  }
  CALL();
}

void RST() {
  ram[--registers.SP] = registers.PC >> 8;
  ram[--registers.SP] = registers.PC;
  registers.PC = (indexOpcode >> 3) & 0b111;
}

void POP() {
  u8 lo = ram[registers.SP++];
  u8 hi = ram[registers.SP++];
  REGISTER16_OPCODE = lo + (hi << 8);
}

void PUSH() {
  u16 tmp = REGISTER16_OPCODE;
  ram[--registers.SP] = tmp >> 8;
  ram[--registers.SP] = tmp;
}

void RLC() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x80;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = carry ? (tmp << 1) | 0x01 : (tmp << 1) & 0xfe;
  *r8 ? ZFLAGOF : ZFLAGON;
}

void RRC() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x01;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = carry ? (tmp >> 1) | 0x80 : (tmp << 1) & 0x7f;
  *r8 ? ZFLAGOF : ZFLAGON;
}

void RL() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x80;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = tmp << 1;
  *r8 ? ZFLAGOF : ZFLAGON;
}

void RR() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x01;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = tmp >> 1;
  *r8 ? ZFLAGOF : ZFLAGON;
}

void SLA() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x80;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = tmp << 1;
  *r8 ? ZFLAGOF : ZFLAGON;
}

void SRA() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x01;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = (tmp >> 1) + (tmp & 0x80);
  *r8 ? ZFLAGOF : ZFLAGON;
}

void SWAP() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  registers.F = 0x00;
  *r8 = (tmp >> 4) + (tmp << 4);
  *r8 ? ZFLAGOF : ZFLAGON;
}

void SRL() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  u8 tmp = *r8;
  u8 carry = tmp & 0x01;
  registers.F = carry ? 0x08 : 0x00;
  *r8 = tmp >> 1;
  *r8 ? ZFLAGOF : ZFLAGON;
}

void BIT() {
  u8 tmp = *GetAddressR8(cb & 0b00000111);
  NFLAGOF;
  HFLAGON;
  tmp & (0x01 << ((cb >> 3) & 0b00000111)) ? ZFLAGOF : ZFLAGON;
}

void RES() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  *r8 = (*r8) & (~(0x01 << ((cb >> 3) & 0b111)));
}

void SET() {
  u8 *r8 = GetAddressR8(cb & 0b00000111);
  *r8 = (*r8) | (0x01 << ((cb >> 3) & 0b111));
}

void Prefixed() {
  cb = ram[registers.PC++];
  cpu.cbOpcodes[cb]();
}

void LDH_A8_A() {
  u8 lo = ram[registers.PC++];
  ram[0xff00 + lo] = registers.A;
}

void LDH_C_A() { ram[0xff00 + registers.C] = registers.A; }

void LD_A16_A() {
  u8 lo = ram[registers.PC++];
  u8 hi = ram[registers.PC++];
  ram[lo + (hi << 8)] = registers.A;
}

void LDH_A_A8() { registers.A = ram[0xff00 + ram[registers.PC++]]; }

void LDH_A_C() { registers.A = ram[0xff00 + registers.C]; }

void LD_A_A16() {
  u8 lo = ram[registers.PC++];
  u8 hi = ram[registers.PC++];
  registers.A = ram[lo + (hi << 8)];
}

void ADD_SP_E8() {
  int8_t tmp = ram[registers.PC++];
  u8 sum = registers.SP + tmp;
  ZFLAGOF;
  NFLAGOF;
  sum >> 8 == registers.S ? CFLAGOF : CFLAGON;
  (sum & 0xf0) == (registers.SP & 0xf0) + (tmp & 0xf0) ? HFLAGOF : HFLAGON;
  registers.SP = sum;
}

void LD_HL_ADD_SP_E8() {
  int8_t tmp = ram[registers.PC++];
  u8 sum = registers.SP + tmp;
  ZFLAGOF;
  NFLAGOF;
  sum >> 8 == registers.S ? CFLAGOF : CFLAGON;
  (sum & 0xf0) == (registers.SP & 0xf0) + (tmp & 0xf0) ? HFLAGOF : HFLAGON;
  registers.HL = sum;
}

void LD_SP_HL() { registers.SP = registers.HL; }

void DI() {}

void EI() {}

void GenOpcode() {
  void (**opcodes)() = cpu.opcodes;
  for (size_t index = 1; index < OPCODE_SIZE; index++) {
    switch (index & 0b11000000) {
    case 0b00000000: {
      switch (index & 0b00001111) {
      case 0b00000001:
        opcodes[index] = LD_R16_IMM16;
        break;
      case 0b00000010:
        opcodes[index] = LD_R16Meme_A;
        break;
      case 0b00001010:
        opcodes[index] = LD_A_R16Meme;
        break;
      case 0b00001000:
        opcodes[index] = LD_IMM16_SP;
        break;
      case 0b00000011:
        opcodes[index] = INC_R16;
        break;
      case 0b00001011:
        opcodes[index] = DEC_R16;
        break;
      case 0b00001001:
        opcodes[index] = ADD_HL_R16;
        break;
      }

      switch (index & 0b00000111) {
      case 0b00000100:
        opcodes[index] = INC_R8;
        break;
      case 0b00000101:
        opcodes[index] = DEC_R8;
        break;
      case 0b00000110:
        opcodes[index] = LD_R8_IMM8;
        break;
      }

      if ((index & 0b00100111) == 0b00100000) {
        opcodes[index] = JR_COND_E8;
      }

    } break;
    case 0b01000000:
      opcodes[index] = LD_R8_R8;
      break;
    case 0b10000000: {
      switch (index & 0b00111000) {
      case 0b00000000:
        opcodes[index] = ADD_A_R8;
        break;
      case 0b00001000:
        opcodes[index] = ADC_A_R8;
        break;
      case 0b00010000:
        opcodes[index] = SUB_A_R8;
        break;
      case 0b00011000:
        opcodes[index] = SBC_A_R8;
        break;
      case 0b00100000:
        opcodes[index] = AND_A_R8;
        break;
      case 0b00101000:
        opcodes[index] = XOR_A_R8;
        break;
      case 0b00110000:
        opcodes[index] = OR_A_R8;
        break;
      case 0b00111000:
        opcodes[index] = CP_A_R8;
        break;
      }

    } break;
    case 0b11000000: {
      switch (index & 0b00000111) {
      case 0b00000000:
        opcodes[index] = RET_COND;
        break;
      case 0b00000010:
        opcodes[index] = JP_COND;
        break;
      case 0b00000100:
        opcodes[index] = CALL_COND;
        break;
      case 0b00000111:
        opcodes[index] = RST;
        break;
      }

      switch (index & 0b00001111) {
      case 0b00000001:
        opcodes[index] = POP;
        break;
      case 0b00000101:
        opcodes[index] = PUSH;
        break;
      }

    } break;
    }
  }

  opcodes[0x00] = NOP;
  opcodes[0x22] = LD_HLIMeme_A;
  opcodes[0x2A] = LD_A_HLIMeme;
  opcodes[0x32] = LD_HLDMeme_A;
  opcodes[0x3A] = LD_A_HLDMeme;
  opcodes[0x07] = RLCA;
  opcodes[0x0f] = RRCA;
  opcodes[0x17] = RLA;
  opcodes[0x1f] = RRA;
  opcodes[0x27] = DAA;
  opcodes[0x2F] = CPL;
  opcodes[0x37] = SCF;
  opcodes[0x3F] = CCF;
  opcodes[0x18] = JR_E8;
  opcodes[0x10] = STOP;
  opcodes[0xc6] = ADD_A_IMM8;
  opcodes[0xce] = ADC_A_IMM8;
  opcodes[0xd6] = SUB_A_IMM8;
  opcodes[0xde] = SBC_A_IMM8;
  opcodes[0xe6] = AND_A_IMM8;
  opcodes[0xee] = XOR_A_IMM8;
  opcodes[0xf6] = OR_A_IMM8;
  opcodes[0xfe] = CP_A_IMM8;
  opcodes[0xc9] = RET;
  opcodes[0xd9] = RETI;
  opcodes[0xc3] = JP;
  opcodes[0xe9] = JP_HL;
  opcodes[0xcd] = CALL;
  opcodes[0xcb] = Prefixed;
  opcodes[0xe0] = LDH_A8_A;
  opcodes[0xe2] = LDH_C_A;
  opcodes[0xea] = LD_A16_A;
  opcodes[0xf0] = LDH_A_A8;
  opcodes[0xf2] = LDH_A_C;
  opcodes[0xfa] = LD_A_A16;
  opcodes[0xe8] = ADD_SP_E8;
  opcodes[0xF8] = LD_HL_ADD_SP_E8;
  opcodes[0xF9] = LD_SP_HL;
  opcodes[0xF3] = DI;
  opcodes[0xFB] = EI;

  void (**cbOpcodes)() = cpu.cbOpcodes;
  for (size_t index = 0; index < CB_OPCODE_SIZE; index++) {
    switch (index & 0b11111000) {
    case 0b00000000:
      cbOpcodes[index] = RLC;
      break;
    case 0b00001000:
      cbOpcodes[index] = RRC;
      break;
    case 0b00010000:
      cbOpcodes[index] = RR;
      break;
    case 0b00011000:
      cbOpcodes[index] = RL;
      break;
    case 0b00100000:
      cbOpcodes[index] = SLA;
      break;
    case 0b00101000:
      cbOpcodes[index] = SRA;
      break;
    case 0b00110000:
      cbOpcodes[index] = SWAP;
      break;
    case 0b00111000:
      cbOpcodes[index] = SRL;
      break;
    }
    switch (index & 0b11000000) {
    case 0b01000000:
      cbOpcodes[index] = BIT;
      break;
    case 0b10000000:
      cbOpcodes[index] = RES;
      break;
    case 0b11000000:
      cbOpcodes[index] = SET;
      break;
    }
  }
}

void CPUTick() {
  indexOpcode = ram[registers.PC++];
  cpu.opcodes[indexOpcode]();
}
