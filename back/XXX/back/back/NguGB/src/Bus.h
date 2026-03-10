#ifndef Bus_h
#define Bus_h

#include "Cart.h"
#include "Common.h"

#define BUS_ERROR_NOT_SUPPORT 01
#define BUS_ERROR_OUT_OF_CART 02

typedef struct {
  Cart cart;
} Bus;

int BusRead(Bus bus, u16 address, u8 *value);
int BusWrite(Bus bus, u16 address, u8 value);

#endif
