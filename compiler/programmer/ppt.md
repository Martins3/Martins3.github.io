https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
> 提供了ELF 的视图
> 为什么section header table 不能直接放在program header table 下面，似乎是随便放到一个位置

```
#include <stdio.h>
extern char end;
int main(int argc, char *argv[]) {
  printf("%p\n", &end);
  return 0;
}
```

进程地址空间




