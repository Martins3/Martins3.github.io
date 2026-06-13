#include <setjmp.h>
#include <stdio.h>

jmp_buf env;

void abc(void) {
  int error = 1;

  if (error)
    longjmp(env, 1);
}

int call_sigsetjmp(void) { return setjmp(env); }

void xyz(void) {
  int v1 = 1;
  int v3 = 1;

  if (call_sigsetjmp()) {
    printf("[martins3:%s:%d] %d %d\n", __FUNCTION__, __LINE__, v1, v3);
    return;
  }

  v1++;
  v3++;

  abc();
}

/*
 * 这个一运行直接结束了。
 * 需要理解一下 call_sigsetjmp 的含义。
 */
int main(void) {
  xyz();
  return 0;
}
