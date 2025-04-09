#include "Memory.hpp"

#include <iomanip>
#include <iostream>

using namespace std;

void Memory::show(uint16_t address) {
  cout << "Address 0x" << setfill('0') << setw(4) << hex << address;
  cout << " 0x" << setfill('0') << setw(2) << (unsigned)(this->memory[address])
       << endl;
}

bool Memory::readCartridge(string filePath) {
  FILE *fileRom = fopen(filePath.c_str(), "rb");
  if (!fileRom) {
    return true;
  }
  fseek(fileRom, 0L, SEEK_END);
  auto fileSize = ftell(fileRom);
  if (fileSize < 0x8000) {
    fclose(fileRom);
    return true;
  }
  fseek(fileRom, 0L, SEEK_SET);
  fread(this->memory, sizeof(uint8_t), 0x8000, fileRom);
  fclose(fileRom);
  return false;
}
