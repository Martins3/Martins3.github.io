# Ucore Lab One

## 文档地址
https://github.com/chyyuu/ucore_os_docs


#### Understand the X86
https://en.wikibooks.org/wiki/Category:Book:X86_Assembly
https://en.wikibooks.org/wiki/X86_Assembly/Bootloaders

> what is the meaning of `.set` and so on, where can I learn this systematically.

#### A20 Gate
大体来说，
为了向下兼容，所以使用A20 Gate
为了省钱，所以使用键盘控制器
为了防止缓冲区中间还有数据，所以需要等待一下
为了实现切换，所以需要打开A20 Gate

> This is most stupid thing I ever seen

只能访问0-1M、2-3M、4-5M......，这样无法有效访问所有可用内存.
只有一根线A20被屏蔽，而不是A20-A23四根线全部被屏蔽，所以什么叫做 memory wrapping, 为什么只是处理一根线，
这种垃圾属性为什么需要向下兼容。

理论上讲，我们只要操作8042芯片的输出端口（64h）的bit 1，就可以控制A20 Gate，但实际上，当你准备向8042的输入缓冲区里写数据时，可能里面还有其它数据没有处理，所以，我们要首先禁止键盘操作，同时等待数据缓冲区中没有数据以后，才能真正地去操作8042打开或者关闭A20 Gate。


#### crosscompiler
https://github.com/cfenollosa/os-tutorial/tree/master/11-kernel-crosscompiler


#### protected mode
实模式下：`地址值=段值：偏移值= 段值*0x10+偏移值((段值<<4)+偏移值)`
保护模式下：`地址值=选择子：偏移值=描述符表中第(选择子>>3)项描述符给出的段基址+偏移值`

#### readseg
1. ELF文件的格式到底是什么样子的?
2. 


#### 练习1：理解通过make生成执行文件的过程

##### Notes
1. make的四种赋值
https://stackoverflow.com/questions/448910/what-is-the-difference-between-the-gnu-makefile-variable-assignments-a

2. 为什么需要eliminate default suffix rules, `.SUFFIXS`的作用是什么?
`.c.o` 是`%.o: %.c`的old-fashioned style.
https://stackoverflow.com/questions/2010926/what-does-o-suffixes-in-makefile-mean
`Suffix rules` are rules of the form .a.b
the `.SUFFIXES` "target" is a way to define which suffixes you can use in your suffix rules

3. SecondaryExpansion

4. [What does $$@ and the pipe symbol in Makefile stand for?](https://stackoverflow.com/questions/12299369/what-does-and-the-pipe-symbol-in-makefile-stand-for)
https://www.gnu.org/software/make/manual/make.html#Prerequisite-Types

##### PinPoint
the overall framework is generate four targets as dir `bin` suggest.
let's first check the how the makefile give birth `assign`


Let me guess the machinsim of `eval` and `call`

```
# get all the objs
$(call add_files_host,tools/sign.c,sign,sign)
  |
  |----add_files_host = $(call add_files,$(1),$(HOSTCC),$(HOSTCFLAGS),$(2),$(3))
        |
        |-----add_files = $(eval $(call do_add_files_to_packet,$(1),$(2),$(3),$(4),$(5)))
                |
                |---define do_add_files_to_packet
                    __temp_packet__ := $(call packetname,$(4)) # change packet to __objs_packet
                    ifeq ($$(origin $$(__temp_packet__)),undefined) # 
                    $$(__temp_packet__) :=
                    endif
                    __temp_objs__ := $(call toobj,$(1),$(5))
                    $$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(5))))
                    $$(__temp_packet__) += $$(__temp_objs__)
                    endef

# get target from objs
$(call create_target_host,sign,sign)
  |
  |----create_target_host = $(call create_target,$(1),$(2),$(3),$(HOSTCC),$(HOSTCFLAGS))
        |
        |----create_target = $(eval $(call do_create_target,$(1),$(2),$(3),$(4),$(5)))
              |
              |---# add packets and objs to target (target, #packes, #objs[, cc, flags])
                  define do_create_target
                  __temp_target__ = $(call totarget,$(1))
                  __temp_objs__ = $$(foreach p,$(call packetname,$(2)),$$($$(p))) $(3)
                  TARGETS += $$(__temp_target__)
                  ifneq ($(4),)
                  $$(__temp_target__): $$(__temp_objs__) | $$$$(dir $$$$@)
                    $(V)$(4) $(5) $$^ -o $$@
                  else
                  $$(__temp_target__): $$(__temp_objs__) | $$$$(dir $$$$@)
                  endif
                  endef
```

We have to understand functions:
```
# change $(name) to $(OBJPREFIX)$(name): (#names)
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))


totarget = $(addprefix $(BINDIR)$(SLASH),$(1))
```

#### 练习2：使用qemu执行并调试lab1中的软件


#### 练习3：分析bootloader进入保护模式的过程
Real mode : 1M space, no protect, no map and 通过修改A20地址线可以完成从实模式到保护模式的转换
Protect mode :

#### 练习6：完善中断初始化和处理
what is segment selector
https://www.oreilly.com/library/view/understanding-the-linux/0596005652/ch04s04.html



## Adventures

#### Fix `elf32-i386-gcc`
If make the lab1_result, Makefile may complains as follow:
```
+ ld bin/bootblock
'obj/bootblock.out' size: 604 bytes
604 >> 510!!
make: *** [Makefile:157: bin/bootblock] Error 255
```
so we should understand 
```
ifndef GCCPREFIX
GCCPREFIX := $(shell if i386-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
// more code
endif
```

1. `> /dev/null  2>&1`
https://unix.stackexchange.com/questions/163352/what-does-dev-null-21-mean-in-this-article-of-crontab-basics
https://stackoverflow.com/questions/818255/in-the-shell-what-does-21-mean

2. `-m32`
https://github.com/cfenollosa/os-tutorial/tree/master/11-kernel-crosscompiler
https://stackoverflow.com/questions/448457/how-to-use-multiple-versions-of-gcc

3. maybe we need to install multiple version of gcc

Follow the `os-tutorial`, compile binutils get following error:
```
configure: error: source directory already configured;
```
Read the readme, just configre and make in the source code dir instead mkdir a new `binutils-build` dir


## bios 完成的内容是什么
1. 到底是什么被加载到 0x7c00 中间



## 和kernel 对照
1. kernel 的编译系统
2. ucore 中间的启动阶段 被划分为两个部分: boot 和 init , start_kernel 

#### boot/bootasm.h 头文件在Makefile 中间是如何被编译的，单独抽出来编译一下

#### 运行qemu 的方法是什么

#### 从bootasm.S 开始 qemu debug ?
1. bios 把代码加载到 0x7c00 的时候，从qemu 中间找到证据 ?
2. 在内核中间重新操作一遍, 确定grub 加载的物理地址 ?

3. bootmain 将kernel image 读入 到前面

#### 内核在什么位置打开A20线

#### 真正恐怖的地方 init_console
1. pic
2. serial
3. keyboard

pic_enable :中断控制器
cga_init 中间看到了 cga_init 函数，看到了内存映射的内容，nemu 中间显存似乎就是写入到硬件中间的啊!

```c
/* cons_putc - print a single character @c to console devices */
void cons_putc(int c) {
  lpt_putc(c);  // @todo 为什么输出需要输出到lpt和serial 输出一下 VGA 的确使用了显存的方法，但是为什么规定在哪里，并不知道!
  cga_putc(c);
  serial_putc(c);
}
```

#### 时钟中断是如何被触发的
时钟中断，中断到PIC PIC触发idt 中间事先注册程序，注意，硬件不负责保存现场，所以idt 中间含有保存现场的内容

trapentry.S 不就是 entry_64.S 对应的吗 ?

#### TSS
https://en.wikipedia.org/wiki/Task_state_segment#TSS_in_x86-64_mode
stack, interrupt stack and io permission

@todo 扩展试验不做就是自己骗自己

@todo 链接脚本中间将调试信息放到内存中间的操作，如果不添加，会有什么效果

http://www.jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html 可能辅助设置扩展问题
