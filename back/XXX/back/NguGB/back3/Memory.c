#include "Memory.h"
#include <stdio.h>
#include <string.h>

#define BOOT_ROM_SIZE 0x0100
u8 umapCartridge[BOOT_ROM_SIZE];
u8 ram[RAM_SIZE];

void InitMemory() { memset((void *)(ram), 0x00, sizeof(u8) * RAM_SIZE); }

int LoadBootRom(char *filePath) {
  FILE *fileRom = fopen(filePath, "rb");
  if (!fileRom) {
    return EOPEN_FILE_BOOT_ROM;
  }
  fseek(fileRom, 0, SEEK_END);
  size_t bootRomSize = ftell(fileRom);
  if (bootRomSize != BOOT_ROM_SIZE) {
    fclose(fileRom);
    return EFILE_BOOT_ROM_SIZE;
  }
  rewind(fileRom);
  if (bootRomSize != fread((void *)(ram), sizeof(u8), bootRomSize, fileRom)) {
    fclose(fileRom);
    return EREAD_BOOT_ROM;
  }
  fclose(fileRom);
  return 0;
}

int LoadRom(char *filePath) {
  FILE *fileRom = fopen(filePath, "rb");
  if (!fileRom) {
    return EOPEN_FILE_ROM;
  }
  fseek(fileRom, 0, SEEK_END);
  size_t romSize = ftell(fileRom);
  rewind(fileRom);
  romSize -= fread((void *)(umapCartridge), sizeof(u8), BOOT_ROM_SIZE, fileRom);
  romSize -= fread((void *)(ram + BOOT_ROM_SIZE), sizeof(u8), romSize, fileRom);
  fclose(fileRom);
  if (romSize) {
    return EREAD_ROM;
  }
  return 0;
}

void UnmapBootROM() {
  memcpy((void *)(ram), (void *)(umapCartridge), sizeof(u8) * BOOT_ROM_SIZE);
}
