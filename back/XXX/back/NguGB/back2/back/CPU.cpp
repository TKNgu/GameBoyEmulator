#include "CPU.hpp"

#include <bitset>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "OperandMemory.hpp"
#include "OperandRegister.hpp"

using namespace std;

CPU::CPU(string opcodesPath) {
  memset(this->memory, 0x00, 0xffff);
  this->registers.AF = 0x0000;
  this->registers.BC = 0x0000;
  this->registers.DE = 0x0000;
  this->registers.HL = 0x0000;
  this->registers.SP = 0x0000;
  this->registers.PC = 0x0100;
  buildOpcodes(opcodesPath);
}

void CPU::show() {
  cout << "CPU Registers" << endl;
  cout << "AF 0b" << bitset<8>(this->registers.A) << endl;
  cout << "BC 0x" << setfill('0') << setw(4) << hex << this->registers.BC
       << endl;
  cout << "DE 0x" << setfill('0') << setw(4) << hex << this->registers.DE
       << endl;
  cout << "HL 0x" << setfill('0') << setw(4) << hex << this->registers.HL
       << endl;
  cout << "SP 0x" << setfill('0') << setw(4) << hex << this->registers.SP
       << endl;
  cout << "PC 0x" << setfill('0') << setw(4) << hex << this->registers.PC;
  cout << " 0x" << setfill('0') << setw(2) << hex
       << (unsigned)(this->memory[this->registers.PC]) << endl;
}

void CPU::tick() {
  // auto instruction = this->memory.read(this->registers.PC);
  // switch (instruction) {
  // case 0x00:
  //   NOP();
  //   break;
  // case 0x01:
  //   LDBCn16();
  //   break;
  // case 0x02:
  //   LDaBCA();
  //   break;
  // case 0x03:
  //   INCBC();
  //   break;
  // case 0x04:
  //   INCB();
  //   break;
  // case 0x05:
  //   DECB();
  //   break;
  // case 0x06:
  //   LDBn8();
  //   break;
  // case 0x07:
  //   RLCA();
  //   break;
  // case 0x08:
  //   LDa16SP();
  //   break;
  // case 0x09:
  //   ADDHLBC();
  //   break;
  // case 0x10:
  //   break;
  // default:
  //   cout << "Not support " << setfill('0') << setw(2) << hex
  //        << (unsigned)(instruction) << endl;
  // }
}

#define ZFLAGOF this->registers.A &= 0b01110000;
#define ZFLAGON this->registers.A |= 0b10000000;
#define NFLAGOF this->registers.A &= 0b10110000;
#define NFLAGON this->registers.A |= 0b01000000;
#define HFLAGOF this->registers.A &= 0b11010000;
#define HFLAGON this->registers.A |= 0b00100000;
#define CFLAGOF this->registers.A &= 0b11100000;
#define CFLAGON this->registers.A |= 0b00010000;

// void CPU::NOP() { this->registers.PC++; }
//
// void CPU::LDBCn16() {
//   this->registers.BC = this->memory.read16(this->registers.PC + 1);
//   this->registers.PC += 3;
// }
//
// void CPU::LDaBCA() {
//   this->memory.write(this->registers.BC, this->registers.A);
//   this->registers.PC++;
// }
//
// void CPU::INCBC() {
//   this->registers.BC++;
//   this->registers.PC++;
// }
//
// #define UPDATE_NH_FLAG(value) \
//   setZFlag(!value); \ setHFlag((value & 0x0f) == 0x00);
//
// void CPU::INCB() {
//   this->registers.B++;
//   UPDATE_NH_FLAG(this->registers.B)
//   NFLAGOF
//   this->registers.PC++;
// }
//
// void CPU::DECB() {
//   this->registers.B--;
//   UPDATE_NH_FLAG(this->registers.B)
//   NFLAGON
//   this->registers.PC++;
// }
//
// void CPU::LDBn8() {
//   this->registers.B = this->memory.read(++this->registers.PC);
//   this->registers.PC++;
// }
//
// void CPU::RLCA() {
//   auto tmp = this->registers.A & 0x10000000;
//   this->registers.A <<= 1;
//   tmp ? this->registers.A | 0x00000001 : this->registers.A &= 0x11111110;
//   this->registers.A &= 0x00001111;
//   setCFlag(tmp);
//   this->registers.PC++;
// }
//
// void CPU::LDa16SP() {
//   memory.write16(memory.read16(this->registers.PC + 1), this->registers.SP);
//   this->registers.PC += 3;
// }
//
// void CPU::ADDHLBC() {
//   setHFlag((this->registers.L + this->registers.C) > 0xff);
//   auto tmp = this->registers.HL;
//   this->registers.HL += this->registers.BC;
//   NFLAGOF;
//   setCFlag(this->registers.HL < tmp);
//   this->registers.PC++;
// }
//
// void CPU::LDAaBC() {
//   this->registers.A = this->memory.read16(this->registers.BC);
//   this->registers.PC++;
// }

unique_ptr<Operand> CPU::buildOperand(nlohmann::json operand) {
  auto name = operand["name"].get<string>();
  if (operand["immediate"].get<bool>()) {
    cout << "Build immediate ";
    if (name == "AF" || name == "A") {
      cout << "operand registers A/AF" << endl;
      return make_unique<OperandRegister>(this->registers.AF);
    } else if (name == "BC" || name == "B" || name == "C") {
      cout << "operand registers BC/B/C" << endl;
      return make_unique<OperandRegister>(this->registers.BC);
    } else if (name == "DE" || name == "D" || name == "E") {
      cout << "operand registers DE/D/E" << endl;
      return make_unique<OperandRegister>(this->registers.DE);
    } else if (name == "HL" || name == "H" || name == "L") {
      cout << "operand registers HL/H/L" << endl;
      return make_unique<OperandRegister>(this->registers.HL);
    } else if (name == "SP") {
      cout << "operand registers SP" << endl;
      return make_unique<OperandRegister>(this->registers.SP);
    } else if (name == "PC") {
      cout << "operand registers PC" << endl;
      return make_unique<OperandRegister>(this->registers.PC);
    } else if (name == "n16" || name == "n8") {
      cout << "operand memory n16/n8" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.PC + 1);
    }
  } else {
    cout << "Build address ";
    if (name == "AF") {
      cout << "operand memory AF" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.AF);
    } else if (name == "BC") {
      cout << "operand memory BC" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.BC);
    } else if (name == "DE") {
      cout << "operand memory DE" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.DE);
    } else if (name == "HL") {
      cout << "operand memory HL" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.HL);
    } else if (name == "SP") {
      cout << "operand memory SP" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.SP);
    } else if (name == "PC") {
      cout << "operand memory PC" << endl;
      return make_unique<OperandMemory>(this->memory, this->registers.PC);
    } else if (name == "n16") {
      cout << "operand memory n16" << endl;
      auto address = this->memory[this->registers.PC + 1] +
                     (this->memory[this->registers.PC + 1] << 8);
      return make_unique<OperandMemory>(this->memory, address);
    }
  }
  throw runtime_error("Not support " + to_string(operand));
}

void CPU::buildOpcodes(string opcodesPath) {
  ifstream fileOpcodes(opcodesPath);
  auto data = nlohmann::json::parse(fileOpcodes);
  auto index = 0;
  for (auto &instruction : data["unprefixed"]) {
    cout << instruction << endl;
    index++;
    auto mnemonic = instruction["mnemonic"].get<string>();
    auto size = instruction["bytes"].get<unsigned>();
    if (mnemonic == "NOP") {
      cout << "NOP" << endl;
      this->opcodes[index] = [this]() { this->registers.PC++; };
    } else if (mnemonic == "LD") {
      this->opcodes[index] = [this, instruction, size]() {
        auto operands = instruction["operands"];
        auto operandA = buildOperand(operands[0]);
        auto operandB = buildOperand(operands[1]);
        if (operands[1]["bytes"].get<unsigned>() == 2) {
          cout << "LD size 16" << endl;
          operandA->write16(operandB->read16());
        } else {
          cout << "LD size 8" << endl;
          operandA->write8(operandB->read8());
        }
        this->registers.PC += size;
      };
    } else if (mnemonic == "INC") {
      auto operands = instruction["operands"];
      auto operandA = buildOperand(operands[0]);

    } else {
      throw runtime_error("instruction not support");
    }
    if (index > 16) {
      break;
    }
  }
}
