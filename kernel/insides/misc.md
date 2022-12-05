# Program startup process in userspace
作者为了证明main 函数并不是程序的入口，使用gdb 的方法，但是实际上演示结果稍微存在不同的地方:
```
>>> info files
Symbols from "/tmp/a/a.out".
Local exec file:
        `/tmp/a/a.out', file type elf64-x86-64.
        Entry point: 0x1040
        0x0000000000000318 - 0x0000000000000334 is .interp
        0x0000000000000338 - 0x0000000000000358 is .note.gnu.property
        0x0000000000000358 - 0x000000000000037c is .note.gnu.build-id
        0x000000000000037c - 0x000000000000039c is .note.ABI-tag
        0x00000000000003a0 - 0x00000000000003c4 is .gnu.hash
        0x00000000000003c8 - 0x0000000000000458 is .dynsym
        0x0000000000000458 - 0x00000000000004d5 is .dynstr
        0x00000000000004d6 - 0x00000000000004e2 is .gnu.version
        0x00000000000004e8 - 0x0000000000000508 is .gnu.version_r
        0x0000000000000508 - 0x00000000000005c8 is .rela.dyn
        0x0000000000001000 - 0x000000000000101b is .init
        0x0000000000001020 - 0x0000000000001030 is .plt
        0x0000000000001030 - 0x0000000000001040 is .plt.got
        0x0000000000001040 - 0x00000000000011b5 is .text
        0x00000000000011b8 - 0x00000000000011c5 is .fini
        0x0000000000002000 - 0x0000000000002004 is .rodata
        0x0000000000002004 - 0x0000000000002040 is .eh_frame_hdr
        0x0000000000002040 - 0x0000000000002130 is .eh_frame
        0x0000000000003df0 - 0x0000000000003df8 is .init_array
        0x0000000000003df8 - 0x0000000000003e00 is .fini_array
        0x0000000000003e00 - 0x0000000000003fc0 is .dynamic
        0x0000000000003fc0 - 0x0000000000004000 is .got
        0x0000000000004000 - 0x0000000000004010 is .data
        0x0000000000004010 - 0x0000000000004018 is .bss
>>> break 123
No line 123 in the current file.
Make breakpoint pending on future shared library load? (y or [n]) n
>>> break *(0x1030)
Breakpoint 1 at 0x1030
>>> run
Starting program: /tmp/a/a.out
Warning:
Cannot insert breakpoint 1.
Cannot access memory at address 0x1030

>>> info files
Symbols from "/tmp/a/a.out".
Native process:
        Using the running image of child process 26838.
        While running this, GDB does not access memory from...
Local exec file:
        `/tmp/a/a.out', file type elf64-x86-64.
        Entry point: 0x555555555040
        0x0000555555554318 - 0x0000555555554334 is .interp
        0x0000555555554338 - 0x0000555555554358 is .note.gnu.property
        0x0000555555554358 - 0x000055555555437c is .note.gnu.build-id
        0x000055555555437c - 0x000055555555439c is .note.ABI-tag
        0x00005555555543a0 - 0x00005555555543c4 is .gnu.hash
        0x00005555555543c8 - 0x0000555555554458 is .dynsym
        0x0000555555554458 - 0x00005555555544d5 is .dynstr
        0x00005555555544d6 - 0x00005555555544e2 is .gnu.version
        0x00005555555544e8 - 0x0000555555554508 is .gnu.version_r
        0x0000555555554508 - 0x00005555555545c8 is .rela.dyn
        0x0000555555555000 - 0x000055555555501b is .init
        0x0000555555555020 - 0x0000555555555030 is .plt
        0x0000555555555030 - 0x0000555555555040 is .plt.got
        0x0000555555555040 - 0x00005555555551b5 is .text
        0x00005555555551b8 - 0x00005555555551c5 is .fini
        0x0000555555556000 - 0x0000555555556004 is .rodata
        0x0000555555556004 - 0x0000555555556040 is .eh_frame_hdr
        0x0000555555556040 - 0x0000555555556130 is .eh_frame
        0x0000555555557df0 - 0x0000555555557df8 is .init_array
        0x0000555555557df8 - 0x0000555555557e00 is .fini_array
        0x0000555555557e00 - 0x0000555555557fc0 is .dynamic
        0x0000555555557fc0 - 0x0000555555558000 is .got
        0x0000555555558000 - 0x0000555555558010 is .data
        0x0000555555558010 - 0x0000555555558018 is .bss
>>>
```
