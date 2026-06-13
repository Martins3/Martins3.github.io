#include "minicrt.h"
typedef struct __heap_header {
  enum {
    HEAP_BLOCK_FREE = 0xABABABAB,
    HEAP_BLOCK_USED = 0xCDCDCDCD,
  } type;

  unsigned size;
  struct __heap_header *next;
  struct __heap_header *prev;
} heap_header;

#define ADDR_ADD(a, o) (((char *)(a)) + o)
#define HEADER_SIZE (sizeof(heap_header))

static heap_header *list_head = NULL;

void crt_free(void *ptr) {

  heap_header *header;
  header = (heap_header *)ADDR_ADD(ptr, -HEADER_SIZE);
  if (header->type == HEAP_BLOCK_FREE) {
    printf("Warning : double free !\n");
    return;
  }

  header->type = HEAP_BLOCK_FREE;
  // merge if possible
  if (header->next != NULL && header->next->type == HEAP_BLOCK_FREE) {
    header->size += header->next->size;
    // FIXME double linked list , why only forward direction
    header->next = header->next->next;
  }

  if (header->prev != NULL && header->prev->type == HEAP_BLOCK_FREE) {
    header->prev->next = header->next;
    if (header->next != NULL) {
      header->next->prev = header->prev;
    }

    header->prev->size += header->size;
  }
}

void crt_print_memory_usage() {
  heap_header *header = list_head;

  printf("---------\n");
  while (header != NULL) {
    if (header->type == HEAP_BLOCK_USED) {
      printf("%p USED %d\n", header, header->size);
    } else {
      printf("%p FREE %d\n", header, header->size);
    }

    header = header->next;
  }
  printf("---------\n");
}

void *crt_malloc(unsigned size) {
  heap_header *header;
  if (!size)
    return NULL;
  header = list_head;

  while (header != NULL) {
    if (header->type == HEAP_BLOCK_USED) {
      header = header->next;
      continue;
    }

    if (header->size > size + HEADER_SIZE &&
        header->size <= size + HEADER_SIZE * 2) {
      header->type = HEAP_BLOCK_USED;
      // FIXME it should be author's bug, I fix it
      return ADDR_ADD(header, HEADER_SIZE);
    }

    if (header->size > size + HEADER_SIZE * 2) {
      // split
      heap_header *next = (heap_header *)ADDR_ADD(header, size + HEADER_SIZE);
      next->prev = header;
      next->next = header->next;
      next->type = HEAP_BLOCK_FREE;
      next->size = header->size - (size + HEADER_SIZE);
      header->next = next;
      header->size = size + HEADER_SIZE;
      header->type = HEAP_BLOCK_USED;
      return ADDR_ADD(header, HEADER_SIZE);
    }

    header = header->next;
  }

  return NULL;
}

static long long int crt_brk(void *end_data_segment) {
  long long ret = 0;

  asm("movq $45, %%rax \n\t"
      "movq %1, %%rbx \n\t"
      "int $0x80 \n\t"
      "movq %%rax, %0 \n\t"
      : "=r"(ret)
      : "m"(end_data_segment));
  return ret;
}

int crt_heap_init() {
  void *base = NULL;
  heap_header *header = NULL;
  unsigned heap_size = 10000;

  base = (void *)crt_brk(0);
  void *end = ADDR_ADD(base, heap_size);

  end = (void *)crt_brk(end);

  if (end != ADDR_ADD(base, heap_size))
    return -1;

  header = (heap_header *)base;

  header->size = heap_size;
  header->type = HEAP_BLOCK_FREE;
  header->next = header->prev = NULL;

  list_head = header;
  return 0;
}
