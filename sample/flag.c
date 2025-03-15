#include <stdint.h>
#include <stdio.h>

struct CPU {
  union {
    uint16_t af;
    struct {
      union {
        uint8_t f;
        struct {
          uint8_t flag_padding : 4;
          uint8_t f_c : 1;
          uint8_t f_h : 1;
          uint8_t f_n : 1;
          uint8_t f_z : 1;
        };
      };
      uint8_t a;
    };
  };
};

void cpu_printf(const struct CPU *cpu) {
  printf("CPU:\n");
  printf("  AF: 0x%02X\n", cpu->af);
  printf("    A: 0x%02X\n", cpu->a);
  printf("    F: 0b%08b\n", cpu->f);
  printf("      Z: %s\n", cpu->f_z ? "On" : "Off");
  printf("      N: %s\n", cpu->f_n ? "On" : "Off");
  printf("      H: %s\n", cpu->f_h ? "On" : "Off");
  printf("      C: %s\n", cpu->f_c ? "On" : "Off");
}

int main(int argc, char *argv[]) {
  printf("Hello flag\n");
  struct CPU cpu;
  cpu.af = 0x46c7;
  cpu_printf(&cpu);
  cpu.f_c = 0x01;
  cpu.f_z = 0x00;
  cpu.f_h = 1 == 2;
  cpu_printf(&cpu);
  cpu.f_z = 0x11;
  cpu_printf(&cpu);
  cpu.f = 0b10110000;
  cpu_printf(&cpu);
  printf("Sizeof %lu\n", sizeof(cpu));
  return 0;
}
