#include <setjmp.h>
#include <stdio.h>

jmp_buf env;

void abc(void) {
  int error = 1;

  if (error)
    longjmp(env, 1);
}

void xyz(void) {
  int v1 = 1;
  int v3 = 1;

  if (setjmp(env)) {
    printf("[martins3:%s:%d] %d %d\n", __FUNCTION__, __LINE__, v1, v3);
    return;
  }

  v1++;
  v3++;

  abc();
}

int main(void) {
  xyz();
  return 0;
}
