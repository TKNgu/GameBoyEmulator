#ifndef Tile_h
#define Tile_h

#include "Common.h"
#include "GBTexture.h"

typedef struct {
  u8 *address;
  GBTexture gbTexture;
} Tile;

void InitTile(Tile *tile);
void UpdateTile(Tile *tile);
void RenderTile(const Tile *tile, const SDL_Rect *dst);

#endif
