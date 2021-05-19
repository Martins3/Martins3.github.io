# Redox

[官方文档](https://doc.redox-os.org/book/ch01-01-welcome.html)

调试的方法:
https://gitlab.redox-os.org/redox-os/kernel/-/tree/master


## 如何调用的 e1000 driver 的
如何进行这个:
https://gitlab.redox-os.org/redox-os/drivers

```c
cargo rustc --lib --target=/home/maritns3/core/ld/redox/kernel/targets/x86_64-unknown-none.json --release -Z build-std=core,alloc -- -C soft-float -C debuginfo=2 -C lto --emit link=../build/libkernel.a
```

- [ ] 让人奇怪的地方在于，acpi 的只是解析出来了 IOAPIC 和 HPET 来, pcie 是怎么被探索出来的，
  - [ ] pcie 在 acpi 中对应的 table 是什么 ？

- [ ] 至少，需要截获所有的 acpi 的访问，才可以正确模拟

- [ ] 其实，还需要创建一个虚假的 acpi 给 x86

- [ ] 除了 acpi 之外，还有什么是 firmware 传递给操作系统的?
  - [ ] efi 吗?

- [ ] mmio 空间分配是怎么实现的？

- [ ] 也许阅读一下
