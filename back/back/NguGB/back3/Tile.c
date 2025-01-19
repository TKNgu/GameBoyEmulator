#include "Tile.h"
#include "GBTexture.h"
#include <stdint.h>

void InitTile(Tile *tile) { InitGBTexture(&tile->gbTexture); }

void UpdateTile(Tile *tile) {
  LockGBTexture(&tile->gbTexture);
  {
    GBTexture gbTexture = tile->gbTexture;
    for (size_t index = 0; index < gbTexture.height; index++) {
      size_t offset = index << 1;
      u8 first = tile->address[offset];
      u8 second = tile->address[offset + 1];
      uint32_t *data = (uint32_t *)(gbTexture.data + index * gbTexture.pitch);
      for (unsigned subIndex = 0; subIndex < 8; subIndex++) {
        u8 mask = 0x1 << subIndex;
        switch (((first & mask) + (second & mask)) >> subIndex) {
        case 0:
          data[7 - subIndex] = 0x9bbc0f;
          break;
        case 1:
          data[7 - subIndex] = 0x8bac0f;
          break;
        case 2:
          data[7 - subIndex] = 0x306230;
          break;
        case 3:
          data[7 - subIndex] = 0x0f380f;
          break;
        }
      }
    }
  }
  UnlockGBTexture(&tile->gbTexture);
}

void RenderTile(const Tile *tile, const SDL_Rect *dst) {
  RenderGBTexture(&tile->gbTexture, dst);
}
