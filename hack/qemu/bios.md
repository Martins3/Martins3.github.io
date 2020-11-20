## x86-bare-metal-examples
项目地址 : https://github.com/cirosantilli/x86-bare-metal-examples

- [ ] 启动，从 bios 到 boot sector 到 coreboot 到 grub 到 linuz ?

### question
- [ ]  `make -C printf run`
```sh
qemu-system-i386 -drive file='$(MAIN)',format=raw
```
boot sector 存在标准格式的，bios 来执行这个代码。
那么在现代的 linux 中间，boot sector 是谁构建的。


