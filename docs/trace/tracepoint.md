
## 简单分析下 kernel 的 tracepoint 如何工作

其中 xchg   %ax,%ax 就是 nop 指令:

```c
void this(void){
	trace_me_silly(0, 0);
}

int x;
int own(void){
	int i = 0;
	i += x;
	trace_me_silly(0, 0);
	i += x;
	return i;
}
```
当打开 tracepoint 的时候，是无需使用 int3 的，比较怀疑是直接的跳转就可以了。

```txt
Dump of assembler code for function this:
   0xffffffff81d7a870 <+0>:     endbr64
   0xffffffff81d7a874 <+4>:     xchg   %ax,%ax
   0xffffffff81d7a876 <+6>:     jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a87b <+11>:    mov    %gs:0x7e2b198a(%rip),%eax        # 0x2c20c <pcpu_hot+12>
   0xffffffff81d7a882 <+18>:    mov    %eax,%eax
   0xffffffff81d7a884 <+20>:    bt     %rax,0x1199ac4(%rip)        # 0xffffffff82f14350 <__cpu_online_mask>
   0xffffffff81d7a88c <+28>:    jae    0xffffffff81d7a8b7 <this+71>
   0xffffffff81d7a88e <+30>:    incl   %gs:0x7e2b1973(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a895 <+37>:    mov    0x11786e4(%rip),%rax        # 0xffffffff82ef2f80 <__tracepoint_me_silly+64>
   0xffffffff81d7a89c <+44>:    test   %rax,%rax
   0xffffffff81d7a89f <+47>:    je     0xffffffff81d7a8ae <this+62>
   0xffffffff81d7a8a1 <+49>:    mov    0x8(%rax),%rdi
   0xffffffff81d7a8a5 <+53>:    xor    %edx,%edx
   0xffffffff81d7a8a7 <+55>:    xor    %esi,%esi
   0xffffffff81d7a8a9 <+57>:    call   0xffffffff821f6dc8 <__SCT__tp_func_me_silly>
   0xffffffff81d7a8ae <+62>:    decl   %gs:0x7e2b1953(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a8b5 <+69>:    je     0xffffffff81d7a8bc <this+76>
   0xffffffff81d7a8b7 <+71>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a8bc <+76>:    call   0xffffffff821f41f0 <__SCT__preempt_schedule_notrace>
   0xffffffff81d7a8c1 <+81>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
```

```txt
di$ disass own
Dump of assembler code for function own:
   0xffffffff81d7a8e0 <+0>:     endbr64
   0xffffffff81d7a8e4 <+4>:     push   %rbx
   0xffffffff81d7a8e5 <+5>:     mov    0x1b9a2bd(%rip),%ebx        # 0xffffffff83914ba8 <x>
   0xffffffff81d7a8eb <+11>:    xchg   %ax,%ax
   0xffffffff81d7a8ed <+13>:    mov    %ebx,%eax
   0xffffffff81d7a8ef <+15>:    add    %ebx,%eax
   0xffffffff81d7a8f1 <+17>:    pop    %rbx
   0xffffffff81d7a8f2 <+18>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a8f7 <+23>:    mov    %gs:0x7e2b190e(%rip),%eax        # 0x2c20c <pcpu_hot+12>
   0xffffffff81d7a8fe <+30>:    mov    %eax,%eax
   0xffffffff81d7a900 <+32>:    bt     %rax,0x1199a48(%rip)        # 0xffffffff82f14350 <__cpu_online_mask>
   0xffffffff81d7a908 <+40>:    jae    0xffffffff81d7a933 <own+83>
   0xffffffff81d7a90a <+42>:    incl   %gs:0x7e2b18f7(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a911 <+49>:    mov    0x1178668(%rip),%rax        # 0xffffffff82ef2f80 <__tracepoint_me_silly+64>
   0xffffffff81d7a918 <+56>:    test   %rax,%rax
   0xffffffff81d7a91b <+59>:    je     0xffffffff81d7a92a <own+74>
   0xffffffff81d7a91d <+61>:    mov    0x8(%rax),%rdi
   0xffffffff81d7a921 <+65>:    xor    %edx,%edx
   0xffffffff81d7a923 <+67>:    xor    %esi,%esi
   0xffffffff81d7a925 <+69>:    call   0xffffffff821f6dc8 <__SCT__tp_func_me_silly>
   0xffffffff81d7a92a <+74>:    decl   %gs:0x7e2b18d7(%rip)        # 0x2c208 <pcpu_hot+8>
   0xffffffff81d7a931 <+81>:    je     0xffffffff81d7a941 <own+97>
   0xffffffff81d7a933 <+83>:    mov    0x1b9a26f(%rip),%eax        # 0xffffffff83914ba8 <x>
   0xffffffff81d7a939 <+89>:    add    %ebx,%eax
   0xffffffff81d7a93b <+91>:    pop    %rbx
   0xffffffff81d7a93c <+92>:    jmp    0xffffffff821f3544 <__x86_return_thunk>
   0xffffffff81d7a941 <+97>:    call   0xffffffff821f41f0 <__SCT__preempt_schedule_notrace>
   0xffffffff81d7a946 <+102>:   jmp    0xffffffff81d7a933 <own+83>
End of assembler dump.
```
