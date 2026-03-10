#include <stdio.h>

#include "Cart.h"

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("No file rom\n");
    return 1;
  }
  struct Cart cart;
  InitCart(&cart, "");
  if (InitCart(&cart, argv[1])) {
    printf("Error open file rom\n");
    return 1;
  }
  ReleaseCart(&cart);
  return 0;
}
