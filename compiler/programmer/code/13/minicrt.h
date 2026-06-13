#ifndef MINICRT_H_F2BTARHE
#define MINICRT_H_F2BTARHE

typedef unsigned long FILE;
#define EOF -1

#define stdin ((FILE *)0)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)

#define NULL (void *)0

void crt_exit(int);
int puts(const char *str, FILE *stream);
int fwrite(const void *buffer, int size, int count, FILE *stream);

void crt_free(void *ptr);
void *crt_malloc(unsigned size);
void crt_print_memory_usage();

char* strcpy(char* destination, const char* source);

int printf(const char *fmt, ...);

unsigned strlen(const char *str);


/**
 * testimony: https://en.wikibooks.org/wiki/X86_Assembly/Interfacing_with_Linux
 */
#endif /* end of include guard: MINICRT_H_F2BTARHE */
