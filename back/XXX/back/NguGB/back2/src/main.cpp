#include <cstring>
#include <fstream>
#include <iostream>

#include "CPU.hpp"
#include "LCD.hpp"
#include "json.hpp"

using namespace std;

struct Sample {
  bool is8;
  void *address;
};

bool Load(string, uint8_t *);

int main(int, char *[]) {
  cout << "Create Ram" << endl;
  uint8_t ram[0xffff];

  cout << "Create CPU" << endl;
  CPU cpu("data/Opcodes.json", ram);
  cpu.test();

  cout << "Create GPU" << endl;
  LCD lcd(ram);

  cout << "Load rom" << endl;
  memset(ram, 0x00, 0xffff);
  if (!Load("data/cpu_instrs.gb", ram)) {
    cout << "Error load rom" << endl;
    return 0;
  }

  cout << "Boot Up" << endl;
  cpu.bootUp();
  lcd.bootUp();

  // lcd.test();

  cout << "Loop" << endl;
  int count = 0;
  while (count++ < 10) {
    cpu.show();
    cpu.tick();
    lcd.tick();
    lcd.render();
    lcd.input();
  }
  return 0;
}

bool Load(string romFile, uint8_t *ram) {
  FILE *fileRom = fopen(romFile.c_str(), "rb");
  if (!fileRom) {
    return false;
  }
  fseek(fileRom, 0L, SEEK_END);
  auto fileSize = ftell(fileRom);
  if (fileSize < 0x8000) {
    fclose(fileRom);
    return false;
  }
  fseek(fileRom, 0L, SEEK_SET);
  fread(ram, sizeof(uint8_t), 0x8000, fileRom);
  fclose(fileRom);
  return true;
}
