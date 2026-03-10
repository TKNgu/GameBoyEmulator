#ifndef Operand_Register_hpp
#define Operand_Register_hpp

#include <cstdint>

#include "Operand.hpp"

class OperandRegister : public Operand {
public:
  OperandRegister(uint16_t &);
  uint8_t read8() override;
  void write8(uint8_t) override;
  uint16_t read16() override;
  void write16(uint16_t) override;
  uint8_t read8H() override;
  void write8H(uint8_t) override;
  uint8_t read8L() override;
  void write8L(uint8_t) override;

private:
  uint16_t &value;
};

#endif
