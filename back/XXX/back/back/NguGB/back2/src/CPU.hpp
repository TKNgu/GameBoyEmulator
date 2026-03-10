#ifndef CPU_hpp
#define CPU_hpp

#include <cstdint>
#include <memory>

#include "Opcode.hpp"
#include "Operand.hpp"
#include "json.hpp"

class CPU {
public:
  CPU(const std::string, uint8_t *);
  void bootUp();
  void show() const;
  void test();
  void tick();

private:
  struct {
    union {
      uint16_t AF;
      struct {
        uint8_t F;
        uint8_t A;
      };
    };
    union {
      uint16_t BC;
      struct {
        uint8_t C;
        uint8_t B;
      };
    };
    union {
      uint16_t DE;
      struct {
        uint8_t E;
        uint8_t D;
      };
    };
    union {
      uint16_t HL;
      struct {
        uint8_t L;
        uint8_t H;
      };
    };
    uint16_t SP;
    uint16_t PC;
  } registers;
  uint8_t *ram;
  std::function<void()> opcodes[0xff + 1];
  std::function<void()> cbOpcodes[0xff + 1];
  bool ime;

private:
#define ZFLAG this->registers.F & 0b10000000
#define NFLAG this->registers.F & 0b01000000
#define HFLAG this->registers.F & 0b00100000
#define CFLAG this->registers.F & 0b00010000

#define ZFLAGOF this->registers.F &= 0b01110000
#define ZFLAGON this->registers.F |= 0b10000000
#define NFLAGOF this->registers.F &= 0b10110000
#define NFLAGON this->registers.F |= 0b01000000
#define HFLAGOF this->registers.F &= 0b11010000
#define HFLAGON this->registers.F |= 0b00100000
#define CFLAGOF this->registers.F &= 0b11100000
#define CFLAGON this->registers.F |= 0b00010000

private:
  void buildOpcodes(const nlohmann::json &);
  std::function<void()> opcodeFactory(const nlohmann::json &);
  std::function<void()> cbOpcodeFactory(const nlohmann::json &);
  std::unique_ptr<Operand> operandFactory(const nlohmann::json &);
  std::unique_ptr<Operand> flagOperandFactory(const nlohmann::json &);
  std::unique_ptr<Operand> bitOperandFactory(const nlohmann::json &);
  void GB(const nlohmann::json &);
};

#endif
