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
    printf("%s", line);
  }
}

int main(int argc, char *argv[]) {

  print_maps();
  return 0;
}
