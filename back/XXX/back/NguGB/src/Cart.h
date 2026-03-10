#ifndef Cart_h
#define Cart_h

#include <stddef.h>
#include <stdint.h>

#include "Common.h"

typedef struct {
  u8 entry[4];
  u8 logo[0x30];

  char title[16];
  u16 new_lic_code;
  u8 sgb_flag;
  u8 type;
  u8 rom_size;
  u8 ram_size;
  u8 dest_code;
  u8 lic_code;
  u8 version;
  u8 checksum;
  u16 global_checksum;
} CartHeader;

typedef struct {
  size_t size;
  u8 *data;
  CartHeader *header;
} Cart;

int CartInit(Cart *cart, char *file_path);
void CartRelease(Cart cart);
void CartShow(Cart cart);

#endif
