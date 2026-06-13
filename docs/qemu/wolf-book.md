## 代码分析

qemu/cpus.c
qemu/memory.c :
qemu/net/tap.c :

qemu/hw/virtio
qemu/linux-user
qemu/capstone/ : https://github.com/aquynh/capstone
- [ ] qemu/slirp : https://gitlab.freedesktop.org/slirp/libslirp 这个和普通的用户态网络库的关系是什么?

- [ ] qemu/stubs : 这个暂时搞不懂是什么意思

- [ ] 可以通过 kernel-irqchip 来决定 irqchip 被谁模拟

## chapter2


### QOM

1. 类型的注册
  - `type_init`
  - `register_module_init`
  - `type_register`
2. 类型的初始化
  - `type_initialize`
3. 对象的初始化
  - `object_new`
  - `object_initialize`
  - `object_initialize_with_type`

类型的注册
，在 main 之前完成，
类型的初始化, 在 main 中调用，全部初始化，
对象的初始化, 根据命令行参数选择进行初始化


从 edu.c 中间分析:
```c
typedef enum {
    MODULE_INIT_BLOCK,
    MODULE_INIT_OPTS,
    MODULE_INIT_QOM,
    MODULE_INIT_TRACE,
    MODULE_INIT_XEN_BACKEND,
    MODULE_INIT_LIBQOS,
    MODULE_INIT_MAX
} module_init_type;

void module_call_init(module_init_type type)
{
    ModuleTypeList *l;
    ModuleEntry *e;

    l = find_type(type);

    QTAILQ_FOREACH(e, l, node) {
        e->init();
    }
}
```
- [ ] 从 `module_init_type` 看，似乎一个类型(`module_init_type`)的都是在一起初始



## chapter 3 主板和固件模拟
> 草稿放到 qboot.md 和 kernel-img.md 中

QEMU 会调用 `rom_check_and_register_reset`, 其主要的工作是将 `rom_reset` 挂到 `reset_handler`
链表上，将虚拟机重置的时候，会调用这些函数。

## chapter 6

#### 6.2.2 PIC 中断模拟
- kvm_irqchip_create

kvm 模块在处理
KVM_CREATE_IRQCHIP
的时候，会调用
1. kvm_create_pic
2. kvm_ioapic_init
3. kvm_steup_default_irq_routing

## chapter 7 : 设备虚拟化

#### 7.6 vhost net 简介
virtio 的问题在于为了将数据发送出去，需要切入到用户态，然后走 TAP 设备，vsock 直接走内核，从而减少一次用户态。

- [ ] 为什么 block io 不是类似的处理

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
