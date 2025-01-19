#include <stdio.h>

#include "Bus.h"
#include "CPU.h"
#include "Cart.h"
#include "Instruction.h"

int main(int argc, char *argv[]) {
  printf("Hello NguGB\n");

  // Init Instruction
  InstructionInit();

  // Init cart
  Cart cart;
  if (CartInit(&cart, "/home/ngocpt/Work/Lab/data/NguGB/Tetris.gb")) {
    printf("Error load cart\n");
    return 1;
  }
  printf("Cartridge Loaded:\n");
  CartShow(cart);

  // Init bus
  Bus bus = {.cart = cart};

  // Init cpu
  CPU cpu;
  cpu.bus = bus;
  CPUInit(&cpu);

  // Loop running
  int is_running = 1;
  while (is_running) {
    int error = 0;
    if ((error = CPUStep(&cpu))) {
      if (error != CPU_STOP && error != CPU_HALT) {
        printf("Error run cpu step %d\n", error);
        is_running = 0;
      }
    }
    CPUShow(cpu);
  }

  CartRelease(cart);
  return 0;
}
