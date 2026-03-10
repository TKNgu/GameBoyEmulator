#include <stddef.h>
#include <stdlib.h>

struct Buffer {
  size_t size;
  char *data;
};

void init_buffer(struct Buffer *buffer) {
  buffer->size = 0;
  buffer->data = NULL;
}

void release_buffer(struct Buffer *buffer) {
  buffer->size = 0;
  if (buffer->data) {
    free(buffer->data);
  }
}

void add_buffer(struct Buffer *buffer, char data) {}

int main(int argc, char *argv[]) {
  struct Buffer data;
  init_buffer(&data);
  release_buffer(&data);
  return 0;
}
