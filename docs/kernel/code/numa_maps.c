#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void err_quit(char *msg) {
  printf("%s", msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  int fdin;
  char *src;
  struct stat statbuf;
  if (argc != 2)
    err_quit("usage: a.out <file>");

  /* open the input file */
  if ((fdin = open(argv[1], O_RDONLY)) < 0) {
    printf("can't open %s for reading", argv[1]);
    return 0;
  }

  /* find size of input file */
  if (fstat(fdin, &statbuf) < 0) {
    printf("fstat error");
    return 0;
  }

  char z = 'a';
  /* mmap the input file */
  for (int i = 0; i < 10; ++i) {
    if ((src = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0)) ==
        (caddr_t)-1) {
      printf("mmap error for input");
      return 0;
    }
    printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__);
    for (unsigned long j = 0; j < statbuf.st_size; j = j + (4 << 10)) {
      z += *(src + j);
    }
  }

  printf("finished ! %c\n", z);

  sleep(10000);

  return 0;
}
