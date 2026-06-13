#include<stdio.h>
namespace myname {
  int var = 42;
} // namespace name

extern "C" double _ZN6myname3varE;

int main(void){
  printf("%d\n", _ZN6myname3varE);
  return 0;
}
