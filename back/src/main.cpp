#include <cstdint>
#include <iostream>

struct BaseClock {
    bool state = false;

    void update() {
        state = !state;
    }
};

struct PhiClock {
    bool state = false;
    int count = 1;

    void update(BaseClock &baseClock) {
        if (baseClock.state) {
            count++;
            if (count == 2) {
                count = 0;
                state = !state;
            }
        }
    }
};

struct SoCBus {
    uint16_t address;

    uint8_t read() {
        return 0;
    }

    void write(uint8_t) {
    }
};

struct CPURegister {
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
};

int main() {
    std::cout << "Start Emulator" << std::endl;

    int baseClockTick = 2;
    int phiClockTick = 0;

    bool isRunning = true;
    while (isRunning) {
        baseClockTick++;
        if (baseClockTick == 3) {
            baseClockTick = 0;
            phiClockTick++;
        }
        if (phiClockTick) {
            phiClockTick = 0;
        }
    }

    // BaseClock baseClock;
    // PhiClock phiClock;
    //
    // int countSample = 100;
    // bool isRunning = true;
    // while (isRunning) {
    //     baseClock.update();
    //     phiClock.update(baseClock);
    //
    //     std::cout << baseClock.state << " " << phiClock.state << std::endl;
    //
    //     isRunning = countSample-- >= 0;
    // }
    return 0;
}
