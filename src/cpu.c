#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gb.h"

void cpu_printf(CPU *cpu) {
  printf("*******************\n");
  printf("af 0x%04X\n", cpu->af);
  printf(" f 0b%08b\n", cpu->f);
  printf(" a 0x%02X\n", cpu->a);
  printf("bc 0x%04X\n", cpu->bc);
  printf(" b 0x%02X\n", cpu->b);
  printf(" c 0x%02X\n", cpu->c);
  printf("de 0x%04X\n", cpu->de);
  printf(" d 0x%02X\n", cpu->d);
  printf(" e 0x%02X\n", cpu->e);
  printf("hl 0x%04X\n", cpu->hl);
  printf(" h 0x%02X\n", cpu->h);
  printf(" l 0x%02X\n", cpu->l);
  printf("sp 0x%04X\n", cpu->sp);
  printf(" s 0x%02X\n", cpu->s);
  printf(" p 0x%02X\n", cpu->p);
  printf("pc 0x%04X\n", cpu->pc);
}

void RegisterReset(CPU *cpu) {
  cpu->af = 0x0000;
  cpu->bc = 0x0000;
  cpu->de = 0x0000;
  cpu->hl = 0x0000;
  cpu->sp = 0xfffe;

  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = false;
}

uint8_t RegisterINC(CPU *cpu, uint8_t value) {
  uint8_t r = (value + 1) & 0xff;
  cpu->f_z = (r == 0);
  cpu->f_n = false;
  cpu->f_h = ((value & 0xf) == 0xf);
  return r;
}

uint8_t RegisterDEC(CPU *cpu, uint8_t value) {
  uint8_t r = (value - 1) & 0xff;
  cpu->f_z = (r == 0);
  cpu->f_n = true;
  cpu->f_h = ((value & 0xf) == 0);
  return r;
}

uint16_t RegisterAdd16(CPU *cpu, uint16_t a, uint16_t b) {
  uint32_t wa = a;
  uint32_t wb = b;
  uint32_t r = a + b;

  cpu->f_n = false;
  cpu->f_c = r & 0x10000;
  cpu->f_h = (wa ^ wb ^ r) & 0x1000;
  return r;
}

void RegisterRLCA(CPU *cpu) {
  uint8_t a = cpu->a;
  uint8_t c;

  c = a >> 7;
  a = (a << 1) & 0xff;
  a |= c;

  cpu->a = a;

  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void RegisterHLAdd16(CPU *cpu, uint16_t value) {
  uint32_t wa = (uint32_t)cpu->hl;
  uint32_t wb = (uint32_t)value;
  uint32_t r = wa + wb;

  cpu->f_n = false;
  cpu->f_c = r & 0x10000;
  cpu->f_h = (wa ^ wb ^ r) & 0x1000;
  cpu->hl = (uint16_t)r;
}

uint16_t *RegisterDecodeR16(CPU *cpu, uint8_t id) {
  switch (id) {
  case 0b00000000:
    return &cpu->bc;
  case 0b00010000:
    return &cpu->de;
  case 0b00100000:
    return &cpu->hl;
  default: // 0b00110000
    return &cpu->sp;
  }
}

void gb_cpu_reset(struct gb *gb) {
  struct gb_cpu *cpu = &gb->cpu;

  cpu->irq_enable = false;
  cpu->irq_enable_next = false;

  cpu->halted = false;

  RegisterReset(cpu);

  /* XXX For the time being we don't emulate the BOOTROM so we start the
   * execution just past it */
  cpu->pc = 0x100;

  if (gb->gbc) {
    /* In GBC mode the boot ROM sets A to 0x11 before starting the game.
     * The game can use this to detect whether it's running on DMG or GBC.
     */
    cpu->a = 0x11;
  }
}

static inline void gb_cpu_clock_tick(struct gb *gb, int32_t cycles) {
  gb->timestamp += cycles >> gb->double_speed;

  if (gb->timestamp >= gb->sync.first_event) {
    /* We have a device sync pending */
    gb_sync_check_events(gb);
  }
}

/****************
 * Instructions *
 ****************/

typedef void (*gb_instruction_f)(struct gb *);

/* Addresses of the interrupt handlers in memory */
static const uint16_t gb_irq_handlers[5] = {[GB_IRQ_VSYNC] = 0x0040,
                                            [GB_IRQ_LCD_STAT] = 0x0048,
                                            [GB_IRQ_TIMER] = 0x0050,
                                            [GB_IRQ_SERIAL] = 0x0058,
                                            [GB_IRQ_INPUT] = 0x0060};

// Update
void CPUTick(CPU *cpu, unsigned int tick) {
  gb_cpu_clock_tick(cpu->memory, tick);
}

uint8_t MemoryRead(Memory *memory, uint16_t address) {
  uint8_t b = gb_memory_readb(memory, address);
  gb_cpu_clock_tick(memory, 4);
  return b;
}

void MemoryWrite(Memory *memory, uint16_t address, uint8_t value) {
  gb_memory_writeb(memory, address, value);
  gb_cpu_clock_tick(memory, 4);
}

uint8_t CPUReadNextI8(CPU *cpu) {
  uint8_t i8 = MemoryRead(cpu->memory, cpu->pc);
  cpu->pc = (cpu->pc + 1) & 0xffff;
  return i8;
}

uint16_t CPUReadNextI16(CPU *cpu) {
  uint16_t low = CPUReadNextI8(cpu);
  uint16_t high = CPUReadNextI8(cpu);
  return (high << 8) | low;
}

void CPUMemoryWriteI8(CPU *cpu, uint16_t address, uint8_t value) {
  MemoryWrite(cpu->memory, address, value);
}

void CPUMemoryWriteI16(CPU *cpu, uint16_t address, uint16_t value) {
  MemoryWrite(cpu->memory, address, (uint8_t)(value & 0xff));
  MemoryWrite(cpu->memory, address + 1, (uint8_t)((value >> 8) & 0xff));
}

void CPU_LOAD_PC(CPU *cpu, uint16_t pc) {
  cpu->pc = pc;
  CPUTick(cpu, 4);
}

static CPUInstruction cpuInstructions[0x100];

void NOP(CPU *) {}

static size_t opcode = 0x00;

uint16_t *DecodeR16(CPU *cpu) {
  switch (opcode & 0b00110000) {
  case 0b00000000:
    return &cpu->bc;
  case 0b00010000:
    return &cpu->de;
  case 0b00100000:
    return &cpu->hl;
  default: // 0b00110000
    return &cpu->sp;
  }
}

uint16_t *DecodeR16Mem(CPU *cpu) {
  static uint16_t r16mem;
  switch (opcode & 0b00110000) {
  case 0b00000000:
    return &cpu->bc;
  case 0b00010000:
    return &cpu->de;
  case 0b00100000:
    r16mem = cpu->hl;
    cpu->hl++;
    return &r16mem;
  default: // 0b00110000
    r16mem = cpu->hl;
    cpu->hl--;
    return &r16mem;
  }
}

uint8_t *DecodeR8(CPU *cpu) {
  switch (opcode & 0b00111000) {
  case 0b00000000:
    return &cpu->b;
  case 0b00001000:
    return &cpu->c;
  case 0b00010000:
    return &cpu->d;
  case 0b00011000:
    return &cpu->e;
  case 0b00100000:
    return &cpu->h;
  case 0b00101000:
    return &cpu->l;
  case 0b00110000:
    return NULL; // Update late [HL]
  default:       // 0b00111000:
    return &cpu->a;
  }
}

uint8_t *DecodeR8Source(CPU *cpu) {
  switch (opcode & 0b00000111) {
  case 0b00000000:
    return &cpu->b;
  case 0b00000001:
    return &cpu->c;
  case 0b00000010:
    return &cpu->d;
  case 0b00000011:
    return &cpu->e;
  case 0b00000100:
    return &cpu->h;
  case 0b00000101:
    return &cpu->l;
  case 0b00000110:
    return NULL; // Update late [HL]
  default:       // 0b00000111:
    return &cpu->a;
  }
}

void LD_R16_IMM16(CPU *cpu) {
  uint16_t *r16 = DecodeR16(cpu);
  *r16 = CPUReadNextI16(cpu);
}

void LD_MR16MEM_A(CPU *cpu) {
  uint16_t *r16mem = DecodeR16Mem(cpu);
  CPUMemoryWriteI8(cpu, *r16mem, cpu->a);
}

void LD_A_MR16MEM(CPU *cpu) {
  uint16_t *r16mem = DecodeR16Mem(cpu);
  cpu->a = MemoryRead(cpu->memory, *r16mem);
}

void LD_MIMM16_SP(CPU *cpu) {
  uint16_t imm16 = CPUReadNextI16(cpu);
  CPUMemoryWriteI16(cpu, imm16, cpu->sp);
}

void INC_R16(CPU *cpu) {
  uint16_t *r16 = DecodeR16(cpu);
  uint16_t value = *r16;
  value = (value + 1) & 0xffff;
  *r16 = value;
  CPUTick(cpu, 4);
}

void DEC_R16(CPU *cpu) {
  uint16_t *r16 = DecodeR16(cpu);
  uint16_t value = *r16;
  value = (value - 1) & 0xffff;
  *r16 = value;
  CPUTick(cpu, 4);
}

void ADD_HL_R16(CPU *cpu) {
  uint16_t *r16 = DecodeR16(cpu);
  RegisterHLAdd16(cpu, *r16);
  CPUTick(cpu, 4);
}

void INC_R8(CPU *cpu) {
  if (((opcode >> 3) & 0b00000111) == 0b00000110) {
    uint8_t value = MemoryRead(cpu->memory, cpu->hl);
    value = RegisterINC(cpu, value);
    MemoryWrite(cpu->memory, cpu->hl, value);
  } else {
    uint8_t *r8 = DecodeR8(cpu);
    uint8_t value = RegisterINC(cpu, *r8);
    *r8 = value;
  }
}

void DEC_R8(CPU *cpu) {
  if (((opcode >> 3) & 0b00000111) == 0b00000110) {
    uint8_t value = MemoryRead(cpu->memory, cpu->hl);
    value = RegisterDEC(cpu, value);
    MemoryWrite(cpu->memory, cpu->hl, value);
  } else {
    uint8_t *r8 = DecodeR8(cpu);
    uint8_t value = RegisterDEC(cpu, *r8);
    *r8 = value;
  }
}

void LD_R8_IMM8(CPU *cpu) {
  if (((opcode >> 3) & 0b00000111) == 0b00000110) {
    MemoryWrite(cpu->memory, cpu->hl, CPUReadNextI8(cpu));
  } else {
    uint8_t *r8 = DecodeR8(cpu);
    *r8 = CPUReadNextI8(cpu);
  }
}

void CPURLCA(CPU *cpu) {
  uint8_t a = cpu->a;
  uint8_t c;

  c = a >> 7;
  a = (a << 1) & 0xff;
  a |= c;

  cpu->a = a;
  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPURRCA(CPU *cpu) {
  uint8_t a = cpu->a;
  uint8_t c;

  c = a & 1;
  a = a >> 1;
  a |= (c << 7);

  cpu->a = a;
  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPURLA(CPU *cpu) {
  uint8_t a = cpu->a;
  uint8_t c = cpu->f_c;
  uint8_t new_c;

  /* Current carry goes to LSB of A, MSB of A becomes new carry */
  new_c = a >> 7;
  a = (a << 1) & 0xff;
  a |= c;

  cpu->a = a;
  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = new_c;
}

void CPURRA(CPU *cpu) {
  uint8_t a = cpu->a;
  uint8_t c = cpu->f_c;
  uint8_t new_c;

  /* Current carry goes to MSB of A, LSB of A becomes new carry */
  new_c = a & 1;
  a = a >> 1;
  a |= (c << 7);

  cpu->a = a;
  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = new_c;
}

void CPUDAA(CPU *cpu) {
  uint8_t a = cpu->a;
  uint8_t adj = 0;

  /* See if we had a carry/borrow for the low nibble in the last operation */
  if (cpu->f_h) {
    /* Yes, we have to adjust it. */
    adj |= 0x06;
  }

  /* See if we had a carry/borrow for the high nibble in the last operation */
  if (cpu->f_c) {
    // Yes, we have to adjust it.
    adj |= 0x60;
  }

  if (cpu->f_n) {
    /* If the operation was a substraction we're done since we can never
     * end up in the A-F range by substracting without generating a
     * (half)carry. */
    a -= adj;
  } else {
    /* Additions are a bit more tricky because we might have to adjust
     * even if we haven't overflowed (and no carry is present). For
     * instance: 0x8 + 0x4 -> 0xc. */
    if ((a & 0xf) > 0x09) {
      adj |= 0x06;
    }

    if (a > 0x99) {
      adj |= 0x60;
    }

    a += adj;
  };

  cpu->a = a;
  cpu->f_z = (a == 0);
  cpu->f_c = ((adj & 0x60) != 0);
  cpu->f_h = false;
}

void CPUCPL(CPU *cpu) {
  /* Complement A */
  cpu->a = ~cpu->a;
  cpu->f_n = true;
  cpu->f_h = true;
}

void CPUSCF(CPU *cpu) {
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = true;
}

void CPUCCF(CPU *cpu) {
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = !cpu->f_c;
}

void CPU_JR_IMM8(CPU *cpu) {
  uint8_t i8 = CPUReadNextI8(cpu);
  uint16_t pc = cpu->pc;
  pc += (int8_t)(i8);
  CPU_LOAD_PC(cpu, pc);
}

bool DecodeCond(CPU *cpu) {
  switch (opcode & 0b00011000) {
  case 0b00000000:
    return !cpu->f_z;
  case 0b00001000:
    return cpu->f_z;
  case 0b00010000:
    return !cpu->f_c;
  default: // 0b00011000:
    return cpu->f_c;
  }
}

void CPU_JR_COND_IMM8(CPU *cpu) {
  if (DecodeCond(cpu)) {
    CPU_JR_IMM8(cpu);
  } else {
    CPUReadNextI8(cpu);
  }
}

void CPU_STOP(CPU *cpu) {
  struct gb *gb = cpu->memory;
  if (gb->speed_switch_pending) {
    /* If a speed change has been requested it is executed on STOP and the
     * execution resumes normally after that. */
    /* XXX: this should take about 16ms when switching to double-speed
     * mode and about 32ms when switching back to normal mode */

    /* Clock speed is going to change, synchronize the relevant devices
     * with the current clock speed */
    gb_timer_sync(gb);
    gb_dma_sync(gb);

    gb->double_speed = !gb->double_speed;

    /* Re-sync with new prediction */
    gb_timer_sync(gb);
    gb_dma_sync(gb);

    return;
  }

  // XXX TODO: stop CPU and screen until button press
  fprintf(stderr, "Implement STOP!\n");
  die();
}

void CPU_LD_R8_R8(CPU *cpu) {
  uint8_t value;
  if ((opcode & 0b00000111) == 0b00000110) {
    value = MemoryRead(cpu->memory, cpu->hl);
  } else {
    uint8_t *src = DecodeR8Source(cpu);
    value = *src;
  }

  if (((opcode >> 3) & 0b00000111) == 0b00000110) {
    MemoryWrite(cpu->memory, cpu->hl, value);
  } else {
    uint8_t *dst = DecodeR8(cpu);
    *dst = value;
  }
}

void CPU_HALT(CPU *cpu) { cpu->halted = true; }

uint8_t CPU_ADD_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
  /* Check for carry using 16bit arithmetic */
  uint16_t al = a;
  uint16_t bl = b;

  uint16_t r = al + bl;

  cpu->f_z = !(r & 0xff);
  cpu->f_n = false;
  cpu->f_h = (a ^ b ^ r) & 0x10;
  cpu->f_c = r & 0x100;

  return r;
}

void CPU_ADD_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_ADD_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_ADC_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
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

void CPU_ADC_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_ADC_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_SUB_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
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

void CPU_SUB_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_SUB_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_SBC_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
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

void CPU_SBC_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_SBC_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_AND_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
  uint8_t r = a & b;

  cpu->f_z = (r == 0);
  cpu->f_n = false;
  cpu->f_h = true;
  cpu->f_c = false;

  return r;
}

void CPU_AND_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_AND_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_XOR_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
  uint8_t r = a ^ b;

  cpu->f_z = (r == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = false;

  return r;
}

void CPU_XOR_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_XOR_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_OR_SET_FLAGS(CPU *cpu, uint8_t a, uint8_t b) {
  uint8_t r = a | b;

  cpu->f_z = (r == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = false;

  return r;
}

void CPU_OR_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  cpu->a = CPU_OR_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_CP_A_R8(CPU *cpu) {
  uint8_t value;
  uint8_t *r8 = DecodeR8Source(cpu);
  if (r8) {
    value = *r8;
  } else {
    value = MemoryRead(cpu->memory, cpu->hl);
  }
  CPU_SUB_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_ADD_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_ADD_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_ADC_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_ADC_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_SUB_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_SUB_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_SBC_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_SBC_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_AND_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_AND_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_XOR_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_XOR_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_OR_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  cpu->a = CPU_OR_SET_FLAGS(cpu, cpu->a, value);
}

void CPU_CP_A_IMM8(CPU *cpu) {
  uint8_t value = CPUReadNextI8(cpu);
  CPU_SUB_SET_FLAGS(cpu, cpu->a, value);
}

uint8_t CPU_POPB(CPU *cpu) {
  uint8_t value = MemoryRead(cpu->memory, cpu->sp);
  cpu->sp = (cpu->sp + 1) & 0xffff;
  return value;
}

uint16_t CPU_POPW(CPU *cpu) {
  uint16_t low = CPU_POPB(cpu);
  uint16_t high = CPU_POPB(cpu);
  return low | (high << 8);
}

void CPU_RET(CPU *cpu) {
  uint16_t addr = CPU_POPW(cpu);
  CPU_LOAD_PC(cpu, addr);
}

void CPU_RET_COND(CPU *cpu) {
  if (DecodeCond(cpu)) {
    CPU_RET(cpu);
  }
  CPUTick(cpu, 4);
}

void CPU_RETi(CPU *cpu) {
  CPU_RET(cpu);
  cpu->irq_enable = true;
  cpu->irq_enable_next = true;
}

void CPU_JP_IMM16(CPU *cpu) {
  uint16_t i16 = CPUReadNextI16(cpu);
  CPU_LOAD_PC(cpu, i16);
}

void CPU_JP_COND_IMM16(CPU *cpu) {
  if (DecodeCond(cpu)) {
    CPU_JP_IMM16(cpu);
  } else {
    CPUReadNextI16(cpu);
  }
}

void CPU_JP_HL(CPU *cpu) { CPU_LOAD_PC(cpu, cpu->hl); }

void CPU_PUSHB(CPU *cpu, uint8_t value) {
  cpu->sp = (cpu->sp - 1) & 0xffff;
  MemoryWrite(cpu->memory, cpu->sp, value);
}

void CPU_PUSHW(CPU *cpu, uint16_t value) {
  CPU_PUSHB(cpu, value >> 8);
  CPU_PUSHB(cpu, value & 0xff);
}

void CPU_RST(CPU *cpu, uint16_t target) {
  CPU_PUSHW(cpu, cpu->pc);
  CPU_LOAD_PC(cpu, target);
}

void CPU_CALL_IMM16(CPU *cpu) {
  uint16_t i16 = CPUReadNextI16(cpu);
  CPU_RST(cpu, i16);
}

void CPU_CALL_COND_IMM16(CPU *cpu) {
  if (DecodeCond(cpu)) {
    CPU_CALL_IMM16(cpu);
  } else {
    CPUReadNextI16(cpu);
  }
}

void CPU_RST_TGT3(CPU *cpu) {
  static uint16_t address[] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
  uint8_t index = (opcode & 0b00111000) >> 3;
  CPU_RST(cpu, address[index]);
}

uint16_t *DecodeR16skt(CPU *cpu) {
  switch (opcode & 0b00110000) {
  case 0b00000000:
    return &cpu->bc;
    break;
  case 0b00010000:
    return &cpu->de;
    break;
  case 0b00100000:
    return &cpu->hl;
    break;
  default: // 0b00110000;
    /*return &cpu->af; */
    return NULL;
  }
}

void CPU_POP_R16stk(CPU *cpu) {
  uint16_t *r16 = DecodeR16skt(cpu);
  if (r16) {
    *r16 = CPU_POPW(cpu);
  } else {
    cpu->f = (CPU_POPB(cpu) & 0b11110000);
    cpu->a = CPU_POPB(cpu);

    /* Restore flags from memory (low 4 bits are ignored) */
    cpu->f_z = cpu->fz;
    cpu->f_n = cpu->fn;
    cpu->f_h = cpu->fh;
    cpu->f_c = cpu->fc;
  }
}

void CPU_PUSH_R16stk(CPU *cpu) {
  uint16_t *r16 = DecodeR16skt(cpu);
  if (r16) {
    CPU_PUSHW(cpu, *r16);
  } else {
    cpu->fc = cpu->f_c;
    cpu->fh = cpu->f_h;
    cpu->fn = cpu->f_n;
    cpu->fz = cpu->f_z;

    CPU_PUSHB(cpu, cpu->a);
    CPU_PUSHB(cpu, cpu->f);
    CPUTick(cpu, 4);
  }
}

void CPU_LDH_MemC_A(CPU *cpu) {
  uint16_t addr = 0xff00 | cpu->c;
  CPUMemoryWriteI8(cpu, addr, cpu->a);
}

void CPU_LDH_imm8_A(CPU *cpu) {
  uint8_t i8 = CPUReadNextI8(cpu);
  uint16_t addr = 0xff00 | i8;
  MemoryWrite(cpu->memory, addr, cpu->a);
}

void CPU_LD_Imm16_A(CPU *cpu) {
  uint16_t i16 = CPUReadNextI16(cpu);
  MemoryWrite(cpu->memory, i16, cpu->a);
}

void CPU_LDH_A_MemC(CPU *cpu) {
  uint16_t addr = 0xff00 | cpu->c;
  cpu->a = MemoryRead(cpu->memory, addr);
}

void CPU_LDH_A_imm8(CPU *cpu) {
  uint16_t addr = 0xff00 | CPUReadNextI8(cpu);
  cpu->a = MemoryRead(cpu->memory, addr);
}

void CPU_LD_A_imm16(CPU *cpu) {
  uint16_t addr = CPUReadNextI16(cpu);
  cpu->a = MemoryRead(cpu->memory, addr);
}

uint16_t CPUAddSPsi8(CPU *cpu) {
  /* Offset is signed */
  int8_t i8 = CPUReadNextI8(cpu);

  /* Use 32bit arithmetic to avoid signed integer overflow UB */
  int32_t r = cpu->sp;
  r += i8;

  cpu->f_z = false;
  cpu->f_n = false;
  /* Carry and Half-carry are for the low byte */
  cpu->f_h = (cpu->sp ^ i8 ^ r) & 0x10;
  cpu->f_c = (cpu->sp ^ i8 ^ r) & 0x100;

  return (uint16_t)r;
}

void CPU_ADD_SP_Imm8(CPU *cpu) {
  cpu->sp = CPUAddSPsi8(cpu);
  CPUTick(cpu, 8);
}

void CPU_LD_HL_SP_ADD_Imm8(CPU *cpu) {
  cpu->hl = CPUAddSPsi8(cpu);
  CPUTick(cpu, 4);
}

void CPU_LD_SP_HL(CPU *cpu) {
  cpu->sp = cpu->hl;
  CPUTick(cpu, 4);
}

void CPU_DI(CPU *cpu) {
  cpu->irq_enable = false;
  cpu->irq_enable_next = false;
}

void CPU_EI(CPU *cpu) {
  /* Interrupts are re-enabled after the *next* instruction */
  cpu->irq_enable_next = true;
}

uint8_t cbOpcode;

uint8_t *DecodeCBOperand(CPU *cpu) {
  switch (cbOpcode & 0b00000111) {
  case 0b00000000:
    return &cpu->b;
    break;
  case 0b00000001:
    return &cpu->c;
    break;
  case 0b00000010:
    return &cpu->d;
    break;
  case 0b00000011:
    return &cpu->e;
    break;
  case 0b00000100:
    return &cpu->h;
    break;
  case 0b00000101:
    return &cpu->l;
    break;
  case 0b00000110: // HL
    return NULL;
    break;
  default: // case 0b00000111:
    return &cpu->a;
  }
}

void CPUCBRLCSetFlags(CPU *cpu, uint8_t *v) {
  uint8_t c = *v >> 7;
  *v = (*v << 1) | c;
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPUCBRRCSetFlags(CPU *cpu, uint8_t *v) {
  uint8_t c = *v & 1;
  *v = (*v >> 1) | (c << 7);
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPUCBRLSetFlags(CPU *cpu, uint8_t *v) {
  bool new_c = *v >> 7;
  *v = (*v << 1) | (uint8_t)cpu->f_c;
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = new_c;
}

void CPUCBRRSetFlags(CPU *cpu, uint8_t *v) {
  bool new_c = *v & 1;
  uint8_t old_c = cpu->f_c;
  *v = (*v >> 1) | (old_c << 7);
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = new_c;
}

void CPUCBSLASetFlags(CPU *cpu, uint8_t *v) {
  bool c = *v >> 7;
  *v = *v << 1;
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPUCBSRASetFlags(CPU *cpu, uint8_t *v) {
  bool c = *v & 1;
  /* Sign-extend */
  *v = (*v >> 1) | (*v & 0x80);
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPUCBSWAPSetFlags(CPU *cpu, uint8_t *v) {
  *v = ((*v << 4) | (*v >> 4)) & 0xff;
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = false;
}

void CPUCBSRLSetFlags(CPU *cpu, uint8_t *v) {
  bool c = *v & 1;
  *v = *v >> 1;
  cpu->f_z = (*v == 0);
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = c;
}

void CPU_CB_R8(CPU *cpu) {
  void (*cbInstruction)(CPU *, uint8_t *) = NULL;
  switch (cbOpcode & 0b11111000) {
  case 0b00000000:
    cbInstruction = CPUCBRLCSetFlags;
    break;
  case 0b00001000:
    cbInstruction = CPUCBRRCSetFlags;
    break;
  case 0b00010000:
    cbInstruction = CPUCBRLSetFlags;
    break;
  case 0b00011000:
    cbInstruction = CPUCBRRSetFlags;
    break;
  case 0b00100000:
    cbInstruction = CPUCBSLASetFlags;
    break;
  case 0b00101000:
    cbInstruction = CPUCBSRASetFlags;
    break;
  case 0b00110000:
    cbInstruction = CPUCBSWAPSetFlags;
    break;
  default: // 0b00111000
    cbInstruction = CPUCBSRLSetFlags;
    break;
  }
  uint8_t *r8 = DecodeCBOperand(cpu);
  if (r8) {
    cbInstruction(cpu, r8);
  } else {
    uint8_t value = MemoryRead(cpu->memory, cpu->hl);
    cbInstruction(cpu, &value);
    MemoryWrite(cpu->memory, cpu->hl, value);
  }
}

void CPUBitSetFlags(CPU *cpu, uint8_t *v, uint8_t bit) {
  bool set = *v & (1U << bit);
  cpu->f_z = !set;
  cpu->f_n = false;
  cpu->f_h = true;
}

void CPUResSetFlags(CPU *, uint8_t *v, uint8_t bit) { *v = *v & ~(1U << bit); }

void CPUSetSetFlags(CPU *, uint8_t *v, uint8_t bit) { *v = *v | (1U << bit); }

void CPU_CB_BIT(CPU *cpu) {
  void (*cbInstructionBit)(CPU *, uint8_t *, uint8_t) = NULL;
  switch (cbOpcode & 0b11000000) {
  case 0b01000000:
    cbInstructionBit = CPUBitSetFlags;
    break;
  case 0b10000000:
    cbInstructionBit = CPUResSetFlags;
    break;
  default: // 0b11000000:
    cbInstructionBit = CPUSetSetFlags;
    break;
  }
  uint8_t bit = (cbOpcode >> 3) & 0b111;
  uint8_t *r8 = DecodeCBOperand(cpu);
  if (r8) {
    cbInstructionBit(cpu, r8, bit);
  } else {
    uint8_t value = MemoryRead(cpu->memory, cpu->hl);
    cbInstructionBit(cpu, &value, bit);
    MemoryWrite(cpu->memory, cpu->hl, value);
  }
}

CPUInstruction cpuCBInstructions[0x100];

void CPUCBInit() {
  for (size_t indexOpcodeCB = 0; indexOpcodeCB < 0x100; indexOpcodeCB++) {
    if ((indexOpcodeCB & 0b11000000) == 0b00000000) {
      cpuCBInstructions[indexOpcodeCB] = CPU_CB_R8;
    } else {
      cpuCBInstructions[indexOpcodeCB] = CPU_CB_BIT;
    }
  }
}

void CPU_CB(CPU *cpu);

void gb_cpu_init() {
  printf("Init function\n");
  memset(cpuInstructions, 0x00, 0x100 * sizeof(CPUInstruction));
  memset(cpuCBInstructions, 0x00, 0x100 * sizeof(CPUInstruction));

  for (size_t indexOpcode = 0; indexOpcode < 0x100; indexOpcode++) {
    uint8_t block0 = indexOpcode & 0b11001111;
    if (block0 == 0b00000001) {
      cpuInstructions[indexOpcode] = LD_R16_IMM16;
      continue;
    } else if (block0 == 0b00000010) {
      cpuInstructions[indexOpcode] = LD_MR16MEM_A;
      continue;
    } else if (block0 == 0b00001010) {
      cpuInstructions[indexOpcode] = LD_A_MR16MEM;
      continue;
    } else if (block0 == 0b00000011) {
      cpuInstructions[indexOpcode] = INC_R16;
      continue;
    } else if (block0 == 0b00001011) {
      cpuInstructions[indexOpcode] = DEC_R16;
      continue;
    } else if (block0 == 0b00001001) {
      cpuInstructions[indexOpcode] = ADD_HL_R16;
      continue;
    }

    block0 = indexOpcode & 0b11000111;
    if (block0 == 0b00000100) {
      cpuInstructions[indexOpcode] = INC_R8;
      continue;
    } else if (block0 == 0b00000101) {
      cpuInstructions[indexOpcode] = DEC_R8;
      continue;
    } else if (block0 == 0b00000110) {
      cpuInstructions[indexOpcode] = LD_R8_IMM8;
      continue;
    }

    block0 = indexOpcode & 0b11100111;
    if (block0 == 0b00100000) {
      cpuInstructions[indexOpcode] = CPU_JR_COND_IMM8;
      continue;
    }

    uint8_t block1 = indexOpcode & 0b11000000;
    if (block1 == 0b01000000) {
      cpuInstructions[indexOpcode] = CPU_LD_R8_R8;
      continue;
    }

    uint8_t block2 = indexOpcode & 0b11111000;
    if (block2 == 0b10000000) {
      cpuInstructions[indexOpcode] = CPU_ADD_A_R8;
      continue;
    } else if (block2 == 0b10001000) {
      cpuInstructions[indexOpcode] = CPU_ADC_A_R8;
      continue;
    } else if (block2 == 0b10010000) {
      cpuInstructions[indexOpcode] = CPU_SUB_A_R8;
      continue;
    } else if (block2 == 0b10011000) {
      cpuInstructions[indexOpcode] = CPU_SBC_A_R8;
      continue;
    } else if (block2 == 0b10100000) {
      cpuInstructions[indexOpcode] = CPU_AND_A_R8;
      continue;
    } else if (block2 == 0b10101000) {
      cpuInstructions[indexOpcode] = CPU_XOR_A_R8;
      continue;
    } else if (block2 == 0b10110000) {
      cpuInstructions[indexOpcode] = CPU_OR_A_R8;
      continue;
    } else if (block2 == 0b10111000) {
      cpuInstructions[indexOpcode] = CPU_CP_A_R8;
      continue;
    }

    uint8_t block3 = indexOpcode & 0b11100111;
    if (block3 == 0b11000000) {
      cpuInstructions[indexOpcode] = CPU_RET_COND;
      continue;
    } else if (block3 == 0b11000010) {
      cpuInstructions[indexOpcode] = CPU_JP_COND_IMM16;
      continue;
    } else if (block3 == 0b11000100) {
      cpuInstructions[indexOpcode] = CPU_CALL_COND_IMM16;
      continue;
    }

    block3 = indexOpcode & 0b11000111;
    if (block3 == 0b11000111) {
      cpuInstructions[indexOpcode] = CPU_RST_TGT3;
      continue;
    }

    block3 = indexOpcode & 0b11001111;
    if (block3 == 0b11000001) {
      cpuInstructions[indexOpcode] = CPU_POP_R16stk;
      continue;
    } else if (block3 == 0b11000101) {
      cpuInstructions[indexOpcode] = CPU_PUSH_R16stk;
      continue;
    }
  }

  cpuInstructions[0x00] = NOP;
  cpuInstructions[0x08] = LD_MIMM16_SP;
  cpuInstructions[0x07] = CPURLCA;
  cpuInstructions[0x0f] = CPURRCA;
  cpuInstructions[0x17] = CPURLA;
  cpuInstructions[0x1f] = CPURRA;
  cpuInstructions[0x27] = CPUDAA;
  cpuInstructions[0x2f] = CPUCPL;
  cpuInstructions[0x37] = CPUSCF;
  cpuInstructions[0x3f] = CPUCCF;
  cpuInstructions[0x18] = CPU_JR_IMM8;
  cpuInstructions[0x10] = CPU_STOP;

  cpuInstructions[0x76] = CPU_HALT;

  cpuInstructions[0b11000110] = CPU_ADD_A_IMM8;
  cpuInstructions[0b11001110] = CPU_ADC_A_IMM8;
  cpuInstructions[0b11010110] = CPU_SUB_A_IMM8;
  cpuInstructions[0b11011110] = CPU_SBC_A_IMM8;
  cpuInstructions[0b11100110] = CPU_AND_A_IMM8;
  cpuInstructions[0b11101110] = CPU_XOR_A_IMM8;
  cpuInstructions[0b11110110] = CPU_OR_A_IMM8;
  cpuInstructions[0b11111110] = CPU_CP_A_IMM8;

  cpuInstructions[0b11001001] = CPU_RET;
  cpuInstructions[0b11011001] = CPU_RETi;

  cpuInstructions[0b11000011] = CPU_JP_IMM16;
  cpuInstructions[0b11101001] = CPU_JP_HL;

  cpuInstructions[0b11001101] = CPU_CALL_IMM16;

  cpuInstructions[0b11001011] = CPU_CB;

  cpuInstructions[0b11100010] = CPU_LDH_MemC_A;
  cpuInstructions[0b11100000] = CPU_LDH_imm8_A;
  cpuInstructions[0b11101010] = CPU_LD_Imm16_A;
  cpuInstructions[0b11110010] = CPU_LDH_A_MemC;
  cpuInstructions[0b11110000] = CPU_LDH_A_imm8;
  cpuInstructions[0b11111010] = CPU_LD_A_imm16;

  cpuInstructions[0b11101000] = CPU_ADD_SP_Imm8;
  cpuInstructions[0b11111000] = CPU_LD_HL_SP_ADD_Imm8;
  cpuInstructions[0b11111001] = CPU_LD_SP_HL;

  cpuInstructions[0b11110011] = CPU_DI;
  cpuInstructions[0b11111011] = CPU_EI;

  CPUCBInit();
}

static void gb_cpu_check_interrupts(struct gb *gb) {
  struct gb_cpu *cpu = &gb->cpu;
  struct gb_irq *irq = &gb->irq;
  uint8_t active_irq;
  uint16_t handler;
  unsigned i;

  /* See if we have an interrupt pending */
  active_irq = irq->irq_enable & irq->irq_flags & 0x1f;

  if (!active_irq) {
    return;
  }

  /* We have an active IRQ, that gets us outside of halted mode even if the
   * IME is not set in the CPU */
  cpu->halted = false;

  if (!cpu->irq_enable) {
    /* IME is not set, nothing to do */
    return;
  }

  /* Find the first active IRQ. The order is significant, IRQs with a lower
   * number have the priority. */
  for (i = 0; i < 5; i++) {
    if (active_irq & (1U << i)) {
      break;
    }
  }

  /* That shouldn't happen since we check if we have an active IRQ above */
  assert(i < 5);

  handler = gb_irq_handlers[i];

  cpu->irq_enable = false;
  cpu->irq_enable_next = false;

  /* Entering Interrupt context takes 12 cycles */
  gb_cpu_clock_tick(gb, 12);

  /* Push current PC on the stack */
  /*gb_cpu_pushw(gb, gb->cpu.pc);*/
  CPU_PUSHW(cpu, cpu->pc);

  /* We're about to handle this interrupt, acknowledge it */
  irq->irq_flags &= ~(1U << i);

  /* Jump to the IRQ handler */
  CPU_LOAD_PC(cpu, handler);
}

int32_t gb_cpu_run_cycles(struct gb *gb, int32_t cycles) {
  struct gb_cpu *cpu = &gb->cpu;
  /* Rebase the synchronization timestamps, which has the side effect of
   * setting gb->timestamp to 0 */
  gb_sync_rebase(gb);

  while (gb->timestamp < cycles) {
    /* We check for interrupt before anything else since it could get us
     * out of halted mode */
    gb_cpu_check_interrupts(gb);
    cpu->irq_enable = cpu->irq_enable_next;

    if (cpu->halted) {
      int32_t skip_cycles;

      /* The CPU is halted so we skip to the next event or `cycles`,
       * whichever comes first */
      if (cycles < gb->sync.first_event) {
        skip_cycles = cycles - gb->timestamp;
      } else {
        skip_cycles = gb->sync.first_event - gb->timestamp;
      }

      gb_cpu_clock_tick(gb, skip_cycles << gb->double_speed);

      /* See if any event needs to run. This may trigger an IRQ which
       * will un-halt the CPU in the next iteration */
      gb_sync_check_events(gb);

    } else {
      opcode = CPUReadNextI8(&gb->cpu);
      CPUInstruction instruction = cpuInstructions[opcode];
      instruction(&gb->cpu);
    }
  }

  return gb->timestamp;
}

void CPU_CB(CPU *cpu) {
  cbOpcode = CPUReadNextI8(cpu);
  CPUInstruction cpuCBInstruction = cpuCBInstructions[cbOpcode];
  cpuCBInstruction(cpu);
}
