#include "MemoryBank.h"

#include <stdlib.h>

int InitMemoryBank(struct MemoryBank *memoryBank, size_t size, size_t num) {
  memoryBank->memoryBank = (uint8_t **)malloc(sizeof(uint8_t *));
  if (memoryBank->memoryBank == NULL) {
    return 1;
  }
  for (size_t index = 0; index < num; index++) {
    uint8_t *tmp = (uint8_t *)malloc(size);
    if (tmp == NULL) {
      memoryBank->size = index;
      goto RELEASE_MEMORY_BANK;
    }
    memoryBank->memoryBank[index] = tmp;
  }
  memoryBank->size = num;
  return 0;

RELEASE_MEMORY_BANK:
  for (size_t index = 0; index < memoryBank->size; index++) {
    free(memoryBank->memoryBank[index]);
  }
  free(memoryBank->memoryBank);
  return 1;
}

void ReleaseMemoryBank(struct MemoryBank *memoryBank) {
  for (size_t index = 0; index < memoryBank->size; index++) {
    free(memoryBank->memoryBank[index]);
  }
  free(memoryBank->memoryBank);
}

int MemoryBankRealloc(struct MemoryBank *memoryBank, size_t size, size_t num) {
  uint8_t **tmp = (uint8_t **)realloc(memoryBank->memoryBank, num);
  if (tmp == NULL) {
    return 1;
  }
  memoryBank->memoryBank = tmp;
  for (size_t index = memoryBank->size; index < num; index++) {
    memoryBank->memoryBank[index] = (uint8_t *)malloc(size);
    if (memoryBank->memoryBank[index] == NULL) {
      for (size_t subIndex = memoryBank->size; subIndex < index; subIndex++) {
        free(memoryBank->memoryBank[subIndex]);
        return 1;
      }
    }
  }
  memoryBank->size = num;
  return 0;
}
