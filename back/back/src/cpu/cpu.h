#ifndef _GB_CPU_H_
#define _GB_CPU_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

struct gb;

struct gb_cpu {
  /* Interrupt Master Enable (IME) flag */
  bool irq_enable;

  /* Value of IRQ enable on the next cycle (for delayed EI) */
  bool irq_enable_next;

  /* True if the CPU is currently halted */
  bool halted;

  /* Program Counter */
  uint16_t pc;

  /* Stack Pointer */
  union {
    uint16_t sp;
    struct {
      uint8_t p;
      uint8_t s;
    };
  };

  /* register */
  union {
    uint16_t af;
    struct {
      uint8_t f;
      uint8_t a;
    };
  };
  union {
    uint16_t bc;
    struct {
      uint8_t c;
      uint8_t b;
    };
  };
  union {
    uint16_t de;
    struct {
      uint8_t e;
      uint8_t d;
    };
  };
  union {
    uint16_t hl;
    struct {
      uint8_t l;
      uint8_t h;
    };
  };

  /* Zero flag */
  bool f_z;
  /* Substract flag */
  bool f_n;
  /* Half-Carry flag */
  bool f_h;
  /* Carry flag */
  bool f_c;

  struct gb *gb;
};

typedef struct gb_cpu CPU;

// void init_cpu(struct gb *gb);
// void release_cpu(struct gb *gb);

int32_t gb_cpu_run_cycles(struct gb *gb, int32_t cycles);

uint8_t cpu_readb(CPU *cpu, uint16_t address);
void cpu_writeb(CPU *cpu, uint16_t address, uint8_t value);

#endif /* _GB_CPU_H_ */
