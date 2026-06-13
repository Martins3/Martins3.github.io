# 并发调试工具 TSan
Clang ThreadSanitizer (TSan)

https://valgrind.org/docs/manual/hg-manual.html

https://valgrind.org/docs/manual/drd-manual.html

## 内核的工具在 ./kernel 中讨论吧


显然，思考的角度:
1. 原理
2. 性能影响
4. 使用限制
  - 那些无法发现
  - 那些被误判
 
## 两个 san 工具

是所有的编译器都是有这个东西吗?



带 AddressSanitizer 的调试版本，用于检测内存错误（use-after-free、double-free、越界等）：
  gcc -o pthread/elimination-backoff-stack.asan \
      pthread/elimination-backoff-stack.c \
      -lpthread -O0 -g -fsanitize=address -std=c11
  • -fsanitize=address：注入 ASan 运行时检测
  • -O0：关闭优化，确保栈跟踪信息准确
  • 运行时会显著变慢（约 10-20 倍），但能精确定位内存 bug
  还有一个 .tsan
  对话过程中还生成了 ThreadSanitizer 版本（检测 data race）：
  gcc -o pthread/elimination-backoff-stack.tsan ... -fsanitize=thread



<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
