#include "OperandMemory.hpp"

OperandMemory::OperandMemory(uint8_t *memory, uint16_t address)
    : memory(memory), address(address) {}

uint8_t OperandMemory::read8() { return this->memory[this->address]; }

void OperandMemory::write8(uint8_t value) { this->memory[address] = value; }

uint16_t OperandMemory::read16() {
  return this->memory[this->address] + (this->memory[this->address + 1] << 8);
}

void OperandMemory::write16(uint16_t value) {
  this->memory[this->address] = (uint8_t)(value);
  this->memory[this->address + 1] = (uint8_t)(value >> 8);
}

uint8_t OperandMemory::read8H() { return this->memory[this->address + 1]; }
void OperandMemory::write8H(uint8_t value) {
  this->memory[this->address + 1] = value;
}

uint8_t OperandMemory::read8L() { return this->memory[this->address]; }

void OperandMemory::write8L(uint8_t value) {
  this->memory[this->address] = value;
}
