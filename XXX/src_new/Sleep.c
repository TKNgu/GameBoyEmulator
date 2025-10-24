#include "Sleep.h"

#include <time.h>

#define MILLI_NANO 1000000L
#define TIME_KERNEL_SLEEP 10000L

void NanoSleep(long nanoseconds) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (nanoseconds > TIME_KERNEL_SLEEP) {
        long tmp = nanoseconds - TIME_KERNEL_SLEEP;
        struct timespec sleepTime = {
            .tv_sec = tmp / SECONDS2NANO,
            .tv_nsec = tmp % SECONDS2NANO,
        };
        nanosleep(&sleepTime, NULL);
    }

    struct timespec now;
    long elapsed;
    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed = (now.tv_sec - start.tv_sec) * SECONDS2NANO +
                  (now.tv_nsec - start.tv_nsec);
    } while (elapsed < nanoseconds);
}

void MilliSleep(long milliseconds) {
    NanoSleep(milliseconds * MILLI_NANO);
}
