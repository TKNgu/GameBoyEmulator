#include "OperandRegister.hpp"

OperandRegister::OperandRegister(uint16_t &value) : value(value) {}

uint8_t OperandRegister::read8() { return this->value; }

void OperandRegister::write8(uint8_t value) { this->value = value; }

uint16_t OperandRegister::read16() { return this->value; }

void OperandRegister::write16(uint16_t value) { this->value = value; }

uint8_t OperandRegister::read8H() { return (uint8_t)(this->value >> 8); }

void OperandRegister::write8H(uint8_t value) {
  this->value = (value << 8) + this->value & 0xff;
}

uint8_t OperandRegister::read8L() { return (this->value & 0xff); }

void OperandRegister::write8L(uint8_t value) {
  this->value = value + this->value & 0xff00;
}
