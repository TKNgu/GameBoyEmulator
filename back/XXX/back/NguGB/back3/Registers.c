#include "Registers.h"
#include <stdio.h>
#include <string.h>

Registers registers;

void InitRegisters() { memset((void *)(&registers), 0x00, sizeof(Registers)); }
