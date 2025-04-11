#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "MMU.h"

struct CPU {
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
};

int InitCPU(struct CPU *cpu);
int CPUTick(struct CPU *cpu, struct MMU *mmu);

#endif
