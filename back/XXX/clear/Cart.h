#ifndef Cart_h
#define Cart_h

#include "MemoryBank.h"

struct Cart {
    struct MemoryBank romBank;
    struct MemoryBank ramBank;
};

int InitCart(struct Cart *cart, const char *fileName);
void ReleaseCart(struct Cart *cart);

#endif
