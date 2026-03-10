#ifndef Memory_h
#define Memory_h

#include "Common.h"

#define RAM_SIZE 0x10000 // 8k

extern u8 ram[RAM_SIZE];

void InitMemory();
int LoadBootRom(char *filePath);
int LoadRom(char *filePath);
void UnmapBootROM();

#endif
