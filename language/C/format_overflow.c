/** 程序说明：
  %#x：按16进制输出，并在前面加上0x
  %.20d：按10进制输出，输出20位，并在前面补0
  %n：将显示内容的长度输出到一个变量中去，换句话说%n的作用就是把已经打印出来字符的数量保存到对应的内存地址当中
*/
#include <stdio.h>
int main() {
  int num = 0x61616161;
  printf("Before : num = %#x \n", num);
  /*溢出语句: %n将长度20保存到地址为&num的内存，覆盖num*/
  printf("%d%n\n", num, &num);
  /* printf("%d%n\n", num); */ // 触发错误，
  printf("After : num = %#x \n", num);
  return 0;
}
