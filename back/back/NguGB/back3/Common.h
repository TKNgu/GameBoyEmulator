#ifndef Common_h
#define Common_h

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// Code
#define EOPEN_FILE_BOOT_ROM 0x0001
#define EFILE_BOOT_ROM_SIZE 0x0002
#define EREAD_BOOT_ROM 0x0003

#define EOPEN_FILE_ROM 0x1001
#define EREAD_ROM 0x1002

#define SDL_INIT 0x2001
#define SDL_CREATE_WINDOW 0x2002
#define SDL_CREATE_RENDERER 0x2003

#endif
