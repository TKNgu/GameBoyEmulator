#include <stdio.h>
#include <stdlib.h>

#include "GB.h"

int main(int argc, char *argv[]) {
    GB *gb = (GB *)malloc(sizeof(GB));
    InitGB(gb);
    while (gb->isRunning) {
        GBStep(gb);
    }
    free(gb);
    return 0;
}
