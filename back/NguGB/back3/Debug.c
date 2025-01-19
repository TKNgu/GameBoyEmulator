#include "Debug.h"
#include "Common.h"
#include "Memory.h"
#include "Registers.h"
#include <stdio.h>
#include <string.h>

typedef enum {
  STEP = 0,
  ADDRESS = 1,
  COUNT = 2,
} DebugState;

char cmd[128];
u16 address;
u8 value;
int runCount = 0;
DebugState state = STEP;

void Debug() {
  switch (state) {
  case COUNT:
    if (runCount) {
      runCount--;
      printf("Run %d\n", runCount);
      return;
    } else {
      state = STEP;
    }
    break;
  case STEP: {
    ShowRegisters();
  READ:
    printf(">");
    scanf("%s", cmd);
    fflush(stdin);
    if (!strcmp(cmd, "checksum")) {
      u16 x = 0;
      for (u16 i = 0x0134; i <= 0x014C; i++) {
        printf("Value %x\n", ram[i]);
        x = x - ram[i] - 1;
      }
      printf("Checsum %d\n", x);
    }
    if (!strcmp(cmd, "se")) {
      scanf("%x %x", &address, &value);
      ram[address] = value;
    }
    if (!strcmp(cmd, "sh")) {
      ShowRegisters();
    }
    if (!strcmp(cmd, "r8")) {
      printf("0x%x\n", ram[registers.PC + 1]);
    }
    if (!strcmp(cmd, "r16")) {
      printf("0x%x 0x%x\n", ram[registers.PC + 1], ram[registers.PC + 2]);
    }
    if (!(strcmp(cmd, "m"))) {
      scanf("%x", &address);
      printf("0x%x\n", ram[address]);
    }
    if (!(strcmp(cmd, "op"))) {
      printf("0x%x 0x%x 0x%x\n", ram[registers.PC], ram[registers.PC + 1],
             ram[registers.PC + 2]);
    }
    if (!(strcmp(cmd, "co"))) {
      scanf("%d", &runCount);
      state = COUNT;
      return;
    }
    if (!(strcmp(cmd, "go"))) {
      scanf("%x", &address);
      state = ADDRESS;
      return;
    }
    if (!(strcmp(cmd, "ne"))) {
      cmd[0] = 0x00;
      return;
    }
    goto READ;
  } break;
  case ADDRESS: {
    if (registers.PC != address) {
      return;
    }
    state = STEP;
    goto READ;
  } break;
  default:
    break;
  }
}

void Log() {
  printf("PC: 0x%02x A: 0x%02x BC: %04x DE: %04x HL: %04x SP: %04x "
         "Z:%d N:%d H:%d "
         "C:%d\n",
         registers.PC, registers.A, registers.BC, registers.DE, registers.HL,
         registers.SP, ZFLAG, NFLAG, HFLAG, CFLAG);
}

void ShowRegisters() {
  printf("*************************\n");
  printf("AF 0x%x 0x%x\n", registers.A, registers.F);
  printf("BC 0x%x 0x%x\n", registers.B, registers.C);
  printf("DE 0x%x 0x%x\n", registers.D, registers.E);
  printf("HL 0x%x 0x%x\n", registers.H, registers.L);
  printf("SP 0x%x\n", registers.SP);
  printf("PC 0x%x\n", registers.PC);
  printf("Z:%d N:%d H:%d C:%d\n", ZFLAG, NFLAG, HFLAG, CFLAG);
  printf("*************************\n");
  printf("Ram: 0x%02x 0x%02x 0x%02x\n", ram[registers.PC],
         ram[registers.PC + 1], ram[registers.PC + 2]);
  printf("*************************\n");
}
