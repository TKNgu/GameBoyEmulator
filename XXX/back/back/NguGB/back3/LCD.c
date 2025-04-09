#include "LCD.h"
#include "GBTexture.h"
#include "Memory.h"
#include "Registers.h"

#define LCD_ON (lcd.registers->LCDC & 0b10000000)
#define WINDOW_TILE_MAP ((lcd.registers & 0b01000000) ? 0x9800 : 0x9c00)
#define WINDOW_ON (lcd.registers->LCDC & 0b00100000)
#define BGW_DATA (lcd.registers->LCDC & 0b00010000)
#define BG_TILE_MAP ((lcd.registers->LCDC & 0b00001000) ? 0x8000 : 0x8800)
#define OBJ_SIZE ((lcd.registers->LCDC & 0b00000100) ? 0x9c00 : 0x9800)
#define OBJ_ON (lcd.registers->LCDC & 0b00000010)
#define BGW_ON (lcd.registers->LCD & 0b00000001)

LCD lcd;

void InitLCD() {
  lcd = (LCD){.registers = (LCDRegister *)(ram + 0xff40),
              .gbTexture = {
                  .width = 160,
                  .height = 144,
              }};
  InitGBTexture(&lcd.gbTexture);
}
void LCDTick() {
  if (!LCD_ON) {
    printf("LCD OFF\n");
    return;
  }
  LockGBTexture(&lcd.gbTexture);
  { printf(""); }
  UnlockGBTexture(&lcd.gbTexture);
}

void LCDRender() {
  SDL_Rect dst = {
      .x = 0,
      .y = 0,
      .w = 320,
      .h = 288,
  };
  RenderGBTexture(&lcd.gbTexture, &dst);
};
