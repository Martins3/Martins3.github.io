#include <assert.h>  // assert
#include <errno.h>   // strerror
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp
#include <unistd.h> // sleep

struct cpu_regs_t {
  union {
    struct {
      uint32_t eax;
      uint32_t ebx;
      uint32_t ecx;
      uint32_t edx;
    };
    uint32_t regs[4];
  };
};


void cpuid_dump_normal(struct cpu_regs_t *regs) {
  char buffer[sizeof(struct cpu_regs_t) + 1];
  printf("CPUID %08x:%02x = %08x %08x %08x %08x | %s\n", state->last_leaf.eax,
         state->last_leaf.ecx, regs->eax, regs->ebx, regs->ecx, regs->edx,
         reg_to_str(buffer, regs));
}

int main(int argc, char *argv[]) { return 0; }
