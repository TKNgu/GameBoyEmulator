#include "CPU.h"
#include "Debug.h"
#include "DebugTile.h"
#include "GBTexture.h"
#include "Graphic.h"
#include "LCD.h"
#include "Memory.h"
#include "Registers.h"
#include "Tile.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
  // if (SDL_Init(SDL_INIT_VIDEO) < 0) {
  //   printf("Error init video\n");
  //   return 1;
  // }
  // SDL_Window *window =
  //     SDL_CreateWindow("Hello C big project", SDL_WINDOWPOS_CENTERED,
  //                      SDL_WINDOWPOS_CENTERED, 800, 640,
  // SDL_WINDOW_SHOWN);
  // if (!window) {
  //   printf("Error crate window");
  //   goto QUIT;
  // }
  // renderer = SDL_CreateRenderer(window, -1, 0);
  // if (!renderer) {
  //   printf("Error create renderer");
  //   goto DESTROY_WINDOW;
  // }

  GenOpcode();
  InitRegisters();
  InitMemory();
  int errorCode = LoadBootRom("data/DMG_ROM.bin");
  if (errorCode) {
    printf("Error load  boot rom %x\n", errorCode);
    // goto DESTROY_WINDOW;
  }
  errorCode = LoadRom("data/01-special.gb");
  if (errorCode) {
    printf("Error load  rom %x\n", errorCode);
    return 1;
    // goto DESTROY_WINDOW;
  }

  int isRunning = 1;
  int isUnmapBootROM = 1;

  isUnmapBootROM = 0;
  UnmapBootROM();
  registers.PC = 0x0100;

  while (isRunning) {
    // if (isUnmapBootROM && ram[0xFF50]) {
    //   UnmapBootROM();
    //   isUnmapBootROM = 0;
    // }
    Debug();
    CPUTick();
  }

  //   const float TIME_PER_FRAME = 1000.f / 60.f;
  //   int isRunning = 1;
  //   int isUnmapBootROM = 1;
  //   InitLCD();
  //
  //   DebugTile debugTile = {
  //       .address = ram + 0x8000,
  //       .numberTile = 384,
  //   };
  //   InitDebugTile(&debugTile);
  //
  //   while (isRunning) {
  //     uint32_t timePointNow = SDL_GetTicks();
  //     SDL_Event event;
  //     while (SDL_PollEvent(&event)) {
  //       if (event.type == SDL_QUIT) {
  //         isRunning = 0;
  //         break;
  //       }
  //     }
  //
  //     if (isUnmapBootROM && ram[0xFF50]) {
  //       UnmapBootROM();
  //       isUnmapBootROM = 0;
  //     }
  //     CPUTick();
  //     LCDTick();
  //     UpdateDebugTile(&debugTile);
  //
  //     SDL_RenderClear(renderer);
  //     SDL_Rect dstDebugTile = {
  //         .x = 320,
  //         .y = 0,
  //         .w = 480,
  //         .h = 640,
  //     };
  //     RenderDebugTile(&debugTile, dstDebugTile);
  //     LCDRender();
  //     SDL_RenderPresent(renderer);
  //     float delayTime = TIME_PER_FRAME - SDL_GetTicks() + timePointNow;
  //     if (delayTime > 0) {
  //       SDL_Delay((Uint32)(delayTime));
  //     }
  //   }
  //
  //   ClearDebugTile(&debugTile);
  // DESTROY_RENDERER:
  //   SDL_DestroyRenderer(renderer);
  // DESTROY_WINDOW:
  //   SDL_DestroyWindow(window);
  // QUIT:
  //   SDL_Quit();
  return 0;
}
