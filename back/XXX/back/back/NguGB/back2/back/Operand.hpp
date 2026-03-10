#ifndef Operand_hpp
#define Operand_hpp

#include <cstdint>

class Operand {
public:
  ~Operand() {}
  virtual uint8_t read8() = 0;
  virtual void write8(uint8_t) = 0;
  virtual uint16_t read16() = 0;
  virtual void write16(uint16_t) = 0;
  virtual uint8_t read8H() = 0;
  virtual void write8H(uint8_t) = 0;
  virtual uint8_t read8L() = 0;
  virtual void write8L(uint8_t) = 0;

  void show();
};

#endif
