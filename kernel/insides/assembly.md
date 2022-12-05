https://0xax.github.io/categories/assembler/
// 一共含有8个章节，都可以看一下


1. bp指针是为了函数调用的软件方便，有没有什么办法取消bp指针?
可以的，但是需要小心的控制编译器，让编译器所有sp的操作严格对称。


```
                         +--------------+
                         |              |
                    +    |              |
                    |    +--------------+
                    |    |              |
                    |    |   arg(N-1)   |  starts from 7'th argument for x86_64
                    |    |              |
                    |    +--------------+
                    |    |              |
                    |    |     argN     |
                    |    |              |
                    |    +--------------+
                    |    |              |
                    |    |Return address|  %rbp + 8
Stack grows down    |    |              |
                    |    +--------------+
                    |    |              |
                    |    |     %rbp     |  Frame base pointer
                    |    |              |
                    |    +--------------+
                    |    |              |
                    |    |  local var1  |  %rbp - 8
                    |    |              |
                    |    +--------------+
                    |    |              |
                    |    | local ^ar 2  | <-- %rsp
                    |    |              |
                    v    +--------------+
                         |              |
                         |              |
                         +--------------+
```
从图中解释:
1. sp指向总是表示stack顶端
2. sp的数值是可以被如call ret push指令修改的，或者mov直接修改
3. 在函数调用的过程中间,一般含有如下的
```
  push    rbp
  mov     rbp, rsp
  sub     rsp, 24
```
那么bp和sp分别是函数stack空间的首尾,同时[bp]中间保存的值就是上一个函数的bp, 

如果当前函数是最后的一个函数，那么当前函数不需要`sub rsp, 24`。只是为后面的函数调用提供条件而已。


函数调用约定:
https://en.wikibooks.org/wiki/X86_Disassembly/Calling_Conventions

In the CDECL calling convention the following holds:
1. Arguments are passed on the stack in Right-to-Left order, and return values are passed in eax.
2. The calling function cleans the stack. This allows CDECL functions to have variable-length argument lists (aka variadic functions). For this reason the number of arguments is not appended to the name of the function by the compiler, and the assembler and the linker are therefore unable to determine if an incorrect number of arguments is used.

```
_cdecl int MyFunction1(int a, int b) {
  return a + b;
}
```

and the following function call:

```
 x = MyFunction1(2, 3);
```

These would produce the following assembly listings, respectively:

```
_MyFunction1:
push ebp
mov ebp, esp
mov eax, [ebp + 8]
mov edx, [ebp + 12]
add eax, edx
pop ebp
ret
```

and

```
push 3
push 2
call _MyFunction1
add esp, 8
```

从文档中间可以看出:

ret指令的
```
  rtl_pop(&t0);
  rtl_j(t0);
  print_asm("ret");
```

```
make_EHelper(call) {
  rtl_push(eip);
  rtl_j(decoding.jmp_eip);
  print_asm("call %x", decoding.jmp_eip);
}
```
```

make_EHelper(leave) {
  cpu.esp = cpu.ebp;
  rtl_pop(&cpu.ebp);
  print_asm("leave");
}

```

# x87 FPU 


# hello world
```
section .data
    msg db      "hello, world!"

section .text
    global _start
_start:
    mov     rax, 1
    mov     rdi, 1
    mov     rsi, msg
    mov     rdx, 13
    syscall
    mov    rax, 60
    mov    rdi, 0
    syscall
```




# Ref
1. https://godbolt.org/



# ques
1. `global`
2. `.text`
3. nasm
4. section



