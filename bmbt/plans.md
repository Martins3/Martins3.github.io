# Bare Mental Binary Translator

## 需要解决的问题
- [ ] 正确的编程模式:
  - https://includeos.readthedocs.io/en/latest/Features.html
    - CPU 核的数量一样多的线程

- [ ] 创建一个只有只需要最少驱动的环境:
  - 内存管理
  - uart 设备 / ejtag / 串口 / 显示设备

#### 基础环境搭建

## 可以借鉴的代码
- [ ] captive 
   - IncludeOS 中间应该有类似的东西, 不然 C++ 的 new 都是需要自己重写的

- [x] 找打更多的 IncludeOS, 那种可以直接在硬件上运行的那种
    - 笑死，根本没有[^1]
- [ ] Qemu 处理硬件的方法
  - [ ] 除了 PCI 驱动，还存在什么驱动 ?
    - 比如 x86 的中断控制器 ?
        - [ ] 其实也不是很难，让所有的设备走模拟，使用 qemu 来观测一下 x86 的运行即可, 之后的操作也是可以按照这个进行
    - 怀疑，虽然很多设备都是 pci 设备，但是还是映射出来一堆空间来实现真正的操作, 这些操作都是需要模拟的
  - [ ] 不用 virtio 运行一个内核试试?

## 想法
- 应该首先放到虚拟机中间测试才对的啊，按道理如果可以在 bm 上跑起来，那么必然需要在虚拟机上跑起来的
  - [ ] 首先写一个 unikernel 出来，让 loongarch 的 qemu 可以运行才可以
  - [ ] 制作一个 C 语言的版本的 InlcudeOS，是不是会轻松很多 ?
    - [ ] 既然 captive / includeos 都是支持 c++ 的，而且不知道以后会不会使用 LLVM 的啊!

- [x] includeos 的 hypervisor 在什么地方，为什么可以脱离 qemu 运行啊 ?
  - 既然如此，那么就不需要 qemu 也是可以知道如何运行 unikernel 了
  - 如果，加入，让 unikernel 支持了各种 posix 系统调用, 那么将整个 qemu 放到上面也不是不可能的操作
  - 这个东西居然使用的是 qemu 运行的 : https://github.com/includeos/vmrunner/blob/master/bin/boot

[^1]: https://github.com/cetic/unikernels
