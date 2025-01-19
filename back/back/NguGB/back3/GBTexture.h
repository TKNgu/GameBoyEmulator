#ifndef GBTexture_h
#define GBTexture_h

#include <SDL2/SDL.h>

typedef struct {
  unsigned width;
  unsigned height;
  SDL_Texture *texture;
  void *data;
  int pitch;
} GBTexture;

void InitGBTexture(GBTexture *gbTexture);
void LockGBTexture(GBTexture *gbTexture);
void UnlockGBTexture(GBTexture *gbTexture);
void RenderGBTexture(const GBTexture *gbTexture, const SDL_Rect *dst);

#endif
