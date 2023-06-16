
## 启动 qemu
不懂，为什么启动的时候还是 unmap
```txt
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+258
    vfio_remove_dma+42
    vfio_iommu_type1_ioctl+2815
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 76685
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+340
    vfio_remove_dma+42
    vfio_iommu_type1_ioctl+2815
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 814635
```

## 关闭 qemu
```txt
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+340
    vfio_remove_dma+42
    vfio_iommu_type1_detach_group+1531
    vfio_group_detach_container+80
    vfio_group_fops_release+72
    __fput+134
    task_work_run+90
    do_exit+834
    do_group_exit+49
    get_signal+2440
    arch_do_signal_or_restart+62
    exit_to_user_mode_prepare+415
    syscall_exit_to_user_mode+27
    do_syscall_64+74
    entry_SYSCALL_64_after_hwframe+114
]: 266265
```
