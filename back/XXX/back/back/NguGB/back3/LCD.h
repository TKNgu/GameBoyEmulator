#ifndef LCD_h
#define LCD_h

#include "Common.h"
#include "GBTexture.h"

typedef struct {
  u8 LCDC;
  u8 STAT;
  u8 LYC;
  u8 LY;
} LCDRegister;

typedef struct {
  LCDRegister *registers;
  GBTexture gbTexture;
} LCD;

extern LCD lcd;

void InitLCD();
void LCDTick();
void LCDRender();

#endif
