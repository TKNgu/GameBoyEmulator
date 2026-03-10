#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gb.h"
#include "sdl.h"
#include "mmu.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <rom>\n", argv[0]);
    return EXIT_FAILURE;
  }

  struct gb *gb = calloc(1, sizeof(*gb));
  if (NULL == gb) {
    perror("calloc failed");
    return EXIT_FAILURE;
  }

    mmu.gb = gb;

  const char *rom_file = argv[1];

  // AUDIO
  /* Initialize the semaphores before we start the frontend */
  for (size_t i = 0; i < GB_SPU_SAMPLE_BUFFER_COUNT; i++) {
    struct gb_spu_sample_buffer *buf = &gb->spu.buffers[i];

    memset(buf->samples, 0, sizeof(buf->samples));

    /* We initialize the buffers by saying that they're ready to be sent
     * to the frontend. This way the frontend won't starve for audio while
     * we start the emulation. */
    sem_init(&buf->free, 0, 0);
    sem_init(&buf->ready, 0, 1);
  }

  gb_sdl_frontend_init(gb);

  gb_cart_load(gb, rom_file);
  gb_sync_reset(gb);
  gb_irq_reset(gb);

  struct gb_cpu *cpu = &gb->cpu;
  cpu->irq_enable = false;
  cpu->irq_enable_next = false;

  cpu->halted = false;

  cpu->sp = 0xfffe;
  cpu->af = 0x0000;
  cpu->bc = 0x0000;
  cpu->de = 0x0000;
  cpu->hl = 0x0000;

  cpu->f_z = false;
  cpu->f_n = false;
  cpu->f_h = false;
  cpu->f_c = false;

  /* XXX For the time being we don't emulate the BOOTROM so we start the
   * execution just past it */
  cpu->pc = 0x100;

  if (gb->gbc) {
    /* In GBC mode the boot ROM sets A to 0x11 before starting the game.
     * The game can use this to detect whether it's running on DMG or GBC.
     */
    cpu->a = 0x11;
  }
  cpu->gb = gb;

  gb_gpu_reset(gb);
  gb_input_reset(gb);
  gb_dma_reset(gb);
  gb_timer_reset(gb);
  gb_spu_reset(gb);

  gb->iram_high_bank = 1;
  gb->vram_high_bank = false;
  gb->quit = false;
  gb->double_speed = false;
  gb->speed_switch_pending = false;

  while (!gb->quit) {
    gb->frontend.refresh_input(gb);

    /* We refresh the input at 120Hz. This is a trade-off, if we refresh
     * faster we'll reduce latency at the cost of performance. */
    gb_cpu_run_cycles(gb, GB_CPU_FREQ_HZ / 120);
  }

  gb->frontend.destroy(gb);
  gb_cart_unload(gb);

  free(gb);

  return 0;
}
