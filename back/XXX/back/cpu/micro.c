#include "micro.h"
#include <stdint.h>

uint8_t cpu_inc(CPU *cpu, uint8_t v) {
  uint8_t r = (v + 1) & 0xff;

  cpu->f_z = (r == 0);
  cpu->f_n = false;
  /* We'll have a half-carry if the low nibble is 0xf */
  cpu->f_h = ((v & 0xf) == 0xf);
  /* Carry is not modified by this instruction */

  return r;
}

uint8_t cpu_dec(CPU *cpu, uint8_t v) {
  uint8_t r = (v - 1) & 0xff;

  cpu->f_z = (r == 0);
  cpu->f_n = true;
  /* We'll have a half-carry if the low nibble is 0 */
  cpu->f_h = ((v & 0xf) == 0);
  /* Carry is not modified by this instruction */

  return r;
}

uint16_t cpu_addw_set_flags(CPU *cpu, uint16_t a, uint16_t b) {
  /* Widen to 32bits to get the carry */
  uint32_t wa = a;
  uint32_t wb = b;

  uint32_t r = a + b;

  cpu->f_n = false;
  cpu->f_c = r & 0x10000;
  cpu->f_h = (wa ^ wb ^ r) & 0x1000;
  /* cpu->f_z is not altered */

  return r;
}

uint16_t cpu_sub_set_flags(CPU *cpu, uint16_t a, uint16_t b) {
  /* Check for carry using 16bit arithmetic */
  uint16_t al = a;
  uint16_t bl = b;

  uint16_t r = al - bl;

  cpu->f_z = !(r & 0xff);
  cpu->f_n = true;
  cpu->f_h = (a ^ b ^ r) & 0x10;
  cpu->f_c = r & 0x100;

  return r;
}

uint8_t cpu_sbc_set_flags(CPU *cpu, uint8_t a, uint8_t b) {
  /* Check for carry using 16bit arithmetic */
  uint16_t al = a;
  uint16_t bl = b;
  uint16_t c = cpu->f_c;

  uint16_t r = al - bl - c;

  cpu->f_z = !(r & 0xff);
  cpu->f_n = true;
  cpu->f_h = (a ^ b ^ r) & 0x10;
  cpu->f_c = r & 0x100;

  return r;
}

uint8_t cpu_add_set_flags(CPU *cpu, uint8_t a, uint8_t b) {
  uint16_t al = a;
  uint16_t bl = b;

  uint16_t r = al + bl;

  cpu->f_z = !(r & 0xff);
  cpu->f_n = false;
  cpu->f_h = (a ^ b ^ r) & 0x10;
  cpu->f_c = r & 0x100;

  return r;
}

uint8_t cpu_adc_set_flags(CPU *cpu, uint8_t a, uint8_t b) {
  /* Check for carry using 16bit arithmetic */
  uint16_t al = a;
  uint16_t bl = b;
  uint16_t c = cpu->f_c;

  uint16_t r = al + bl + c;

  cpu->f_z = !(r & 0xff);
  cpu->f_n = false;
  cpu->f_h = (a ^ b ^ r) & 0x10;
  cpu->f_c = r & 0x100;

  return r;
}

uint16_t cpu_add_sp_si8(CPU *cpu) {

}
