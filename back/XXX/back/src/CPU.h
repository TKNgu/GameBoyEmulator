#ifndef CPU_h
#define CPU_h

#include <stdint.h>

#include "Memory.h"

typedef struct {
    struct {
        union {
            uint16_t AF;
            struct {
                uint8_t F;
                uint8_t A;
            };
        };
        union {
            uint16_t BC;
            struct {
                uint8_t C;
                uint8_t B;
            };
        };
        union {
            uint16_t DE;
            struct {
                uint8_t E;
                uint8_t D;
            };
        };
        union {
            uint16_t HL;
            struct {
                uint8_t L;
                uint8_t H;
            };
        };
        union {
            uint16_t SP;
            struct {
                uint8_t P;
                uint8_t S;
            };
        };
        uint16_t PC;
    } registers;
    Memory *memory;
} CPU;

void InitCPU(CPU *cpu, Memory *memory);
void CPUStep(CPU *cpu);

#endif
