#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void print_maps() {
  FILE *map;
  map = fopen("/proc/self/maps", "r");

  char line[512];

  while (!feof(map)) {
    if (fgets(line, 512, map) == NULL)
      break;
    if (strstr(line, "[heap]") != NULL) {
      printf("%s", line);
    }
  }
}

int main(int argc, char *argv[]) {
  print_maps();

  char *a = (char *)malloc(sizeof(char) * 20);
  for (int i = 0; i < 60; ++i) {
    *a = 'b';
    a = (char *)malloc(sizeof(char) * 900);
    print_maps();
    printf("%p\n", a);
    *a = 'b';
    // putchar(*a);
  }

  return 0;
}
