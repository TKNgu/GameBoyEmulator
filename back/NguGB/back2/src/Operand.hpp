#ifndef Operand_hpp
#define Operand_hpp

#include <cstdint>
#include <string>

class Operand {
public:
  inline Operand(bool size, void *address, bool signed8 = false)
      : size(size), address(address), signed8(signed8) {
    this->half = *((uint8_t *)(this->address)) & 0x0f;
  }
  virtual ~Operand() {}
  inline bool is8() { return this->size; }
  inline bool isE8() { return this->signed8; }
  inline virtual bool flag() { return false; }
  inline uint8_t read8() { return *(uint8_t *)(this->address); }
  inline int8_t readE8() { return *(int8_t *)(this->address); }
  inline void write8(uint8_t value) { *(uint8_t *)(this->address) = value; }
  inline uint16_t read16() { return *(uint16_t *)(this->address); }
  inline void write16(uint16_t value) { *(uint16_t *)(this->address) = value; }
  inline uint8_t readHalf() { return this->half; }
  virtual uint8_t readMask() { return 0x00; };

private:
  bool signed8;
  bool size;
  void *address;
  uint8_t half;
};

class OperandFlag : public Operand {
public:
  OperandFlag(uint8_t maskFlag, uint8_t *address)
      : Operand(true, address), maskFlag(maskFlag) {}
  inline bool flag() override { return read8() & this->maskFlag; }

private:
  uint8_t maskFlag;
};

class OperandBit : public Operand {
public:
  OperandBit(uint8_t mask) : Operand(0x00, nullptr), mask(mask) {}
  uint8_t readMask() override { return this->mask; }

private:
  uint8_t mask;
};

#endif
