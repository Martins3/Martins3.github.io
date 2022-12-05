#include "minicrt.h"

int puts(const char *str, FILE *stream) {
  int len = strlen(str);
  if (fwrite(str, 1, len, stream) != len) {
    return EOF;
  }
  return len;
}

int putc(const char *c) {
  if (fwrite(c, 1, 1, stdout) != 1) {
    return EOF;
  }
  return 1;
}

static char *convert(long long int num, int base) {
  static char Representation[] = "0123456789abcdef";
  static char buffer[50];
  char *ptr;

  ptr = &buffer[49];
  *ptr = '\0';

  do {
    *--ptr = Representation[num % base];
    num /= base;
  } while (num != 0);

  return (ptr);
}

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

int static _puts(const char *str) {
  int len = 0;
  while (*str != '\0') {
    putc(str);
    str++;
    len++;
  }
  return len;
}

int printf(const char *fmt, ...) {
  const char *traverse;
  unsigned int i;
  char c;
  char *s;
  int len = 0;

  // Module 1: Initializing Myprintf's arguments
  va_list arg;
  va_start(arg, fmt);

  for (traverse = fmt; *traverse != '\0'; traverse++) {
    while (*traverse != '%' && *traverse != '\0') {
      putc(traverse);
      len++;
      traverse++;
    }

    if (*traverse == '\0') {
      break;
    }

    traverse++;

    // Module 2: Fetching and executing arguments
    switch (*traverse) {
    case 'c':
      // FIXME
      c = va_arg(arg, int); // Fetch char argument
      putc(&c);
      len++;
      break;

    case 'd':
      i = va_arg(arg, int); // Fetch Decimal/Integer argument
      if (i < 0) {
        i = -i;
        static char the_bar = '-';
        putc(&the_bar);
        len++;
      }
      len += _puts(convert(i, 10));
      break;

    case 'o':
      i = va_arg(arg, unsigned int); // Fetch Octal representation
      len += _puts(convert(i, 8));
      break;

    case 's':
      s = va_arg(arg, char *); // Fetch string
      len += _puts(s);
      break;

    case 'p':
      s = va_arg(arg, void *); // Fetch pointer
      len += _puts(convert((long long)s, 16));
      break;

    case 'x':
      i = va_arg(arg, unsigned int); // Fetch Hexadecimal representation
      len += _puts(convert(i, 16));
      break;
    }
  }

  // Module 3: Closing argument list to necessary clean-up
  va_end(arg);

  return len;
}
