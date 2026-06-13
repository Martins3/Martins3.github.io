#include "minicrt.h"

/**
 * For some unknown reason, we can not print string from string
 */

void a() {
  char *s = (char *)crt_malloc(sizeof(char) * 4);
  s[0] = 'C';
  s[1] = 'R';
  s[2] = 'T';
  s[3] = '\0';
  // puts(s, stdout);
  int a;
  printf("what is the address %s\n", s);
}

void b() {
 char str[] = "safsdfad";
  puts(str, stdout);
}

void c() {
  crt_print_memory_usage();

  char *s = (char *)crt_malloc(sizeof(char) * 1000);
  crt_print_memory_usage();

  crt_free(s);
  crt_print_memory_usage();

  char *a = (char *)crt_malloc(sizeof(char) * 1000);
  crt_print_memory_usage();

  char *b = (char *)crt_malloc(sizeof(char) * 1000);
  crt_print_memory_usage();

  char *c = (char *)crt_malloc(sizeof(char) * 1000);
  crt_print_memory_usage();

  crt_free(b);
  crt_print_memory_usage();

  char *d = (char *)crt_malloc(sizeof(char) * 2000);
  crt_print_memory_usage();

  crt_free(a);
  crt_print_memory_usage();
}

extern int end;
extern int etext;
extern int edata;

void d() {
  printf("BSS END : %p\n", &end);
  printf("TEXT END : %p\n", &etext);
  printf("DATA END : %p\n", &edata);

  crt_print_memory_usage();
}

void e() {
  while (1)
    ;
}

void g(int argc, char *argv[]) {
  printf("%d\n", argc);
  char *buffer = (char *)crt_malloc(sizeof(char) * 1000);

  printf("%s\n", "args value");
  for (int i = 0; i < argc; ++i) {
    if (strcpy(buffer, argv[i]) != NULL) {
      printf("%d : %s\n", i, buffer);
    }
  }
}




int main(int argc, char *argv[]) {
  // g(argc, argv);
  c();
  return 0;
}
