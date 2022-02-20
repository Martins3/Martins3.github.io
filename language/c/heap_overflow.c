//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUFFERSIZE 16
#define OVERLAYSIZE 8 /* 我们将覆盖buf2 的前OVERLAYSIZE 个字节 */
int main() {
  ulong diff;
  /*动态分配长度为BUFFER-SIZE的2个内存块，并将返回指针赋值给buf1、 buf2*/
  char *buf1 = (char *)malloc(BUFFERSIZE);
  char *buf2 = (char *)malloc(BUFFERSIZE);
  /*将分配的两个内存块指针相减，将地址差值赋值给diff*/
  diff = (ulong)buf2 - (ulong)buf1;
  /*打印内存块指针buf1和buf2的值，内存块地址差diff的十六进制值和十进制值*/
  printf("buf1 = %p, buf2 = %p, diff = 0x %lx(%lu) bytes\n", buf1, buf2, diff,
         diff);
  /* 将buf2指向的内存块存放字符串，前面字节用’a’填充，最后一个字节填充结束符’\0’*/
  memset(buf2, 'a', BUFFERSIZE - 1);
  buf2[BUFFERSIZE - 1] = '\0';
  printf("before overflow : buf2 = %s \n", buf2); /*打印溢出前buf2指向的字符串*/
  /* 用diff+OVERLAYSIZE 个’b’填充buf1，覆盖了buf2的前8个字节*/
  memset(buf1, 'b', (uint)(diff + OVERLAYSIZE)); /*溢出语句*/
  printf("after overflow : buf2 = %s \n", buf2);
  /*打印溢出后buf2指向的字符串*/
  return 0;
}
