#include <stdio.h>
#include <libtcc.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *buffer;
    size_t size;
    size_t maxSize;
} BufferCode;

void InitBufferCode(BufferCode *bufferCode) {
    bufferCode->maxSize = 512;
    bufferCode->buffer = (char *)malloc(bufferCode->maxSize * sizeof(char));
    bufferCode->size = 0;
}

void ReleaseBufferCode(BufferCode *bufferCode) {
    free(bufferCode->buffer);
}

void BufferCodeAdd(BufferCode *bufferCode, char *code, size_t size) {
    if (bufferCode->size + size - 1 >= bufferCode->maxSize) {
        bufferCode->maxSize <<= 1;
        bufferCode->buffer = realloc(bufferCode->buffer, bufferCode->maxSize);
    }
    memcpy(bufferCode->buffer + bufferCode->size, code, (size - 1) * sizeof(char));
    bufferCode->size += size - 1;
    bufferCode->buffer[bufferCode->size] = 0x00;
}

void BufferCodeShow(BufferCode *bufferCode) {
    printf("%s\n", bufferCode->buffer);
}

int Add(int a, int b) {
    return a + b;
}

int main(int argc, char *argv[]) {
    BufferCode bufferCode;
    InitBufferCode(&bufferCode);

    char microCodeAdd[] = "int Add(int, int);\n";
    BufferCodeAdd(&bufferCode, microCodeAdd, sizeof(microCodeAdd));

    char addX2[] = "int AddX2(int a, int b) {return Add(Add(a, b), b);}\n";
    BufferCodeAdd(&bufferCode, addX2, sizeof(addX2));

    BufferCodeShow(&bufferCode);

    TCCState *tccState = tcc_new();
    tcc_set_output_type(tccState, TCC_OUTPUT_MEMORY);
    tcc_add_symbol(tccState, "Add", (const void *)Add);
    tcc_compile_string(tccState, bufferCode.buffer);
    tcc_relocate(tccState, TCC_RELOCATE_AUTO);
    int (*add)(int, int) = (int (*)(int, int))tcc_get_symbol(tccState, "Add");
    int (*fx2)(int, int) = (int (*)(int, int))tcc_get_symbol(tccState, "AddX2");
    printf("Call %d\n", fx2(4,  5));

RELEASE:
    tcc_delete(tccState);
    ReleaseBufferCode(&bufferCode);
    return 0;
}
