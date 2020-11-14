#include <unistd.h>

int main(int argc, char *argv[]) {
  unsigned long syscall_nr = 60;
  long exit_status = 42;

  syscall(syscall_nr, exit_status);
}
