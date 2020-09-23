# as(GNU Assembler)
https://en.wikipedia.org/wiki/X86_assembly_language
https://en.wikibooks.org/wiki/X86_Assembly/GAS_Syntax#Additional_GAS_reading 入门教程
https://sourceware.org/binutils/docs/as/ 完整文档
https://cs.lmu.edu/~ray/notes/gasexamples/ 最佳入门内容

## 关键问题解释

1. .code32 && .code16

https://stackoverflow.com/questions/32395542/objdump-of-code16-and-code32-x86-assembly
https://stackoverflow.com/questions/26539603/why-bootloaders-for-x86-use-16bit-code-first/31528128

`.code` 16 tells the assembler to assume the code will be run in 16bit mode




## 问题
#### include 
1. include 源文件

https://stackoverflow.com/questions/39457263/include-assembly-file-in-another-assembly-file
```
# include "test.S"
```

2. include C header

https://stackoverflow.com/questions/4928238/include-header-with-c-declarations-in-an-assembly-file-without-errors

