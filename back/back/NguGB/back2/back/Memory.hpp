#ifndef Memory_hpp
#define Memory_hpp

#include <cstdint>
#include <cstring>
#include <string>

#define MEMORY_SIZE 0xffff

class Memory {
public:
  inline Memory() { memset(this->memory, 0x00, MEMORY_SIZE); }
  inline uint8_t read(uint16_t address) { return this->memory[address]; }
  inline void write(uint16_t address, uint8_t data) {
    this->memory[address] = data;
  }
  inline uint16_t read16(uint16_t address) {
    return this->memory[address] + (this->memory[address + 1] << 8);
  }
  inline void write16(uint16_t address, uint16_t data) {
    this->memory[address] = (uint8_t)(data);
    this->memory[address + 1] = (uint8_t)(data >> 8);
  }
  void show(uint16_t);
  bool readCartridge(std::string);

private:
  uint8_t memory[MEMORY_SIZE];
};

#endif
