#include "Cart.h"
#include <stdio.h>
#include <stdlib.h>

int CartInit(Cart *cart, char *file_path) {
  FILE *file = fopen(file_path, "rb");
  if (!file) {
    return 1;
  }
  fseek(file, 0, SEEK_END);
  cart->size = ftell(file);
  rewind(file);
  cart->data = (u8 *)malloc(sizeof(u8) * cart->size);
  if (!cart->data) {
    fclose(file);
    return 1;
  }
  fread(cart->data, cart->size, sizeof(u8), file);
  fclose(file);
  cart->header = (CartHeader *)(cart->data + 0x100);
  return 0;
}

void CartRelease(Cart cart) { free(cart.data); }

const char *CartLicName(u8 lic_code) {
  static const char *LIC_CODE[0xA5] = {[0x00] = "None",
                                       [0x01] = "Nintendo R&D1",
                                       [0x08] = "Capcom",
                                       [0x13] = "Electronic Arts",
                                       [0x18] = "Hudson Soft",
                                       [0x19] = "b-ai",
                                       [0x20] = "kss",
                                       [0x22] = "pow",
                                       [0x24] = "PCM Complete",
                                       [0x25] = "san-x",
                                       [0x28] = "Kemco Japan",
                                       [0x29] = "seta",
                                       [0x30] = "Viacom",
                                       [0x31] = "Nintendo",
                                       [0x32] = "Bandai",
                                       [0x33] = "Ocean/Acclaim",
                                       [0x34] = "Konami",
                                       [0x35] = "Hector",
                                       [0x37] = "Taito",
                                       [0x38] = "Hudson",
                                       [0x39] = "Banpresto",
                                       [0x41] = "Ubi Soft",
                                       [0x42] = "Atlus",
                                       [0x44] = "Malibu",
                                       [0x46] = "angel",
                                       [0x47] = "Bullet-Proof",
                                       [0x49] = "irem",
                                       [0x50] = "Absolute",
                                       [0x51] = "Acclaim",
                                       [0x52] = "Activision",
                                       [0x53] = "American sammy",
                                       [0x54] = "Konami",
                                       [0x55] = "Hi tech entertainment",
                                       [0x56] = "LJN",
                                       [0x57] = "Matchbox",
                                       [0x58] = "Mattel",
                                       [0x59] = "Milton Bradley",
                                       [0x60] = "Titus",
                                       [0x61] = "Virgin",
                                       [0x64] = "LucasArts",
                                       [0x67] = "Ocean",
                                       [0x69] = "Electronic Arts",
                                       [0x70] = "Infogrames",
                                       [0x71] = "Interplay",
                                       [0x72] = "Broderbund",
                                       [0x73] = "sculptured",
                                       [0x75] = "sci",
                                       [0x78] = "THQ",
                                       [0x79] = "Accolade",
                                       [0x80] = "misawa",
                                       [0x83] = "lozc",
                                       [0x86] = "Tokuma Shoten Intermedia",
                                       [0x87] = "Tsukuda Original",
                                       [0x91] = "Chunsoft",
                                       [0x92] = "Video system",
                                       [0x93] = "Ocean/Acclaim",
                                       [0x95] = "Varie",
                                       [0x96] = "Yonezawa/s’pal",
                                       [0x97] = "Kaneko",
                                       [0x99] = "Pack in soft",
                                       [0xA4] = "Konami (Yu-Gi-Oh!)"};
  if (lic_code <= 0xA4) {
    return LIC_CODE[lic_code];
  }
  return "UNKNOWN";
}

const char *CartTypeName(u8 type) {
  static const char *ROM_TYPES[] = {
      "ROM ONLY",
      "MBC1",
      "MBC1+RAM",
      "MBC1+RAM+BATTERY",
      "0x04 ???",
      "MBC2",
      "MBC2+BATTERY",
      "0x07 ???",
      "ROM+RAM 1",
      "ROM+RAM+BATTERY 1",
      "0x0A ???",
      "MMM01",
      "MMM01+RAM",
      "MMM01+RAM+BATTERY",
      "0x0E ???",
      "MBC3+TIMER+BATTERY",
      "MBC3+TIMER+RAM+BATTERY 2",
      "MBC3",
      "MBC3+RAM 2",
      "MBC3+RAM+BATTERY 2",
      "0x14 ???",
      "0x15 ???",
      "0x16 ???",
      "0x17 ???",
      "0x18 ???",
      "MBC5",
      "MBC5+RAM",
      "MBC5+RAM+BATTERY",
      "MBC5+RUMBLE",
      "MBC5+RUMBLE+RAM",
      "MBC5+RUMBLE+RAM+BATTERY",
      "0x1F ???",
      "MBC6",
      "0x21 ???",
      "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
  };
  if (type <= 0x22) {
    return ROM_TYPES[type];
  }
  return "UNKNOWN";
}

void CartShow(Cart cart) {
  cart.header->title[15] = 0;
  u8 check_sum = 0;
  for (u16 index = 0x0134; index <= 0x014C; index++) {
    check_sum = check_sum - cart.data[index] - 1;
  }
  printf("\t Checksum : %2.2X (%s)\n", cart.header->checksum,
         (check_sum & 0xFF) ? "PASSED" : "FAILED");
  printf("\t Title    : %s\n", cart.header->title);
  printf("\t Type     : %2.2X (%s)\n", cart.header->type,
         CartTypeName(cart.header->type));
  printf("\t ROM Size : %d KB\n", 32 << cart.header->rom_size);
  printf("\t RAM Size : %2.2X\n", cart.header->ram_size);
  printf("\t LIC Code : %2.2X (%s)\n", cart.header->lic_code,
         CartLicName(cart.header->lic_code));
  printf("\t ROM Vers : %2.2X\n", cart.header->version);
}
