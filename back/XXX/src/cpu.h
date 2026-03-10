#ifndef _GB_CPU_H_
#define _GB_CPU_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct gb Memory;

typedef struct gb_cpu {
  union {
    uint16_t af;
    struct {
      union {
        uint8_t f;
        struct {
          uint8_t : 4;
          uint8_t fc : 1;
          uint8_t fh : 1;
          uint8_t fn : 1;
          uint8_t fz : 1;
        };
      };
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
  union {
    uint16_t sp;
    struct {
      uint8_t p;
      uint8_t s;
    };
  };
  uint16_t pc;

  /* Zero flag */
  bool f_z;
  /* Substract flag */
  bool f_n;
  /* Half-Carry flag */
  bool f_h;
  /* Carry flag */
  bool f_c;

  /* Interrupt Master Enable (IME) flag */
  bool irq_enable;
  /* Value of IRQ enable on the next cycle (for delayed EI) */
  bool irq_enable_next;

  /* True if the CPU is currently halted */
  bool halted;

  Memory *memory;
} CPU;

void gb_cpu_init();
void gb_cpu_reset(struct gb *gb);
int32_t gb_cpu_run_cycles(struct gb *gb, int32_t cycles);

typedef void (*CPUInstruction)(CPU *cpu);

#endif /* _GB_CPU_H_ */
