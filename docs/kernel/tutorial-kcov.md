## 参考这两个文档
- https://docs.kernel.org/dev-tools/kcov.html : 用户态
  - https://github.com/SimonKagstrom/kcov

- https://docs.kernel.org/dev-tools/gcov.html : 分析内核代码本身

一般的基本流程:
```sh
gcc -fPIC -fprofile-arcs -ftest-coverage -Wall -Werror a.c
./a.out
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory out
```

find . -name "*.gcda"

## 使用 https://docs.kernel.org/dev-tools/gcov.html 中的 Appendix B : gather_on_test.sh 拷贝出来
解压到 linux/kcov 中

## 参考这个项目
https://github.com/linux-test-project/lcov/blob/master/README


```txt
🧀  lcov --capture --directory kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov --output-file coverage.info
Capturing coverage data from kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov
Found gcov version: 12.2.0
Using intermediate gcov format
Using JSON module JSON::PP
Scanning kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov for .gcda files ...
Found 2763 data files in kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov
Processing arch/x86/mm/fault.gcda
/home/martins3/core/linux/kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov/arch/x86/mm/fault.gcno:cannot open notes file
/home/martins3/core/linux/kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov/arch/x86/mm/fault.gcda:stamp mismatch with notes file
geninfo: ERROR: GCOV failed for /home/martins3/core/linux/kcov/sys/kernel/debug/gcov/home/martins3/core/linux/kcov/arch/x86/mm/fault.gcda!
```

这个软连接指向的位置不对。

但是似乎什么都没做，然后问题就解决了。

然后:
```sh
genhtml coverage.info --output-directory out
```


## 这是内核的流程?
https://lpc.events/event/11/contributions/1075/attachments/819/1543/Code_Coverage_LPC_Safety_MC.pdf
