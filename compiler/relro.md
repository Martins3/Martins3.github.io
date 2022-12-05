> 本文是对于　https://www.redhat.com/en/blog/hardening-elf-binaries-using-relocation-read-only-relro　的翻译，其中代码含有细微修改，并且在x86_64系统上测试过。

This article describes ELF relocation sections, how to abuse them for arbitrary code execution, and how to protect them at runtime.

这是一篇文章介绍了ELF 可重定向节，并且说明如何利用它执行恶意代码以及如何在运行时保护它。


## ELF Relocation Sections
A dynamically linked ELF binary uses a look-up table called Global Offset Table (GOT) to dynamically resolve functions that are located in shared libraries.
一个动态链接的ELF二进制使用一个叫做全局偏移表(GOT)的查询表来实现动态在共享库中间定位函数。


When you call a function that is located in a shared library, it looks like the following. This is a high-level view of what is going on, there are lots of things the linker is doing that we won’t go into.
当你调用TODO

First, the call is actually pointing to the Procedure Linkage Table (PLT), which exists in the `.plt` section of the binary.
首先，该调用实际上指向了在`.plt`节中间的过程链接表。

`objdump -M intel -d YOUR_BINARY`
```
1294:	e8 c7 fd ff ff       	callq  1060 <printf@plt>
```
The `.plt` section contains x86 instructions that point directly to the GOT, which lives in the `.got.plt` section.
GOT在`.got.plt`节中间，`.plt`节包含了直接跳转到GOT的x86指令。
`objdump -M intel -d YOUR_BINARY`
```
0000000000001020 <.plt>:
    1020:	ff 35 e2 2f 00 00    	pushq  0x2fe2(%rip)        # 4008 <_GLOBAL_OFFSET_TABLE_+0x8>
    1026:	ff 25 e4 2f 00 00    	jmpq   *0x2fe4(%rip)        # 4010 <_GLOBAL_OFFSET_TABLE_+0x10>
    102c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000001030 <strncpy@plt>:
    1030:	ff 25 e2 2f 00 00    	jmpq   *0x2fe2(%rip)        # 4018 <strncpy@GLIBC_2.2.5>
    1036:	68 00 00 00 00       	pushq  $0x0
    103b:	e9 e0 ff ff ff       	jmpq   1020 <.plt>

0000000000001040 <puts@plt>:
    1040:	ff 25 da 2f 00 00    	jmpq   *0x2fda(%rip)        # 4020 <puts@GLIBC_2.2.5>
    1046:	68 01 00 00 00       	pushq  $0x1
    104b:	e9 d0 ff ff ff       	jmpq   1020 <.plt>

0000000000001050 <__stack_chk_fail@plt>:
    1050:	ff 25 d2 2f 00 00    	jmpq   *0x2fd2(%rip)        # 4028 <__stack_chk_fail@GLIBC_2.4>
    1056:	68 02 00 00 00       	pushq  $0x2
    105b:	e9 c0 ff ff ff       	jmpq   1020 <.plt>

0000000000001060 <printf@plt>:
    1060:	ff 25 ca 2f 00 00    	jmpq   *0x2fca(%rip)        # 4030 <printf@GLIBC_2.2.5>
    1066:	68 03 00 00 00       	pushq  $0x3
    106b:	e9 b0 ff ff ff       	jmpq   1020 <.plt>

0000000000001070 <memset@plt>:
    1070:	ff 25 c2 2f 00 00    	jmpq   *0x2fc2(%rip)        # 4038 <memset@GLIBC_2.2.5>
    1076:	68 04 00 00 00       	pushq  $0x4
    107b:	e9 a0 ff ff ff       	jmpq   1020 <.plt>

0000000000001080 <atoi@plt>:
    1080:	ff 25 ba 2f 00 00    	jmpq   *0x2fba(%rip)        # 4040 <atoi@GLIBC_2.2.5>
    1086:	68 05 00 00 00       	pushq  $0x5
    108b:	e9 90 ff ff ff       	jmpq   1020 <.plt>
```
The `.got.plt` section contains binary data. The GOT contain pointers back to the PLT or to the location of the dynamically linked function.
`.got.plt` 包含一些二进制数据。GOT包含指回PLT的指针和动态链接函数。

`objdump -s YOUR_BINARY`

```
Contents of section .got.plt:
 4000 f83d0000 00000000 00000000 00000000  .=..............
 4010 00000000 00000000 36100000 00000000  ........6.......
 4020 46100000 00000000 56100000 00000000  F.......V.......
 4030 66100000 00000000 76100000 00000000  f.......v.......
 4040 86100000 00000000                    ........
```
By default, the GOT is populated dynamically while the program is running. The first time a function is called, the GOT contains a pointer back to the PLT, where the linker is called to find the actual location of the function in question (this is the part we’re not going into detail about). The location found is then written to the GOT. The second time a function is called, the GOT contains the known location of the function. This is called “lazy binding.”
在默认的情况下，GOT会在程序运行时动态填充。当一个函数第一个调用的时候，GOT中间含有一个指回PLT的指针，此时，linker被调用去找到被查询的函数的具体的位置(查询过程我们不会分析)。被查询到的函数地址会被写入到GOT中间。当函数被第二次调用的时候，GOT就会直接提供该函数的地址。这被称为“延迟绑定”。

>  **lazy**
     When generating an executable or shared library, mark it to
     tell the dynamic linker to defer function call resolution to
     the point when the function is called (lazy binding), rather
     than at load time.  Lazy binding is the default.  [1]
> “延迟”
当生成可执行文件或者共享库的时候，让dynamic linker将函数调用的解析从加载时延迟到函数被调用的时间。延迟绑定是默认操作。

There are a couple of design constraints for the GOT and the PLT.

- Because the PLT contains code that is called by the program directly, it needs to be allocated at a known offset from the `.text` segment.

- Because the GOT contains data used by different parts of the program directly, it needs to be allocated at a known static address in memory.

- Because the GOT is “lazy binded,” it needs to be writable.
GOT和PLT含有一系列设计缺陷:
1. 由于PLT中间包含的代码直接被程序调用，所以它的位置和`.text`节的位置保持固定
2. GOT需要被程序的不同位置直接访问的数据，所以它需要被放到内存中间已知的，静态的位置。
3. 由于GOT是“延迟绑定”的，它具有需要可写属性。

## GOT Overwrite
## GOT 覆盖

Since we know that the GOT lives in a predefined place and is writable, all that is needed is a bug that lets an attacker write four bytes anywhere. We’ll use the following vulnerable program to simulate that. Note that we are operating on the same binary we examined above.
由于GOT在事先定义的位置，而且可以被修改，这为攻击者修改GOT表提供了全部需要。接下来使用一段程序来模拟攻击过程，注意上面的代码中

Here we have an intentionality vulnerable program that is hopefully believable enough to make this demo realistic. Understanding this program is not necessary for understanding the exploitation and mitigation techniques being demonstrated, it is only included for completeness.

```
// Include standard I/O declarations
#include <stdio.h>
// Include string declarations
#include <string.h>
#include <stdlib.h>

// Program entry point
int main(int argc, char** argv) {
    // Terminate if program is not run with three parameters.
    if (argc != 4) {
        // Print out the proper use of the program
        puts("./a.out <size> <offset> <string>");
        // Return failure
        return -1;
    }

    // Convert size to an integer
    int size = atoi(argv[1]);
    // Convert offset to an integer
    int offset = atoi(argv[2]);
    // Place string into its own string on the stack
    char* str = argv[3];
    // Declare a 256 byte buffer on the stack
    char buffer[256];

    // Print the location of the buffer for calculating the offset.
    printf("Buffer:\t\t%p\n", &buffer);

    // Fill the buffer with the letter 'A'.
    memset(buffer, 'A', sizeof(buffer));
    // Null-terminate the buffer.
    buffer[255] = 0;
    // Attempt to copy the specified string into the specified location.
    strncpy(buffer + offset, str, size);
    // Print out the buffer.
    printf("%s", buffer);

    // Return success
    return 0;
}
```
`gcc -g -O0 -Wl,-z,norelro -fno-stack-protector -o YOUR_BINARY YOUR_SOURCE_CODE`

First, some reconnaissance. We know from the examination above, that our GOT entry for printf lives at `0x08049754`. We know from tesing the program, that our buffer will live on the stack at `0xbffff284`.

首先，通过前面叙述的内容，我们可以知道GOT对于printf的入口是 `xxx`

> 这他妈,.plt中间反汇编得到的结果根本不一样，内容根本不一样，搞个毛啊


通过测试程序，我的缓冲区的地址在 `xxx`

```
(gdb) x 0x08049754
0x8049754 <_GLOBAL_OFFSET_TABLE_+28>:	0x0804837a
(gdb) p -(0xbffff284 - 0x08049754) % 0x80000000
$1 = 1208263888
(gdb) r 4 1208263888 \$\$\$\$
Starting program: /home/hake/code/relro/c 4 1208263888 \$\$\$\$
Buffer:		bffff284

Program received signal SIGSEGV, Segmentation fault.
0x08048374 in printf@plt ()
(gdb) x 0x08049754
0x8049754 <_GLOBAL_OFFSET_TABLE_+28>:	0x24242424
```
Here, we see that we can overwrite the GOT entry for printf with our string. The program crashes because it’s trying to jump to memory that is not mapped.
从上述的演示，可以看出来我们可以重写printf在GOT中间

## RELRO: RELocation Read-Only
To prevent the above exploitation technique, we can tell the linker to resolve all dynamically linked functions at the beginning of execution and make the GOT read-only. Note that we are operating on a different binary below compiled from the same source code.

> **now**
    When generating an executable or shared library, mark it to
    tell the dynamic linker to resolve all symbols when the program
    is started, or when the shared library is linked to using
    dlopen, instead of deferring function call resolution to the
    point when the function is first called.  [1]

This exploitation mitigation technique is known as RELRO which stands for RELocation Read-Only. The idea is simple, make the relocation sections that are used to resolve dynamically loaded functions read-only. This way, they cannot overwrite them and we cannot take control of execution like we did above.

You can turn on Full RELRO with the gcc compiler option: `-Wl,-z,relro,-z,now`. This gets passed to the linker as `-z relro -z now`. On most modern Linux distributions a variant of RELRO known as Partial RELRO is used by default. Partial RELRO uses the `-z relro` option, but not the `-z now` option.

`gcc -g -O0 -Wl,-z,relro,-z,now -fno-stack-protector -o YOUR_BINARY YOUR_SOURCE_CODE`

```
80484fa:       e8 95 fe ff ff          call   8048394 <printf@plt>

08048394 <printf@plt>:
 8048394:       ff 25 f0 9f 04 08       jmp    DWORD PTR ds:0x8049ff0
 804839a:       68 20 00 00 00          push   0x20
 804839f:       e9 a0 ff ff ff          jmp    8048344 <_init+0x30>

Contents of section .got:
 8049fd4 fc9e0408 00000000 00000000 5a830408  ............Z...
 8049fe4 6a830408 7a830408 8a830408 9a830408  j...z...........
 8049ff4 aa830408 ba830408 00000000           ............
```

The GOT entry for printf lives at `0x08049ff0`. The buffer will live on the stack at `0xbffff284`.
```
(gdb) x 0x08049ff0
0x8049ff0 <_GLOBAL_OFFSET_TABLE_+28>:	0x0804839a
(gdb) p -(0xbffff284 - 0x08049ff0) % 0x80000000
$1 = 1208266092
(gdb) r 4 1208266092 \$\$\$\$
Starting program: /home/hake/code/relro/r 4 1208266092 \$\$\$\$
Buffer:		bffff284

Program received signal SIGSEGV, Segmentation fault.
strncpy (s1=0x8049ff0 "ph\027", s2=0xbffff5fc "$$$", n=4) at strncpy.c:43
43	strncpy.c: No such file or directory.
	in strncpy.c
(gdb) x 0x08049ff0
0x8049ff0 <_GLOBAL_OFFSET_TABLE_+28>:	0x00176870
```
Here, we see that we cannot overwrite the GOT entry for printf with our string. The program crashes because it trying to write to a memory segment that is read-only.

Technical note: All other memory corruption exploitation mitigation techniques were turned off for this demonstration.

Technical note: RELRO automatically applies all the specified protections to following segments: `.ctors`, `.dtors`, `.jcr`, `.dynamic` and `.got`.

## Some Handy Commands

Display Dynamic Relocation Entries `objdump -R YOUR_BINARY`

Show Program Header Table `readelf -l YOUR_BINARY`

Show Section Header Table `readelf -S YOUR_BINARY`

Display Relocations `readelf -r YOUR_BINARY`

[1] Manual page [ld(1)](http://unixhelp.ed.ac.uk/CGI/man-cgi?ld)


[RELRO - A (not so well known) Memory Corruption Mitigation Technique](http://tk-blog.blogspot.com/2009/02/relro-not-so-well-known-memory.html)

[How to Hijack the Global Offset Table with pointers](http://www.exploit-db.com/papers/13203/)

[Chapter 9\. Dynamic Linking: Global Offset Tables](http://bottomupcs.sourceforge.net/csbu/x3824.htm)

[The ELF format - How programs look from the inside](http://greek0.net/elf.html)

[Resolving ELF Relocation Name / Symbols](http://em386.blogspot.com/2006/10/resolving-elf-relocation-name-symbols.html)
