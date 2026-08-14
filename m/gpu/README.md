## 如何实现 GPU 和 PCIe ，就是这里
https://news.ycombinator.com/item?id=41085713

- https://blog.davidv.dev/posts/learning-pcie/
    - https://github.com/DavidVentura/pci-device

- https://xillybus.com/tutorials/pci-express-tlp-pcie-primer-tutorial-guide-1

## TODO
- 手动触发中断
- 中断的申请流程

2. 理解下 devmem 的操作的流程
```txt
sudo devmem 0xfb000000 16 4
sudo devmem 0xfb000000 16
```

3. 这个设备是需要看看的 : hw/misc/pci-testdev.c

4. 图形的展示效果是需要看看的

6. 每一个文章最后的链接都是需要看看的

5. 最后通过这个总结下
- https://github.com/luizinhosuraty/pciemu

## 观察一下 mod probe 的工作

## 用这个测试下 pci 框架

1. 测试 devm_kzalloc


## 等待测试的项目
1. 测试 tasklet
2. 测试 softirq (各种 lock 的 bh)
3. 测试 modprobe 的探测过程
4. 测试 interrupt thread
    - interrupt thread 自动为 qemu 吗?
5. 测试 msi

## 测试 local_bh_disable 的使用
使用 virtio-dummy ，勉强还行，但是有没有更加纯粹一点的

## softirq 可以被 perf 吗?

## softirq 可以中不能 pr_info 吗?

## softirq 可以 stacount 来 backtrace 吗?

## in_serving_softirq 和 in_softirq 啥关系来着 ?

## 如何理解 kernel 中这些的启动的参数?
video=VGA-1:1024x768-32@60,VGA-1:640x480-32@60me

## 是不是该多买点显卡做这些测试?


## 这个人写了一系列的 blog ，也是值得看的
https://mp.weixin.qq.com/s/Xs-iB1tOMpzYKJuFRbfG0Q
