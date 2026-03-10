#ifndef DebugTile_h
#define DebugTile_h

#include "Common.h"
#include "Tile.h"

typedef struct {
  u8 *address;
  size_t numberTile;
  Tile *tiles;
} DebugTile;

void InitDebugTile(DebugTile *debugTile);
void ClearDebugTile(DebugTile *debugTile);
void UpdateDebugTile(DebugTile *debugTile);
void RenderDebugTile(DebugTile *debugTile, SDL_Rect dst);

#endif
