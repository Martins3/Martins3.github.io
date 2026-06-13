#include "minicrt.h"

unsigned strlen(const char *str) {
  int cnt = 0;
  if (!str)
    return 0;
  for (; *str != '\0'; ++str)
    cnt++;
  return cnt;
}
char *strcpy(char *destination, const char *source) {
  if (destination == NULL)
    return NULL;

  char *ptr = destination;

  while (*source != '\0') {
    *destination = *source;
    destination++;
    source++;
  }

  *destination = '\0';

  return ptr;
}
