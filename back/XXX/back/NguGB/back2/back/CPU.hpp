#ifndef CPU_hpp
#define CPU_hpp

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "Memory.hpp"
#include "Operand.hpp"
#include "json.hpp"

class CPU {
public:
  CPU(std::string);
  void show();
  void tick();

private:
  uint8_t memory[0xffff];
  struct Registers {
    union {
      uint16_t AF;
      struct {
        uint8_t A;
        uint8_t _;
      };
    };
    union {
      uint16_t BC;
      struct {
        uint8_t B;
        uint8_t C;
      };
    };
    union {
      uint16_t DE;
      struct {
        uint8_t D;
        uint8_t E;
      };
    };
    union {
      uint16_t HL;
      struct {
        uint8_t H;
        uint8_t L;
      };
    };
    uint16_t SP;
    uint16_t PC;
  } registers;
  std::function<void()> opcodes[0xff];

private:
  std::unique_ptr<Operand> buildOperand(nlohmann::json);
  void buildOpcodes(std::string);

  inline bool isZFlag() { return this->registers.A & 0b10000000; }
  inline void setZFlag(bool value) {
    value ? this->registers.A |= 0b10000000 : this->registers.A &= 0b01110000;
  }
  inline bool isNFlag() { return this->registers.A & 0b01000000; }
  inline void setNFlag(bool value) {
    value ? this->registers.A |= 0b01000000 : this->registers.A &= 0b10110000;
  }
  inline bool isHFlag() { return this->registers.A & 0b0010; }
  inline void setHFlag(bool value) {
    value ? this->registers.A |= 0b00100000 : this->registers.A &= 0b11010000;
  }
  inline bool isCFlag() { return this->registers.A & 0b00010000; }
  inline void setCFlag(bool value) {
    value ? this->registers.A |= 0b00010000 : this->registers.A &= 0b11100000;
  }

  // void NOP();
  // void LDBCn16();
  // void LDaBCA();
  // void INCBC();
  // void INCB();
  // void DECB();
  // void LDBn8();
  // void RLCA();
  // void LDa16SP();
  // void ADDHLBC();
  // void LDAaBC();
};

#endif
