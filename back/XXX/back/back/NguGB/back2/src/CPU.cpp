#include "CPU.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

#include "Debug.hpp"

using namespace std;

CPU::CPU(const string opcodePath, uint8_t *ram) : ram(ram) {
  ifstream fileOpcodes(opcodePath);
  auto data = nlohmann::json::parse(fileOpcodes);
  buildOpcodes(data);
}

void CPU::bootUp() {
  memset(&this->registers, 0x00, sizeof(this->registers));
  this->registers.PC = 0x0100;
  this->ime = false;
}

void CPU::show() const {
  cout << "=========== CPU State ===========" << endl;
  cout << "A  " << ShowHex(this->registers.A) << endl;
  cout << "F  " << ShowBin(this->registers.F) << endl;
  cout << "BC " << ShowHex(this->registers.BC) << endl;
  cout << "DE " << ShowHex(this->registers.DE) << endl;
  cout << "HL " << ShowHex(this->registers.HL) << endl;
  cout << "SP " << ShowHex(this->registers.SP) << endl;
  cout << "PC " << ShowHex(this->registers.PC) << endl;
  cout << endl;
}

void CPU::tick() { this->opcodes[this->ram[this->registers.PC]](); }

void CPU::buildOpcodes(const nlohmann::json &data) {
  auto index = 0u;
  for (const auto &opcode : data["unprefixed"]) {
    this->opcodes[index++] = opcodeFactory(opcode);
  }
  index = 0u;
  for (const auto &opcode : data["cbprefixed"]) {
    this->cbOpcodes[index++] = cbOpcodeFactory(opcode);
  }
}

function<void()> CPU::opcodeFactory(const nlohmann::json &data) {
  auto name = data["mnemonic"].get<string>();
  auto size = data["bytes"].get<unsigned>();
  if (name == "NOP") {
    return [this, size]() { this->registers.PC += size; };
  } else if (name == "LD") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      if (operandA->is8()) {
        operandA->write8(operandB->read8());
      } else {
        operandA->write16(operandB->read16());
      }
      this->registers.PC += size;
    };
  } else if (name == "INC") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      if (operandA->is8()) {
        auto value = operandA->read8();
        value++;
        operandA->write8(value);
        value ? ZFLAGOF : ZFLAGON;
        NFLAGOF;
        auto half = value & 0x0f;
        half == 0x00 ? HFLAGON : HFLAGOF;
      } else {
        operandA->write16(operandA->read16() + 1);
      }
      this->registers.PC += size;
    };
  } else if (name == "DEC") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      if (operandA->is8()) {
        auto value = operandA->read8();
        value--;
        operandA->write8(value);
        value ? ZFLAGOF : ZFLAGON;
        NFLAGON;
        auto half = value & 0x0f;
        half == 0x0f ? HFLAGON : HFLAGOF;
      } else {
        operandA->write16(operandA->read16() - 1);
      }
      this->registers.PC += size;
    };
  } else if (name == "RLCA") {
    return [this, data, size]() {
      auto tmp = this->registers.A;
      auto carry = tmp & 0b10000000;
      tmp <<= 1;
      ZFLAGOF;
      NFLAGOF;
      HFLAGOF;
      carry ? CFLAGON : CFLAGOF;
      CFLAG ? tmp |= 0b00000001 : tmp &= 0b11111110;
      this->registers.A = tmp;
      this->registers.PC += size;
    };
  } else if (name == "ADD") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      if (operandA->is8()) {
        if (operandB->isE8()) {
          auto last = this->registers.SP;
          auto half = this->registers.SP & 0xfff0;
          this->registers.SP += operandB->readE8();
          half != (this->registers.SP & 0xfff0) ? HFLAGON : HFLAGOF;
          ZFLAGON;
          if (operandB->readE8() > 0) {
            last > this->registers.SP ? CFLAGON : CFLAGOF;
          } else {
            last < this->registers.SP ? CFLAGON : CFLAGOF;
          }
        } else {
          auto value = operandA->read8();
          auto halfA = operandA->read8() & 0x0f;
          auto halfB = operandB->read8() & 0x0f;
          operandA->write8(operandA->read8() + operandB->read8());
          value > operandA->read8() ? CFLAGON : CFLAGOF;
          (halfA + halfB) > 0x0f ? HFLAGON : HFLAGOF;
          operandA->read8() ? ZFLAGON : ZFLAGOF;
        }
      } else {
        auto value = operandA->read16();
        auto halfA = operandA->read16() & 0x0f;
        auto halfB = operandB->read16() & 0x0f;
        operandA->write16(operandA->read16() + operandB->read16());
        value > operandA->read16() ? CFLAGON : CFLAGOF;
        (halfA + halfB) > 0x0f ? HFLAGON : HFLAGOF;
      }
      NFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "RRCA") {
    return [this, data, size]() {
      auto tmp = this->registers.A;
      auto carry = tmp & 0b00000001;
      tmp >>= 1;
      ZFLAGOF;
      NFLAGOF;
      HFLAGOF;
      carry ? CFLAGON : CFLAGOF;
      CFLAG ? tmp |= 0b10000000 : tmp &= 0b01111111;
      this->registers.A = tmp;
      this->registers.PC += size;
    };
  } else if (name == "STOP") {
    return [this, size]() { this->registers.PC += size; };
  } else if (name == "RLA") {
    return [this, data, size]() {
      auto tmp = this->registers.A;
      auto carry = tmp & 0b10000000;
      tmp <<= 1;
      ZFLAGOF;
      NFLAGOF;
      HFLAGOF;
      CFLAG ? tmp |= 0b00000001 : tmp &= 0b11111110;
      carry ? CFLAGON : CFLAGOF;
      this->registers.A = tmp;
      this->registers.PC += size;
    };
  } else if (name == "JR") {
    return [this, data, size]() {
      if (data["operands"].size() == 1) {
        auto operandA = operandFactory(data["operands"][0]);
        this->registers.PC += operandA->readE8();
      } else {
        auto flagOperand = flagOperandFactory(data["operands"][0]);
        auto operandA = operandFactory(data["operands"][1]);
        if (flagOperand->flag()) {
          this->registers.PC += operandA->readE8();
        } else {
          this->registers.PC += size;
        }
      }
    };
  } else if (name == "RRA") {
    return [this, data, size]() {
      auto tmp = this->registers.A;
      auto carry = tmp & 0b00000001;
      tmp >>= 1;
      CFLAG ? tmp |= 0b10000000 : tmp &= 0b01111111;
      this->registers.A = tmp;
      ZFLAGOF;
      NFLAGOF;
      HFLAGOF;
      carry ? CFLAGON : CFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "DAA") {
    return [this, data, size]() {
      if (NFLAG) {
        if (CFLAG) {
          this->registers.A -= 0x60;
        }
        if (HFLAG) {
          this->registers.A -= 0x06;
        }
      } else {
        if (CFLAG || this->registers.A > 0x99) {
          this->registers.A += 0x60;
          CFLAGON;
        }
        if (HFLAG || (this->registers.A & 0x0f) > 0x09) {
          this->registers.A += 0x06;
        }
      }
      this->registers.A ? ZFLAGOF : ZFLAGON;
      HFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "CPL") {
    return [this, data, size]() {
      this->registers.A ^= 0xff;
      NFLAGON;
      HFLAGON;
      this->registers.PC += size;
    };
  } else if (name == "SCF") {
    return [this, data, size]() {
      NFLAGOF;
      HFLAGOF;
      CFLAGON;
      this->registers.PC += size;
    };
  } else if (name == "CCF") {
    return [this, data, size]() {
      NFLAGOF;
      HFLAGOF;
      CFLAG ^ 0x01 ? CFLAGON : CFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "HALT") {
    return [this, data, size]() {
      // TODO
    };
  } else if (name == "ADC") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t value = operandA->read8();
      operandA->write8(operandA->read8() + operandB->read8() +
                       (CFLAG ? 0x01 : 0x00));
      operandA->read8() ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      (halfA + halfB + (CFLAG ? 0x01 : 0x00)) > 0x0f ? HFLAGON : HFLAGOF;
      value > operandA->read8() ? CFLAGON : CFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "SUB") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t tmpA = operandA->read8();
      uint8_t tmpB = operandB->read8();
      operandA->write8(operandA->read8() - operandB->read8());
      NFLAGON;
      tmpA < tmpB ? CFLAGON : CFLAGOF;
      halfA < halfB ? HFLAGON : HFLAGOF;
      operandA->read8() ? ZFLAGOF : ZFLAGON;
      this->registers.PC += size;
    };
  } else if (name == "SBC") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t tmpA = operandA->read8();
      uint8_t tmpB = operandB->read8();
      operandA->write8(operandA->read8() - operandB->read8() -
                       (CFLAG ? 0x01 : 0x00));
      NFLAGON;
      tmpA < tmpB + (CFLAG ? 0x01 : 0x00) ? CFLAGON : CFLAGOF;
      halfA < (halfB + (CFLAG ? 0x01 : 0x00)) ? HFLAGON : HFLAGOF;
      operandA->read8() ? ZFLAGOF : ZFLAGON;
      this->registers.PC += size;
    };
  } else if (name == "AND") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t value = operandA->read8();
      operandA->write8(operandA->read8() & operandB->read8());
      operandA->read8() ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGON;
      CFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "XOR") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t value = operandA->read8();
      operandA->write8(operandA->read8() ^ operandB->read8());
      operandA->read8() ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGOF;
      CFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "OR") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t value = operandA->read8();
      operandA->write8(operandA->read8() | operandB->read8());
      operandA->read8() ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGOF;
      CFLAGOF;
      this->registers.PC += size;
    };
  } else if (name == "CP") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      uint8_t halfA = operandA->read8() & 0x0f;
      uint8_t halfB = operandB->read8() & 0x0f;
      uint8_t value = operandA->read8();
      uint8_t tmp = operandA->read8() - operandB->read8();
      NFLAGON;
      operandA->read8() < operandB->read8() ? CFLAGON : CFLAGOF;
      halfA < halfB ? HFLAGON : HFLAGOF;
      tmp ? ZFLAGOF : ZFLAGON;
      this->registers.PC += size;
    };
  } else if (name == "RET") {
    return [this, data, size]() {
      if (data["operands"].size() == 1) {
        auto flagOperand = flagOperandFactory(data["operands"][0]);
        if (flagOperand->flag()) {
          this->registers.PC = this->ram[this->registers.SP] +
                               (this->ram[this->registers.SP + 1] << 8);
          this->registers.SP += 2;
        } else {
          this->registers.PC += size;
        }
      } else {
        this->registers.PC = this->ram[this->registers.SP] +
                             (this->ram[this->registers.SP + 1] << 8);
        this->registers.SP += 2;
      }
    };
  } else if (name == "POP") {
    return [this, data, size] {
      auto operandA = operandFactory(data["operands"][0]);
      operandA->write16(this->ram[this->registers.SP] +
                        (this->ram[this->registers.SP + 1] << 8));
      this->registers.SP += 2;
    };
  } else if (name == "JP") {
    return [this, data, size]() {
      if (data["operands"].size() == 1) {
        auto operandA = operandFactory(data["operands"][0]);
        this->registers.PC = operandA->read16();
      } else {
        auto flagOperand = flagOperandFactory(data["operands"][0]);
        auto operandA = operandFactory(data["operands"][1]);
        if (flagOperand->flag()) {
          this->registers.PC = operandA->read16();
        } else {
          this->registers.PC += size;
        }
      }
    };
  } else if (name == "CALL") {
    return [this, data, size]() {
      if (data["operands"].size() == 1) {
        auto operandA = operandFactory(data["operands"][0]);
        this->registers.SP -= 2;
        this->ram[this->registers.SP] = this->registers.PC & 0xff;
        this->ram[this->registers.SP + 1] = this->registers.PC >> 8;
        this->registers.PC = operandA->read16();
      } else {
        auto flagOperand = flagOperandFactory(data["operands"][0]);
        auto operandA = operandFactory(data["operands"][1]);
        if (flagOperand->flag()) {
          this->registers.SP -= 2;
          this->ram[this->registers.SP] = this->registers.PC & 0xff;
          this->ram[this->registers.SP + 1] = this->registers.PC >> 8;
          this->registers.PC = operandA->read16();
        } else {
          this->registers.PC += size;
        }
      }
    };
  } else if (name == "PUSH") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      this->registers.SP -= 2;
      this->ram[this->registers.SP] = operandA->read16();
      this->ram[this->registers.SP + 1] = operandA->read16() >> 8;
      this->registers.PC += size;
    };
  } else if (name == "RST") {
    return [this, data, size]() {
      auto name = data["operands"][0]["name"];
      auto address = 0x00;
      if (name == "$00") {
        address = 0x00;
      } else if (name == "$08") {
        address = 0x08;
      } else if (name == "$10") {
        address = 0x10;
      } else if (name == "$18") {
        address = 0x18;
      } else if (name == "$20") {
        address = 0x20;
      } else if (name == "$28") {
        address = 0x28;
      } else if (name == "$30") {
        address = 0x30;
      } else if (name == "$38") {
        address = 0x38;
      }
      this->registers.SP -= 2;
      this->ram[this->registers.SP] = this->registers.PC & 0xff;
      this->ram[this->registers.SP + 1] = this->registers.PC >> 8;
      this->registers.PC = address;
    };
  } else if (name == "LDH") {
    return [this, data, size]() {
      auto operandA = operandFactory(data["operands"][0]);
      auto operandB = operandFactory(data["operands"][1]);
      operandA->write8(operandB->read8());
      this->registers.PC += size;
    };
  } else if (name == "PREFIX") {
    return [this, data, size]() {
      this->cbOpcodes[this->ram[this->registers.PC + 1]]();
      this->registers.PC += 2;
    };
  } else if (name == "RETI") {
    return [this, data, size]() {
      this->registers.PC = this->ram[this->registers.SP] +
                           (this->ram[this->registers.SP + 1] << 8);
      this->registers.SP += 2;
      this->ime = true;
    };
  } else if (name == "DI") {
    return [this, data, size]() {
      this->ime = false;
      this->registers.PC += size;
    };
    cout << data << endl;
  } else if (name == "EI") {
    return [this, data, size]() {
      this->ime = true;
      this->registers.PC += size;
    };
  } else {
    return []() {};
  }
  return []() {};
}

function<void()> CPU::cbOpcodeFactory(const nlohmann::json &data) {
  auto name = data["mnemonic"].get<string>();
  if (name == "RLC") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto tmp = operand->read8();
      auto carry = tmp & 0b10000000;
      tmp <<= 1;
      carry ? CFLAGON : CFLAGOF;
      CFLAG ? tmp |= 0b00000001 : tmp &= 0b11111110;
      tmp ? ZFLAGOF : ZFLAGON;
      operand->write8(tmp);
      NFLAGOF;
      HFLAGOF;
    };
  } else if (name == "RRC") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto tmp = operand->read8();
      auto carry = tmp & 0b00000001;
      tmp >>= 1;
      carry ? CFLAGON : CFLAGOF;
      CFLAG ? tmp |= 0b10000000 : tmp &= 0b01111111;
      tmp ? ZFLAGOF : ZFLAGON;
      operand->write8(tmp);
      NFLAGOF;
      HFLAGOF;
    };
  } else if (name == "RL") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto tmp = operand->read8();
      auto carry = tmp & 0b10000000;
      tmp <<= 1;
      CFLAG ? tmp |= 0b00000001 : tmp &= 0b11111110;
      operand->write8(tmp);
      carry ? CFLAGON : CFLAGOF;
      tmp ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGOF;
    };
  } else if (name == "RR") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto tmp = operand->read8();
      auto carry = tmp & 0b00000001;
      tmp >>= 1;
      CFLAG ? tmp |= 0b10000000 : tmp &= 0b01111111;
      operand->write8(tmp);
      carry ? CFLAGON : CFLAGOF;
      tmp ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGOF;
    };
  } else if (name == "SLA") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto tmp = operand->read8();
      auto carry = tmp & 0b10000000;
      tmp = (tmp << 1) & 0b11111110;
      tmp ? ZFLAGOF : ZFLAGON;
      carry ? CFLAGOF : CFLAGOF;
      operand->write8(tmp);
    };
  } else if (name == "SRA") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto tmp = operand->read8();
      auto carry = tmp & 0b10000000;
      tmp >>= 1;
      carry ? tmp |= 0b10000000 : tmp &= 0b01111111;
      tmp ? ZFLAGOF : ZFLAGON;
      carry ? CFLAGOF : CFLAGOF;
      operand->write8(tmp);
    };
  } else if (name == "SWAP") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto value = operand->read8();
      auto tmp = value & 0x0f;
      value = (value >> 4) + (tmp << 4);
      operand->write8(value);
      value ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGOF;
      CFLAGOF;
    };
  } else if (name == "SRL") {
    return [this, data]() {
      auto operand = operandFactory(data["operands"][0]);
      auto value = operand->read8();
      auto tmp = value & 0b00000001;
      value >>= 1;
      value &= 0b01111111;
      value ? ZFLAGON : ZFLAGOF;
      NFLAGOF;
      HFLAGOF;
      tmp ? CFLAGOF : CFLAGON;
    };
  } else if (name == "BIT") {
    return [this, data]() {
      auto operandBit = bitOperandFactory(data["operands"][0]);
      auto operand = operandFactory(data["operands"][1]);
      operand->read8() & operandBit->readMask() ? ZFLAGOF : ZFLAGON;
      NFLAGOF;
      HFLAGOF;
    };
  } else if (name == "RES") {
    return [this, data]() {
      auto operandBit = bitOperandFactory(data["operands"][0]);
      auto operand = operandFactory(data["operands"][1]);
      uint8_t mask = ~operandBit->readMask();
      operand->write8(operand->read8() & mask);
    };
  } else if (name == "SET") {
    return [this, data]() {
      auto operandBit = bitOperandFactory(data["operands"][0]);
      auto operand = operandFactory(data["operands"][1]);
      uint8_t mask = operandBit->readMask();
      operand->write8(operand->read8() | mask);
    };
  }
  return []() {};
}

unique_ptr<Operand> CPU::operandFactory(const nlohmann::json &data) {
  auto name = data["name"].get<string>();
  auto isImmediate = data["immediate"].get<bool>();
  if (isImmediate) {
    if (name == "AF") {
      return make_unique<Operand>(false, &this->registers.AF);
    } else if (name == "A") {
      return make_unique<Operand>(true, &this->registers.A);
    } else if (name == "BC") {
      return make_unique<Operand>(false, &this->registers.BC);
    } else if (name == "B") {
      return make_unique<Operand>(true, &this->registers.B);
    } else if (name == "C") {
      return make_unique<Operand>(true, &this->registers.C);
    } else if (name == "DE") {
      return make_unique<Operand>(false, &this->registers.DE);
    } else if (name == "D") {
      return make_unique<Operand>(true, &this->registers.D);
    } else if (name == "E") {
      return make_unique<Operand>(true, &this->registers.E);
    } else if (name == "HL") {
      return make_unique<Operand>(false, &this->registers.HL);
    } else if (name == "H") {
      return make_unique<Operand>(true, &this->registers.H);
    } else if (name == "L") {
      return make_unique<Operand>(true, &this->registers.L);
    } else if (name == "SP") {
      return make_unique<Operand>(false, &this->registers.SP);
    } else if (name == "PC") {
      return make_unique<Operand>(false, &this->registers.PC);
    } else if (name == "n8") {
      return make_unique<Operand>(true, this->ram + this->registers.PC + 1);
    } else if (name == "e8") {
      return make_unique<Operand>(true, this->ram + this->registers.PC + 1,
                                  true);
    } else if (name == "a8") {
      return make_unique<Operand>(true, this->ram + this->registers.PC + 1);
    } else if (name == "n16") {
      return make_unique<Operand>(false, this->ram + this->registers.PC + 1);
    } else if (name == "a16") {
      return make_unique<Operand>(false, this->ram + this->registers.PC + 1);
    } else {
      return nullptr;
    }
  }
  if (name == "AF") {
    return make_unique<Operand>(false, this->ram + this->registers.AF);
  } else if (name == "A") {
    return make_unique<Operand>(true, this->ram + 0xff00 + this->registers.A);
  } else if (name == "BC") {
    return make_unique<Operand>(false, this->ram + this->registers.BC);
  } else if (name == "B") {
    return make_unique<Operand>(true, this->ram + 0xff00 + this->registers.B);
  } else if (name == "C") {
    return make_unique<Operand>(true, this->ram + 0xff00 + this->registers.C);
  } else if (name == "DE") {
    return make_unique<Operand>(false, this->ram + this->registers.DE);
  } else if (name == "D") {
    return make_unique<Operand>(true, this->ram + 0xff00 + this->registers.D);
  } else if (name == "E") {
    return make_unique<Operand>(true, &this->registers.E);
  } else if (name == "HL") {
    return make_unique<Operand>(false, this->ram + this->registers.HL);
  } else if (name == "H") {
    return make_unique<Operand>(true, this->ram + 0xff00 + this->registers.H);
  } else if (name == "L") {
    return make_unique<Operand>(true, this->ram + 0xff00 + this->registers.L);
  } else if (name == "SP") {
    return make_unique<Operand>(false, this->ram + this->registers.SP);
  } else if (name == "PC") {
    return make_unique<Operand>(false, this->ram + this->registers.PC);
  } else if (name == "a8") {
    uint8_t address = this->ram[this->registers.PC + 1];
    return make_unique<Operand>(true, this->ram + 0xff00 + address);
  } else if (name == "a16") {
    uint16_t address = this->ram[this->registers.PC + 1] +
                       (this->ram[this->registers.PC + 2] << 8);
    return make_unique<Operand>(false, this->ram + address);
  }
  return nullptr;
}

unique_ptr<Operand> CPU::flagOperandFactory(const nlohmann::json &data) {
  auto name = data["name"].get<string>();
  if (name == "NZ") {
    return make_unique<OperandFlag>(0b11000000, &this->registers.F);
  } else if (name == "Z") {
    return make_unique<OperandFlag>(0b10000000, &this->registers.F);
  } else if (name == "NC") {
    return make_unique<OperandFlag>(0b01010000, &this->registers.F);
  } else if (name == "C") {
    return make_unique<OperandFlag>(0b00010000, &this->registers.F);
  }
  return nullptr;
}

unique_ptr<Operand> CPU::bitOperandFactory(const nlohmann::json &data) {
  auto name = data["name"].get<string>();
  uint8_t mask = name.c_str()[0] - 0x30;
  return make_unique<OperandBit>(mask);
}
