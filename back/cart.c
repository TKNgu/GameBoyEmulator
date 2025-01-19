#include "gb.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* 16KB ROM banks */
#define GB_ROM_BANK_SIZE (16 * 1024)
/* 8KB RAM banks */
#define GB_RAM_BANK_SIZE (8 * 1024)

/* GB ROMs are at least 32KB (2 banks) */
#define GB_CART_MIN_SIZE (GB_ROM_BANK_SIZE * 2)

/* I think the biggest licensed GB cartridge is 8MB but let's add a margin in
 * case there are homebrews with even bigger carts. */
#define GB_CART_MAX_SIZE (32U * 1024 * 1024)

uint8_t (*gb_cart_rom_readb)(struct gb *gb, uint16_t addr);

uint8_t gb_cart_rom_readb_base(struct gb *gb, uint16_t addr) {
  struct gb_cart *cart = &gb->cart;
  unsigned rom_off = addr;

  switch (cart->model) {
  case GB_CART_SIMPLE:
    /* No mapper */
    break;
  case GB_CART_MBC1:
    if (addr >= GB_ROM_BANK_SIZE) {
      /* Bank 1 can be remapped through this controller */
      unsigned bank = cart->cur_rom_bank;

      if (cart->mbc1_bank_ram) {
        /* When MBC1 is configured to bank RAM it can only address
         * 16 ROM banks */
        bank %= 32;
      } else {
        bank %= 128;
      }

      if (bank == 0) {
        /* Bank 0 can't be mirrored that way, using a bank of 0 is
         * the same thing as using 1 */
        bank = 1;
      }

      bank %= cart->rom_banks;

      rom_off += (bank - 1) * GB_ROM_BANK_SIZE;
    }
    break;
  case GB_CART_MBC2:
  case GB_CART_MBC3:
    if (addr >= GB_ROM_BANK_SIZE) {
      rom_off += (cart->cur_rom_bank - 1) * GB_ROM_BANK_SIZE;
    }
    break;
  case GB_CART_MBC5:
    if (addr >= GB_ROM_BANK_SIZE) {
      /* Bank 0 can be remapped as bank 1 with this controller, so we
       * need to be careful to handle that case correctly */
      unsigned bank = cart->cur_rom_bank % cart->rom_banks;

      rom_off -= GB_ROM_BANK_SIZE;
      rom_off += bank * GB_ROM_BANK_SIZE;
    }
    break;
  default:
    /* Should not be reached */
    die();
  }

  return cart->rom[rom_off];
}

static void gb_cart_get_rom_title(struct gb *gb, char title[17]) {
  struct gb_cart *cart = &gb->cart;
#define TITLE_SIZE 16
  memcpy(title, cart->header->title, TITLE_SIZE);
  title[TITLE_SIZE] = '\0';
  for (size_t index = 0; index < TITLE_SIZE; index++) {
    char c = title[index];
    if (0 == c) {
      break;
    }
    if (!isprint(c)) {
      title[index] = '?';
    }
  }
}

int gb_rom_load(struct gb_cart *cart, const char *rom_path) {
  FILE *file = fopen(rom_path, "rb");
  if (file == NULL) {
    perror("Can't open ROM file");
    return 1;
  }

  size_t file_size;
  if (fseek(file, 0, SEEK_END) == -1 || (file_size = ftell(file)) == -1 ||
      fseek(file, 0, SEEK_SET) == -1) {
    perror("Can't get ROM file length");
    goto ERROR;
  }

  if (file_size == 0) {
    fprintf(stderr, "ROM file is empty!\n");
    goto ERROR;
  }

  if (file_size > GB_CART_MAX_SIZE) {
    fprintf(stderr, "ROM file is too big!\n");
    goto ERROR;
  }

  if (file_size < GB_CART_MIN_SIZE) {
    fprintf(stderr, "ROM file is too small!\n");
    goto ERROR;
  }

  cart->rom_length = file_size;
  cart->rom = malloc(cart->rom_length);
  if (cart->rom == NULL) {
    perror("Can't allocate ROM buffer");
    goto ERROR;
  }

  file_size = fread(cart->rom, sizeof(uint8_t), cart->rom_length, file);
  fclose(file);
  if (file_size < cart->rom_length) {
    fprintf(stderr, "Failed to load ROM file (read %u bytes, expected %u)\n",
            (unsigned)file_size, cart->rom_length);
    free(cart->rom);
    goto ERROR;
  }
  cart->header = (CartHeader *)(cart->rom + 0x0134);
  return 0;

ERROR:
  fclose(file);
  return 1;
}

int gb_cart_init(struct gb_cart *cart) {
  switch (cart->header->cartridgeType) {
  case 0x00:
    cart->model = GB_CART_SIMPLE;
    break;
  case 0x01: /* MBC1, no RAM */
  case 0x02: /* MBC1, with RAM */
  case 0x03: /* MBC1, with RAM and battery backup */
    cart->model = GB_CART_MBC1;
    break;
  case 0x05: /* MBC2 */
  case 0x06: /* MBC2 with battery backup */
    cart->model = GB_CART_MBC2;
    /* MBC2 always has 512 * 4bits of RAM available */
    cart->ram_banks = 1;
    /* Allocate 512 bytes for convenience, but only the low 4 bits should
     * be used */
    cart->ram_length = 512;
    break;
  case 0x0f: /* MBC3, with battery backup and RTC */
  case 0x10: /* MBC3, with RAM, battery backup and RTC */
  case 0x11: /* MBC3, no RAM */
  case 0x12: /* MBC3, with RAM */
  case 0x13: /* MBC3, with RAM and battery backup */
    cart->model = GB_CART_MBC3;
    break;
  case 0x19: /* MBC5, no RAM */
  case 0x1a: /* MBC5, with RAM */
  case 0x1b: /* MBC5, with RAM and battery backup */
    cart->model = GB_CART_MBC5;
    break;
  default:
    fprintf(stderr, "Unsupported cartridge type %x\n",
            cart->header->cartridgeType);
    return 1;
  }
  return 0;
}

int gb_cart_init_rom(struct gb_cart *cart) {
  cart->cur_rom_bank = 1;
  /* Figure out the number of ROM banks for this cartridge */
  switch (cart->header->romSize) {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
    cart->rom_banks = (0b10 << cart->header->romSize);
    break;
  case 0x52:
    cart->rom_banks = 72;
    break;
  case 0x53:
    cart->rom_banks = 80;
    break;
  case 0x54:
    cart->rom_banks = 96;
    break;
  default:
    fprintf(stderr, "Unknown ROM size configuration: %x\n",
            cart->header->romSize);
    return 1;
  }

  if (cart->rom_length < cart->rom_banks * GB_ROM_BANK_SIZE) {
    fprintf(stderr,
            "ROM file is too small to hold the declared"
            " %d ROM banks\n",
            cart->rom_banks);
    return 1;
  }

  return 0;
}

int gen_file_save(const char *rom_path, char **save_path) {
#define SAVE_EXTENSION ".sav"
#define SAVE_EXTENSION_SIZE 5
  const size_t path_len = strlen(rom_path);
  *save_path = malloc(path_len + SAVE_EXTENSION_SIZE);
  if (NULL == *save_path) {
    return 1;
  }
  for (size_t index = path_len - 1; index > 0; index--) {
    if ('.' == rom_path[index]) {
      memcpy(*save_path, rom_path, index);
      memcpy(*save_path + index, SAVE_EXTENSION, SAVE_EXTENSION_SIZE);
      return 0;
    }
  }
  memcpy(*save_path, rom_path, path_len);
  memcpy(*save_path + path_len, SAVE_EXTENSION, SAVE_EXTENSION_SIZE);
  return 0;
}

int gb_cart_init_ram(struct gb_cart *cart, struct gb *gb,
                     const char *rom_path) {
  switch (cart->header->ramSize) {
  case 0: /* No RAM */
    cart->ram_banks = 0;
    cart->ram_length = 0;
    break;
  case 1:
    /* One bank but only 2kB (so really 1/4 of a bank) */
    cart->ram_banks = 1;
    cart->ram_length = GB_RAM_BANK_SIZE / 4;
    break;
  case 2:
    cart->ram_banks = 1;
    cart->ram_length = GB_RAM_BANK_SIZE;
    break;
  case 3:
    cart->ram_banks = 4;
    cart->ram_length = GB_RAM_BANK_SIZE * 4;
    break;
  case 4:
    cart->ram_banks = 16;
    cart->ram_length = GB_RAM_BANK_SIZE * 16;
    break;
  default:
    fprintf(stderr, "Unknown RAM size configuration: %x\n",
            cart->header->ramSize);
    return 1;
  }

  cart->has_rtc = false;
  bool has_battery_backup = false;
  /* Check if cart has a battery for memory backup */
  switch (cart->header->cartridgeType) {
  case 0x0f:
  case 0x10:
    cart->has_rtc = true;
  case 0x03:
  case 0x06:
  case 0x09:
  case 0x13:
  case 0x1b:
  case 0x1e:
  case 0xff:
    has_battery_backup = true;
  }

  /* Allocate RAM buffer */
  if (cart->ram_length > 0) {
    cart->ram = calloc(1, cart->ram_length);
    if (cart->ram == NULL) {
      perror("Can't allocate RAM buffer");
      return true;
    }
  } else if (!cart->has_rtc) {
    /* Memory backup without RAM or RTC doesn't make a lot of sense */
    has_battery_backup = false;
    return 0;
  }
  if (!has_battery_backup) {
    return 0;
  }

  if (gen_file_save(rom_path, &cart->save_file)) {
    return 1;
  }

  /* First we attempt to load the save file if it already exists */
  FILE *f = fopen(cart->save_file, "rb");
  if (NULL == f) {
    if (cart->has_rtc) {
      gb_rtc_init(gb);
    }
    return 0;
  }
  if (cart->ram_length > 0) {
    size_t nread = fread(cart->ram, 1, cart->ram_length, f);
    if (nread != cart->ram_length) {
      fprintf(stderr, "RAM save file is too small!\n");
      free(cart->save_file);
      fclose(f);
      return 1;
    }
  }
  if (cart->has_rtc) {
    gb_rtc_load(gb, f);
  }
  fclose(f);

  printf("Loaded RAM save from '%s'\n", cart->save_file);
  return 0;
}

void gb_cart_load(struct gb *gb, const char *rom_path) {
  struct gb_cart *cart = &gb->cart;
  if (gb_rom_load(cart, rom_path)) {
    goto DIE;
  }

  if (gb_cart_init(cart)) {
    goto RELESER_ROM;
  }

  if (gb_cart_init_rom(cart)) {
    goto RELESER_ROM;
  }

  char rom_title[17];

  cart->ram = NULL;
  // cart->cur_ram_bank = 0;
  cart->ram_write_protected = true;
  cart->mbc1_bank_ram = false;
  cart->dirty_ram = false;

  if (gb_cart_init_ram(cart, gb, rom_path)) {
    goto RELESER_ROM;
  }

  /* See if we have a DMG or GBC game */
  gb->gbc = (cart->header->cgbFlag & 0x80);

  gb_cart_get_rom_title(gb, rom_title);

  printf("Succesfully Loaded %s\n", rom_path);
  printf("Title: '%s'\n", rom_title);
  printf("ROM banks: %u (%uKiB)\n", cart->rom_banks,
         cart->rom_banks * GB_ROM_BANK_SIZE / 1024);
  printf("RAM banks: %u (%uKiB)\n", cart->ram_banks, cart->ram_length / 1024);
  gb_cart_rom_readb = gb_cart_rom_readb_base;
  return;

error:

  if (cart->ram) {
    free(cart->ram);
    cart->ram = NULL;
  }

RELESER_ROM:
  free(cart->rom);

DIE:
  die();
}

static void gb_cart_ram_save(struct gb *gb) {
  struct gb_cart *cart = &gb->cart;
  FILE *f;

  if (cart->save_file == NULL) {
    /* No battery backup, nothing to do */
    return;
  }

  if (!cart->dirty_ram) {
    /* No changes to RAM since last save, nothing to do */
    return;
  }

  f = fopen(cart->save_file, "wb");
  if (f == NULL) {
    fprintf(stderr, "Can't create or open save file '%s': %s", cart->save_file,
            strerror(errno));
    die();
  }

  if (cart->ram_length > 0) {
    /* Dump RAM to file */
    if (fwrite(cart->ram, 1, cart->ram_length, f) < 0) {
      perror("fwrite failed");
      fclose(f);
      die();
    }
  }

  if (cart->has_rtc) {
    gb_rtc_dump(gb, f);
  }

  fflush(f);
  fclose(f);

  printf("Saved RAM\n");
  cart->dirty_ram = false;
}

void gb_cart_unload(struct gb *gb) {
  struct gb_cart *cart = &gb->cart;

  gb_cart_ram_save(gb);

  if (cart->save_file) {
    free(cart->save_file);
  }

  if (cart->rom) {
    free(cart->rom);
    cart->rom = NULL;
  }

  if (cart->ram) {
    free(cart->ram);
    cart->ram = NULL;
  }
}

void gb_cart_sync(struct gb *gb) {
  gb_cart_ram_save(gb);
  gb_sync_next(gb, GB_SYNC_CART, GB_SYNC_NEVER);
}

void gb_cart_rom_writeb(struct gb *gb, uint16_t addr, uint8_t v) {
  struct gb_cart *cart = &gb->cart;

  switch (cart->model) {
  case GB_CART_SIMPLE:
    /* Nothing to be done */
    break;
  case GB_CART_MBC1:
    if (addr < 0x2000) {
      cart->ram_write_protected = ((v & 0xf) != 0xa);
    } else if (addr < 0x4000) {
      /* Set ROM bank, bits [4:0] */
      cart->cur_rom_bank &= ~0x1f;
      cart->cur_rom_bank |= v & 0x1f;
    } else if (addr < 0x6000) {
      /* Set RAM bank *or* ROM bank [6:5] depending on the mode */
      cart->cur_rom_bank &= 0x1f;
      cart->cur_rom_bank |= (v & 3) << 5;

      if (cart->ram_banks > 0) {
        cart->cur_ram_bank = (v & 3) % cart->ram_banks;
      }
    } else {
      /* Change MBC1 banking mode */
      cart->mbc1_bank_ram = v & 1;
    }
    break;
  case GB_CART_MBC2:
    if (addr < 0x2000) {
      cart->ram_write_protected = ((v & 0xf) != 0xa);
    } else if (addr < 0x4000) {
      cart->cur_rom_bank = v & 0xf;
      if (cart->cur_rom_bank == 0) {
        cart->cur_rom_bank = 1;
      }
    }
    break;
  case GB_CART_MBC3:
    if (addr < 0x2000) {
      cart->ram_write_protected = ((v & 0xf) != 0xa);
    } else if (addr < 0x4000) {
      /* Set ROM bank */
      cart->cur_rom_bank = (v & 0x7f) % cart->rom_banks;
      if (cart->cur_rom_bank == 0) {
        cart->cur_rom_bank = 1;
      }
    } else if (addr < 0x6000) {
      /* Set RAM bank (v < 3) *or* RTC access */
      cart->cur_ram_bank = v;
    } else if (addr < 0x8000) {
      if (cart->has_rtc) {
        gb_rtc_latch(gb, v == 1);
      }
    }
    break;
  case GB_CART_MBC5:
    if (addr < 0x2000) {
      cart->ram_write_protected = ((v & 0xf) != 0xa);
    } else if (addr < 0x3000) {
      /* Set ROM bank, low 8 bits */
      cart->cur_rom_bank &= 0x100;
      cart->cur_rom_bank |= v;
    } else if (addr < 0x4000) {
      /* Set ROM bank, MSB */
      cart->cur_rom_bank &= 0xff;
      cart->cur_rom_bank |= (v & 1) << 8;
    } else if (addr < 0x6000) {
      /* Set RAM bank */
      if (cart->ram_banks > 0) {
        cart->cur_ram_bank = (v & 0xf) % cart->ram_banks;
      }
    }
    break;
  default:
    /* Should not be reached */
    die();
  }
}

unsigned gb_cart_mbc1_ram_off(struct gb *gb, uint16_t addr) {
  struct gb_cart *cart = &gb->cart;
  unsigned bank;

  if (cart->ram_banks == 1) {
    /* Cartridges which only have one RAM bank can have only a
     * partial 2KB RAM chip that's mirrored 4 times. */
    return addr % cart->ram_length;
  }

  bank = cart->cur_ram_bank;

  if (cart->mbc1_bank_ram) {
    bank %= 4;
  } else {
    /* In this mode we only support one bank */
    bank = 0;
  }

  return bank * GB_RAM_BANK_SIZE + addr;
}

uint8_t gb_cart_ram_readb(struct gb *gb, uint16_t addr) {
  struct gb_cart *cart = &gb->cart;
  unsigned ram_off;

  switch (cart->model) {
  case GB_CART_SIMPLE:
    /* No RAM */
    return 0xff;
  case GB_CART_MBC1:
    if (cart->ram_banks == 0) {
      /* No RAM */
      return 0xff;
    }

    ram_off = gb_cart_mbc1_ram_off(gb, addr);
    break;
  case GB_CART_MBC2:
    ram_off = addr % 512;
    break;
  case GB_CART_MBC3:
    if (cart->cur_ram_bank <= 3) {
      /* RAM access */
      unsigned b;

      if (cart->ram_banks == 0) {
        /* No RAM */
        return 0xff;
      }

      b = cart->cur_ram_bank % cart->ram_banks;

      ram_off = b * GB_RAM_BANK_SIZE + addr;
    } else {
      /* RTC access. Only accessible when the RAM is not write
       * protected (even for reads) */
      if (cart->has_rtc && !cart->ram_write_protected) {
        return gb_rtc_read(gb, cart->cur_ram_bank);
      } else {
        return 0xff;
      }
    }

    break;
  case GB_CART_MBC5:
    if (cart->ram_banks == 0) {
      /* No RAM */
      return 0xff;
    }

    ram_off = cart->cur_ram_bank * GB_RAM_BANK_SIZE + addr;
    break;
  default:
    /* Should not be reached */
    die();
    return 0xff;
  }

  return cart->ram[ram_off];
}

void gb_cart_ram_writeb(struct gb *gb, uint16_t addr, uint8_t v) {
  struct gb_cart *cart = &gb->cart;
  unsigned ram_off;

  if (cart->ram_write_protected) {
    return;
  }

  switch (cart->model) {
  case GB_CART_SIMPLE:
    /* No RAM */
    return;
  case GB_CART_MBC1:
    if (cart->ram_banks == 0) {
      /* No RAM */
      return;
    }

    ram_off = gb_cart_mbc1_ram_off(gb, addr);
    break;
  case GB_CART_MBC2:
    ram_off = addr % 512;
    /* MBC2 only has 4 bits per address, so the high nibble is unusable */
    v |= 0xf0;
    break;
  case GB_CART_MBC3:
    if (cart->cur_ram_bank <= 3) {
      /* RAM access */
      unsigned b;

      if (cart->ram_banks == 0) {
        /* No RAM */
        return;
      }

      b = cart->cur_ram_bank % cart->ram_banks;

      ram_off = b * GB_RAM_BANK_SIZE + addr;
    } else {
      /* RTC access. Only accessible when the RAM is not write
       * protected (even for reads) */
      if (cart->has_rtc) {
        gb_rtc_write(gb, cart->cur_ram_bank, v);
      }
      goto write_done;
    }

    break;
  case GB_CART_MBC5:
    if (cart->ram_banks == 0) {
      /* No RAM */
      return;
    }

    ram_off = cart->cur_ram_bank * GB_RAM_BANK_SIZE + addr;
    break;
  default:
    /* Should not be reached */
    die();
  }

  cart->ram[ram_off] = v;

write_done:
  if (cart->save_file) {
    cart->dirty_ram = true;
    /* Schedule a save in a short while if we don't have changes by then
     */
    gb_sync_next(gb, GB_SYNC_CART, 3 * GB_CPU_FREQ_HZ);
  }
}
