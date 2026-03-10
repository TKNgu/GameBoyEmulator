#ifndef micro_h
#define micro_h

#include "cpu.h"
#include <stdint.h>

uint8_t cpu_inc(CPU *cpu, uint8_t v);
uint8_t cpu_dec(CPU *cpu, uint8_t v);

uint16_t cpu_addw_set_flags(CPU *cpu, uint16_t a, uint16_t b);
uint16_t cpu_sub_set_flags(CPU *cpu, uint16_t a, uint16_t b);

uint8_t cpu_sbc_set_flags(CPU *cpu, uint8_t a, uint8_t b);

uint8_t cpu_add_set_flags(CPU *cpu, uint8_t a, uint8_t b);

uint8_t cpu_adc_set_flags(CPU *cpu, uint8_t a, uint8_t b);

uint16_t cpu_add_sp_si8(CPU *cpu);

#endif
