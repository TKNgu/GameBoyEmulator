#ifndef Registers_h
#define Registers_h

#include "Common.h"

typedef struct {
  union {
    u16 BC;
    struct {
      u8 C;
      u8 B;
    };
  };
  union {
    u16 DE;
    struct {
      u8 E;
      u8 D;
    };
  };
  union {
    u16 HL;
    struct {
      u8 L;
      u8 H;
    };
  };
  union {
    u16 SP;
    struct {
      u8 P;
      u8 S;
    };
  };
  union {
    u16 AF;
    struct {
      u8 F;
      u8 A;
    };
  };
  u16 PC;
} Registers;

extern Registers registers;

#define ZFLAG ((registers.F & 0b10000000) >> 7)
#define NFLAG ((registers.F & 0b01000000) >> 6)
#define HFLAG ((registers.F & 0b00100000) >> 5)
#define CFLAG ((registers.F & 0b00010000) >> 4)

#define ZFLAGOF (registers.F &= 0b01110000)
#define ZFLAGON (registers.F |= 0b10000000)
#define NFLAGOF (registers.F &= 0b10110000)
#define NFLAGON (registers.F |= 0b01000000)
#define HFLAGOF (registers.F &= 0b11010000)
#define HFLAGON (registers.F |= 0b00100000)
#define CFLAGOF (registers.F &= 0b11100000)
#define CFLAGON (registers.F |= 0b00010000)

void InitRegisters();

#endif
