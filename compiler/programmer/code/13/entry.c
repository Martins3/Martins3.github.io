#include "minicrt.h"
/**
 * ref: https://cs.lmu.edu/~ray/notes/syscalls/
*/

int crt_heap_init();
extern int main(int argc, char *argv[]);


static void crt_fatal_error(const char *msg) {
  crt_exit(1);
}

void mini_crt_entry(void) {
  int argc;
  char **argv;
  char *ebp_reg = 0;

  asm("movq %%rbp, %0 \n" : "=r"(ebp_reg));

  argc = *(int *)(ebp_reg + 8);
  argv = (char **)(ebp_reg + 16);

  if(crt_heap_init()){
    crt_fatal_error("init heap failed");
  }

  crt_exit(main(argc, argv));
}

void crt_exit(int exit_code) {
  // FIXME
  asm("movq %0, %%rbx \n\t"
      "movl $1, %%eax \n\t"
      "int $0x80 \n\t"
      "hlt \n\t" ::"m"(exit_code));
}

