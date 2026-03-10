#include "GBTexture.h"
#include "Graphic.h"

void InitGBTexture(GBTexture *gbTexture) {
  gbTexture->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         gbTexture->width, gbTexture->height);
}

void LockGBTexture(GBTexture *gbTexture) {
  SDL_LockTexture(gbTexture->texture, NULL, &(gbTexture->data),
                  &(gbTexture->pitch));
}

void UnlockGBTexture(GBTexture *gbTexture) {
  SDL_UnlockTexture(gbTexture->texture);
}

void RenderGBTexture(const GBTexture *gbTexture, const SDL_Rect *dst) {
  SDL_RenderCopy(renderer, gbTexture->texture, NULL, dst);
}
