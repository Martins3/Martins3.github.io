#include "minicrt.h"
static int write(long long fd, const void *buffer, long long size) {
  long long ret = 0;
  asm("movq $4, %%rax \n\t"
      "movq %1, %%rbx \n\t"
      "movq %2, %%rcx \n\t"
      "movq %3, %%rdx \n\t"
      "int $0x80       \n\t"
      "movq %%rax, %0 \n\t"
      : "=m"(ret)
      : "m"(fd), "m"(buffer), "m"(size));
  if(ret == -14){
   crt_exit(14);
  }

  return ret;
}

int fwrite(const void *buffer, int size, int count, FILE *stream) {
  return write((long long int)stream, buffer, size * count);
};
