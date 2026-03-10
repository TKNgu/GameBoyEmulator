#include "Cart.h"
#include "MemoryBank.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ROM_BANK_SIZE 16384    // 16 * 1024 16k
#define RAM_BANK_SIZE 8192     // 8 * 1024 8k
#define CART_MIN_SIZE 32768    // 2 * ROM_BANK_SIZE
#define CART_MAX_SIZE 65994752 // some homebrews carts

typedef uint8_t RomBank[ROM_BANK_SIZE];
typedef uint8_t RamBank[RAM_BANK_SIZE];

struct CartHeader {
  uint8_t padding[0x0100];
  uint8_t entryPoint[0x04];
  uint8_t logo[0x30];
  union {
    uint8_t fullTitle[0x10];
    struct {
      union {
        uint8_t title[0x00f];
        struct {
          uint8_t shortTitle[0x0b];
          uint8_t manufacurerCode[0x04];
        };
      };
      uint8_t cgbFlag;
    };
  };
  uint16_t newLicenseeCode;
  uint8_t sgbFlag;
  uint8_t cartrigeType;
  uint8_t romSize;
  uint8_t ramSize;
  uint8_t destinationCode;
  uint8_t oldLicenseeCode;
  uint8_t maskROMVersionNumber;
  uint8_t headerChecksum;
  uint16_t globalChecksum;
};

void CartHeaderPrint(const struct CartHeader *cartHeader);

int InitCart(struct Cart *cart, const char *fileName) {
  FILE *fileRom = fopen(fileName, "rb");
  if (fileRom == NULL) {
    return 1;
  }
  if (fseek(fileRom, 0UL, SEEK_END)) {
    goto CLOSE_FILE;
  }
  size_t romSize = ftell(fileRom);
  if (romSize < CART_MIN_SIZE || romSize > CART_MAX_SIZE) {
    goto CLOSE_FILE;
  }
  if (fseek(fileRom, 0UL, SEEK_SET)) {
    goto CLOSE_FILE;
  }
  if (InitMemoryBank(&cart->romBank, ROM_BANK_SIZE, 1)) {
    goto CLOSE_FILE;
  }
  size_t size = fread(cart->romBank.memoryBank[0], ROM_BANK_SIZE, 1, fileRom);
  if (size != 1) {
    printf("Error read header rom\n");
    goto RELEASE_ROM_BANK;
  }
  struct CartHeader *cartHeader =
      (struct CartHeader *)(cart->romBank.memoryBank[0]);
  CartHeaderPrint(cartHeader);

  switch (cartHeader->cartrigeType) {
  default:
  case 0x00: {
    MemoryBankRealloc(&cart->romBank, ROM_BANK_SIZE, 2);
    InitMemoryBank(&cart->romBank, RAM_BANK_SIZE, 1);
    break;
  }
    goto RELEASE_ROM_BANK;
  }

  fclose(fileRom);
  return 0;

RELEASE_ROM_BANK:
  ReleaseMemoryBank(&cart->romBank);
CLOSE_FILE:
  fclose(fileRom);
  return 1;
}

void ReleaseCart(struct Cart *cart) {
  ReleaseMemoryBank(&cart->romBank);
  ReleaseMemoryBank(&cart->ramBank);
}

const char *GetNameLicensee(uint16_t newLicenseeCode) {
  switch (newLicenseeCode) {
  default:
    return "Not found";
  }
}

const char *GetNameOldLicesee(uint8_t oldLicenseeCode) {
  switch (oldLicenseeCode) {
  default:
    return "Not found";
  }
}

void CartHeaderPrint(const struct CartHeader *cartHeader) {
  printf("Rom header\n");
  printf("Logo");
  for (size_t index = 0; index < sizeof(cartHeader->logo); index++) {
    if (!(index % 16)) {
      printf("\n");
    }
    printf("0x%02x ", cartHeader->logo[index]);
  }
  printf("\n");

  printf("Title\n");
  for (size_t index = 0; index < sizeof(cartHeader->title); index++) {
    uint8_t tmp = cartHeader->title[index];
    printf("%c", (isprint(tmp) ? tmp : '?'));
  }
  printf("\n");

  printf("Manufacturer Code\n");
  for (size_t index = 0; index < sizeof(cartHeader->manufacurerCode); index++) {
    printf("0x%02x", cartHeader->manufacurerCode[index]);
  }
  printf("\n");

  printf("CGB flags 0x%02x\n", cartHeader->cgbFlag);
  printf("New licensee code %s\n",
         GetNameLicensee(cartHeader->newLicenseeCode));
  printf("SGB flag 0x%02x\n", cartHeader->sgbFlag);
  printf("Cart type 0x%02x\n", cartHeader->cartrigeType);
  printf("Rom size 0x%02x\n", cartHeader->romSize);
  printf("Ram size 0x%02x\n", cartHeader->ramSize);
  printf("Destination Code 0x%02x\n", cartHeader->destinationCode);
  printf("Old licensee code %s\n",
         GetNameOldLicesee(cartHeader->oldLicenseeCode));
  printf("Mask rom 0x%02x\n", cartHeader->maskROMVersionNumber);
  printf("Header check sum 0x%02x\n", cartHeader->headerChecksum);
  printf("Global checksum 0x%04x\n", cartHeader->globalChecksum);
}
