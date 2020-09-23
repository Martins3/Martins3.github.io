#include <stdio.h>
int main() {
  char name[16];
  gets(name);
  for (int i = 0; i < 16 && name[i]; i++)
    printf("%c", name[i]);
}
