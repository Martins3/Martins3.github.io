# Introduction
作者为了证明main 函数并不是程序的入口，使用gdb 的方法，但是实际上操作非常有意思。

```
➜  a gdb a.out
GNU gdb (GDB) 8.3
Copyright (C) 2019 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-pc-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from a.out...
(gdb) info files
Symbols from "/tmp/a/a.out".
Local exec file:
	`/tmp/a/a.out', file type elf64-x86-64.
	Entry point: 0x1040
	0x00000000000002a8 - 0x00000000000002c4 is .interp
	0x00000000000002c4 - 0x00000000000002e4 is .note.ABI-tag
	0x00000000000002e8 - 0x0000000000000318 is .hash
	0x0000000000000318 - 0x0000000000000334 is .gnu.hash
	0x0000000000000338 - 0x00000000000003e0 is .dynsym
	0x00000000000003e0 - 0x0000000000000464 is .dynstr
	0x0000000000000464 - 0x0000000000000472 is .gnu.version
	0x0000000000000478 - 0x0000000000000498 is .gnu.version_r
	0x0000000000000498 - 0x0000000000000558 is .rela.dyn
	0x0000000000000558 - 0x0000000000000570 is .rela.plt
	0x0000000000001000 - 0x000000000000101b is .init
	0x0000000000001020 - 0x0000000000001040 is .plt
	0x0000000000001040 - 0x0000000000001215 is .text
	0x0000000000001218 - 0x0000000000001225 is .fini
	0x0000000000002000 - 0x0000000000002016 is .rodata
	0x0000000000002018 - 0x0000000000002054 is .eh_frame_hdr
	0x0000000000002058 - 0x0000000000002150 is .eh_frame
	0x0000000000003dd8 - 0x0000000000003de0 is .init_array
	0x0000000000003de0 - 0x0000000000003de8 is .fini_array
	0x0000000000003de8 - 0x0000000000003fd8 is .dynamic
	0x0000000000003fd8 - 0x0000000000004000 is .got
	0x0000000000004000 - 0x0000000000004020 is .got.plt
	0x0000000000004020 - 0x0000000000004030 is .data
	0x0000000000004030 - 0x0000000000004038 is .bss
(gdb) break *0x1040                                         // run 会导致 suspended ?
Breakpoint 1 at 0x1040
(gdb) run
Starting program: /tmp/a/a.out 
[1]  + 25497 suspended (tty output)  gdb a.out
➜  a fg
[1]  + 25497 continued  gdb a.out
Warning:
Cannot insert breakpoint 1.
Cannot access memory at address 0x1040

(gdb) info files 
Symbols from "/tmp/a/a.out".
Native process:
	Using the running image of child process 25625.
	While running this, GDB does not access memory from...
Local exec file:
	`/tmp/a/a.out', file type elf64-x86-64.
	Entry point: 0x555555555040
	0x00005555555542a8 - 0x00005555555542c4 is .interp      // 之后显示的地址于此不同
	0x00005555555542c4 - 0x00005555555542e4 is .note.ABI-tag
	0x00005555555542e8 - 0x0000555555554318 is .hash
	0x0000555555554318 - 0x0000555555554334 is .gnu.hash
	0x0000555555554338 - 0x00005555555543e0 is .dynsym
	0x00005555555543e0 - 0x0000555555554464 is .dynstr
	0x0000555555554464 - 0x0000555555554472 is .gnu.version
	0x0000555555554478 - 0x0000555555554498 is .gnu.version_r
	0x0000555555554498 - 0x0000555555554558 is .rela.dyn
	0x0000555555554558 - 0x0000555555554570 is .rela.plt
	0x0000555555555000 - 0x000055555555501b is .init
	0x0000555555555020 - 0x0000555555555040 is .plt
	0x0000555555555040 - 0x0000555555555215 is .text
	0x0000555555555218 - 0x0000555555555225 is .fini
	0x0000555555556000 - 0x0000555555556016 is .rodata
	0x0000555555556018 - 0x0000555555556054 is .eh_frame_hdr
	0x0000555555556058 - 0x0000555555556150 is .eh_frame
	0x0000555555557dd8 - 0x0000555555557de0 is .init_array
	0x0000555555557de0 - 0x0000555555557de8 is .fini_array
	0x0000555555557de8 - 0x0000555555557fd8 is .dynamic
	0x0000555555557fd8 - 0x0000555555558000 is .got
	0x0000555555558000 - 0x0000555555558020 is .got.plt
	0x0000555555558020 - 0x0000555555558030 is .data
	0x0000555555558030 - 0x0000555555558038 is .bss
(gdb) delete breakpoints
Delete all breakpoints? (y or n) y
(gdb) info files 
Symbols from "/tmp/a/a.out".
Native process:
	Using the running image of child process 25625.
	While running this, GDB does not access memory from...
Local exec file:
	`/tmp/a/a.out', file type elf64-x86-64.
	Entry point: 0x555555555040
	0x00005555555542a8 - 0x00005555555542c4 is .interp
	0x00005555555542c4 - 0x00005555555542e4 is .note.ABI-tag
	0x00005555555542e8 - 0x0000555555554318 is .hash
	0x0000555555554318 - 0x0000555555554334 is .gnu.hash
	0x0000555555554338 - 0x00005555555543e0 is .dynsym
	0x00005555555543e0 - 0x0000555555554464 is .dynstr
	0x0000555555554464 - 0x0000555555554472 is .gnu.version
	0x0000555555554478 - 0x0000555555554498 is .gnu.version_r
	0x0000555555554498 - 0x0000555555554558 is .rela.dyn
	0x0000555555554558 - 0x0000555555554570 is .rela.plt
	0x0000555555555000 - 0x000055555555501b is .init
	0x0000555555555020 - 0x0000555555555040 is .plt
	0x0000555555555040 - 0x0000555555555215 is .text
	0x0000555555555218 - 0x0000555555555225 is .fini
	0x0000555555556000 - 0x0000555555556016 is .rodata
	0x0000555555556018 - 0x0000555555556054 is .eh_frame_hdr
	0x0000555555556058 - 0x0000555555556150 is .eh_frame
	0x0000555555557dd8 - 0x0000555555557de0 is .init_array
	0x0000555555557de0 - 0x0000555555557de8 is .fini_array
	0x0000555555557de8 - 0x0000555555557fd8 is .dynamic
	0x0000555555557fd8 - 0x0000555555558000 is .got
	0x0000555555558000 - 0x0000555555558020 is .got.plt
	0x0000555555558020 - 0x0000555555558030 is .data
	0x0000555555558030 - 0x0000555555558038 is .bss
(gdb) break *0x555555555040
Breakpoint 2 at 0x555555555040
(gdb) run
The program being debugged has been started already.
Start it from the beginning? (y or n) y
Starting program: /tmp/a/a.out 

Breakpoint 2, 0x0000555555555040 in _start ()
```

