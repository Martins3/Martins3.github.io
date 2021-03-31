## x86-bare-metal-examples
项目地址 : https://github.com/cirosantilli/x86-bare-metal-examples

*暂时停止一下，其实还有很多东西可以挖掘*

- [ ] 如何编译链接细节
- [ ] 在自己的 ThinkPad T450 上安装一下
- [ ] ./run bios_hello_world debug 如何实现 debug 的


- 默认编译出来的汇编代码都是 ELF 格式的，ELF 格式中间持有的信息可以让内核和 linker loader 分析，但是运行在 Bios 上的代码是不需要 ELF 信息处理的，所以需要[objcopy](https://stackoverflow.com/questions/19944441/make-executable-binary-file-from-elf-using-gnu-objcopy) 将 ELF 信息去掉。
- BIOS 将 MBR（disk 的第一个 sector） 中间的代码加载到 0x7c00，所以需要使用 loader 调整编译出来的代码(当然如果不访存，是可以不用的)

BIOS 主要提供了显示，键盘，disk 和 memory, pci 等功能[^6]，内存探测使用 int 15 下 e820 中断。[^7]

- [ ] dmidecode

- [ ] https://github.com/cirosantilli/x86-bare-metal-examples#no-bios-hello-world 中间直接向 0xb800 的位置写入数值，从而绕过 bios 提供的 int
  - [ ] 这些位置是谁规定的 ? 和 MMIO 的关系相同吗 ?


- [ ] Timer
  - [x] rtc.S : 0x70 端口
  - linux 使用的是
  - [ ] pit 和 

- [ ] UEFI 作者也没有搞定，所以 seabios , coreboot 和 tianocore 的关系是什么 ?

### grub
- chainloader : x86-bare-metal-examples/grub/chainloader 下编译好之后，[gnome-disk-image-mounter](https://askubuntu.com/questions/69363/mount-single-partition-from-image-of-entire-disk-device/673257#673257) 查看 main.img 可以看到 grub 构建的文件系统。
  - chainloader 要求放进去的是一个没有格式的文件系统，这就是为什么 grub 是可以启动 Windows 的原因。
- multiboot : 可以直接运行 elf 格式的文件

### question
- [x]  `make -C printf run`
```sh
qemu-system-i386 -drive file='$(MAIN)',format=raw
```
boot sector 存在标准格式的，bios 来执行这个代码。
那么在现代的 linux 中间，bios 会从 partition 的第一个 sector, usb , 以及 网卡中间寻找 bootloader[^1],
这个 bootloader 就是 grub, bootloader 负责将 kernel 和 initramdisk 加载到内核中间，而 initramdisk 的工作就是将挂在 /，
可以利用 buildroot 来制作 initramdisk[^2], 猜测 initramdisk 包含了各种读去文件系统的驱动了(检查 /boot/initrd.img-5.4.0-48-generic 的大小，为 51.2M, 所以包含 disk 的驱动是不难的)

- [x] grub 如果负责 linux 加载，他靠什么读 disk ? 应该是 bios 中间的
- [x] coreboot 是什么定位 ? BIOS / UEFI 的开源替代品 [^3]
- [x] initrd / initramdisk 是做什么 ? 猜测是，靠 grub 加载进来的，在内存的文件系统，然后内核靠他将真正的文件系统 mount 进来
    - 但是此时还是没有 驱动 ？

多个 partition 都可以作为 boot sector 是因为 disk 的第一个 sector 存储 MBR[^1] 


- [ ] `make -C printf run` : 如果将 
```
	# printf '\125\252' >> '$(MAIN)'
	printf '\x55\xAA' >> '$(MAIN)'
```
替换为，然后就启动失败了。

- [ ] /home/maritns3/core/linux/arch/x86/boot/setup.ld 是做什么的，想必还有一个 loader script

- [ ] common.S 中间的 Local 找不到对应的 manual, 比如如下代码，虽然知道是为了定义局部 label, 但是为什么不像其他的写成 .local
```asm
/* Convert the low nibble of a r8 reg to ASCII of 8-bit in-place.
 * reg: r8 to be converted
 * Output: stored in reg itself. Letters are uppercase.
 */
.macro HEX_NIBBLE reg
    LOCAL letter, end
    cmp $10, \reg
    jae letter
    add $'0, \reg
    jmp end
letter:
    /* 0x37 == 'A' - 10 */
    add $0x37, \reg
end:
.endm
```
- [x] x86 不可以使用 mov pop 之类的指令设置 cs，只能使用 ljmp, 比如在 common.h 的 `BEGIN` 中间[^5], 猜测其中的原因是，实际上的 ip 总是 cs + ip 的，所以使用 `ljmp $0, $1f` 其实是设置了 cs 并且自动设置了 ip 寄存器。

- [x] `BEGIN` 为什么要单独保存好 dl 急促器在 stage 2 使用, bios 提供了什么不得了东西吗 ?
  - x86-bare-metal-examples/bios_disk_load.S 和  common.h 的 STAGE2，所模拟的 bios 将 driver 设置为 0x80
  - x86-bare-metal-examples/bios_initial_state.S 首先将各种寄存器的 initstate 保存下来，然后打印出来，可以看到，只有 dl = 0x80, 其他都是 0

- [ ] ./c_hello_world
  - [ ] linker.ld  : 里面两个 stackoverflow
  - [ ] main.c 为什么将 s[i] 做 mask ? / 
  - [ ] objdump 看看内容
  - [ ] bootloader 和 cpp :  http://3zanders.co.uk/2017/10/18/writing-a-bootloader3/

###  nasm
x86-bare-metal-examples/nasm 下:
- [ ] 感受一下 nasm 的语法 : https://medium.com/@g33konaut/writing-an-x86-hello-world-boot-loader-with-assembly-3e4c5bdd96cf




[^1]: https://wiki.ubuntu.com/Booting
[^2]: https://gist.github.com/chrisdone/02e165a0004be33734ac2334f215380e
[^3]: https://en.wikipedia.org/wiki/Coreboot
[^4]: https://www.glamenv-septzen.net/en/view/6
[^5]: https://stackoverflow.com/questions/48237933/after-load-the-gdt
[^6]: https://en.wikipedia.org/wiki/BIOS_interrupt_call#Interrupt_table
[^7]: https://en.wikipedia.org/wiki/E820
