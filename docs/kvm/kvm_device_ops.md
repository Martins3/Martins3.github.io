## 通过 kvm_vfio_ops 来理解 kvm_device_ops
<!-- 584de1c4-fc23-4eae-8274-3b9b240c0064 -->

virt/kvm/vfio.c 就是定义了一个 kvm_vfio_ops 了而已，主要是资源释放？

只有 kvm_vfio_ops 被 kvm_device_ioctl 使用过:
```c
static struct kvm_device_ops kvm_vfio_ops = {
	.name = "kvm-vfio",
	.create = kvm_vfio_create,
	.destroy = kvm_vfio_destroy,
	.set_attr = kvm_vfio_set_attr,
	.has_attr = kvm_vfio_has_attr,
};
```

在 qemu 侧的使用:
- vfio_kvm_device_add_fd
  - kvm_vm_ioctl(kvm_state, KVM_CREATE_DEVICE, &cd) : 创建的 kvm_device
  - ioctl(vfio_kvm_device_fd, KVM_SET_DEVICE_ATTR, &attr) : 告诉 kvm_device, 内容是 attr 决定的

attr 一共三种行为:
```c
#define   KVM_DEV_VFIO_GROUP_ADD			1
#define   KVM_DEV_VFIO_GROUP_DEL			2
#define   KVM_DEV_VFIO_GROUP_SET_SPAPR_TCE		3
```
主要就是 GROUP_ADD 和 GROUP_DEL 了:

- kvm_vfio_set_attr
  - kvm_vfio_set_group
    - kvm_vfio_group_add
    - kvm_vfio_group_del

感觉最后就是: kvm_vfio_update_coherency ，这个感知不强:

第一个关键作用，关机的时候来释放 vfio 相关资源:

通过 KVM_DEV_VFIO_GROUP_ADD 动作，调用 kvm_vfio_file_add ，之后这个直通的设备就是和
这个 vfio device 关联的。

```txt
@[
        kvm_vfio_release+5
        kvm_device_release+105
        __fput+227
        task_work_run+89
        do_exit+483
        do_group_exit+48
        get_signal+2063
        arch_do_signal_or_restart+58
        exit_to_user_mode_loop+144
        do_syscall_64+431
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

看看 arm 的 gic 架构继续了解一下 kvm_device_ops 的作用吧，感觉 vfio 这个例子也不是非常合理的。

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
