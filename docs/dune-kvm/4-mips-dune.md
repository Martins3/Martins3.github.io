# Port dune to mips

- [ ] 如何交叉编译从而可以让我在 x86 的电脑上看内核 ?

```c
ARCH=mips64 CROSS_COMPILE=aarch64-linux-gnu- make
ARCH=mips64 CROSS_COMPILE=aarch64-linux-gnu- make defconfig
```
