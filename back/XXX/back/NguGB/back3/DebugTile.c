#include "DebugTile.h"
#include "Tile.h"
#include <sys/types.h>

void InitDebugTile(DebugTile *debugTile) {
  debugTile->tiles = (Tile *)(malloc(sizeof(Tile) * debugTile->numberTile));
  u8 *offset = debugTile->address;
  for (size_t index = 0; index < debugTile->numberTile; index++) {
    debugTile->tiles[index] = (Tile){.address = offset + (index << 4),
                                     .gbTexture = {
                                         .width = 8,
                                         .height = 8,
                                     }};
    InitTile(debugTile->tiles + index);
  }
}

void ClearDebugTile(DebugTile *debugTile) { free(debugTile->tiles); }

void UpdateDebugTile(DebugTile *debugTile) {
  for (size_t index = 0; index < debugTile->numberTile; index++) {
    UpdateTile(debugTile->tiles + index);
  }
}

void RenderDebugTile(DebugTile *debugTile, SDL_Rect dst) {
  const unsigned countWidth = 24;
  const unsigned countHeight = debugTile->numberTile / countWidth +
                               (debugTile->numberTile % countWidth ? 1 : 0);
  const unsigned sizeWidth = dst.w / countWidth;
  const unsigned sizeHeight = dst.h / countHeight;

  unsigned numberTile = debugTile->numberTile;
  Tile *tiles = debugTile->tiles;
  for (unsigned indexY = 0; indexY < countHeight; indexY++) {
    unsigned offsetY = indexY * countWidth;
    for (unsigned indexX = 0; indexX < countWidth; indexX++) {
      unsigned index = offsetY + indexX;
      if (index >= numberTile) {
        return;
      }
      SDL_Rect tileDst = {
          .x = dst.x + indexX * sizeWidth,
          .y = dst.y + indexY * sizeHeight,
          .w = sizeWidth,
          .h = sizeHeight,
      };
      RenderTile(tiles + index, &tileDst);
    }
  }
}
