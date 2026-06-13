## 1. 可以继续好好利用 localmodcaonfig

scripts/kconfig/streamline_config.pl  解析

loop_depend

3. 如果依赖不存在，是否可以强制产生新配置
arch/x86/configs/x86_64_defconfig 为什么可以在没有 config 的时候，而其他的不可以

5. 有办法知道打开一个选项，需要打开多少个文件?

arch/x86/configs/x86_64_defconfig 一共有大概 3000 个文件

## 找到我们需要什么 config
```txt
nvme-core-y				+= core.o ioctl.o sysfs.o pr.o
```

例如 make defconfig 之后，

scripts/kconfig/streamline_config.pl 之后，
```txt
module overlay did not have configs CONFIG_OF_OVERLAY CONFIG_OVERLAY_FS
```

类似这些，其实很多 overlay.o 文件都是一样的，
```txt
overlay ['CONFIG_OF_OVERLAY', 'CONFIG_OVERLAY_FS']
msr ['CONFIG_X86_LOCAL_APIC', 'CONFIG_X86_MSR']
kvm ['CONFIG_KVM', 'CONFIG_KVM', 'CONFIG_KVM', 'CONFIG_KVM_GUEST', 'CONFIG_KVM_E500V2', 'CONFIG_KVM_E500MC', 'CONFIG_KVM_BOOK3S_64', 'CONFIG_KVM_BOOK3S_32', 'CONFIG_KVM', 'CONFIG_KVM', 'CONFIG_KVM_GUEST', 'CONFIG_KVM_X86', 'CONFIG_KVM_XFER_TO_GUEST_WORK']
btintel ['CONFIG_BT_INTEL', 'CONFIG_BT_INTEL_PCIE']
cec ['CONFIG_CEC_CORE', 'CONFIG_RAS_CEC']
watchdog ['CONFIG_PPC_WATCHDOG', 'CONFIG_WATCHDOG_CORE', 'CONFIG_LOCKUP_DETECTOR']
backlight ['CONFIG_PMAC_BACKLIGHT', 'CONFIG_BACKLIGHT_CLASS_DEVICE']
tpm ['CONFIG_TCG_TPM', 'CONFIG_EFI']
libahci ['CONFIG_SATA_AHCI', 'CONFIG_SATA_ACARD_AHCI', 'CONFIG_SATA_AHCI_SEATTLE', 'CONFIG_SATA_AHCI_PLATFORM', 'CONFIG_SATA_HIGHBANK', 'CONFIG_AHCI_BRCM', 'CONFIG_AHCI_CEVA', 'CONFIG_AHCI_DA850', 'CONFIG_AHCI_DM816', 'CONFIG_AHCI_DWC', 'CONFIG_AHCI_IMX', 'CONFIG_AHCI_MTK', 'CONFIG_AHCI_MVEBU', 'CONFIG_AHCI_SUNXI', 'CONFIG_AHCI_ST', 'CONFIG_AHCI_TEGRA', 'CONFIG_AHCI_XGENE', 'CONFIG_AHCI_QORIQ']
dax ['CONFIG_DAX', 'CONFIG_FS_DAX']
```
## 如果用 lsmod ，有点 module 是 built-in ，怎么办?

## 有时候，CONFIG 的依赖是可以知道的
给 n100 直接添加 streamline_config 的结果，会有如下错误:
```txt
WARNING: unmet direct dependencies detected for FB_IOMEM_HELPERS
  Depends on [n]: HAS_IOMEM [=y] && FB_CORE [=n]
  Selected by [m]:
  - DRM_XE_DISPLAY [=y] && HAS_IOMEM [=y] && DRM [=m] && DRM_XE [=m] && DRM_XE [=m]=m [=m] && HAS_IOPORT [=y]

WARNING: unmet direct dependencies detected for FB_IOMEM_HELPERS
  Depends on [n]: HAS_IOMEM [=y] && FB_CORE [=n]
  Selected by [m]:
  - DRM_XE_DISPLAY [=y] && HAS_IOMEM [=y] && DRM [=m] && DRM_XE [=m] && DRM_XE [=m]=m [=m] && HAS_IOPORT [=y]
```
如果这个时候，去打开 CONFIG_FB 就可以

但是 CONFIG_USB 显然就没有配置的

### 最后，还是需要 systemd 来检查一下

例如这个:
```txt
Mar 05 00:49:03 localhost systemd[1]: Started irqbalance.service - irqbalance daemon.
Mar 05 00:49:03 localhost (qbalance)[546]: irqbalance.service: PrivateNetwork=yes is configured, but the kernel does not support or we lack privileges for network namespace, proceeding without.
Mar 05 00:49:03 localhost (qbalance)[546]: irqbalance.service: ProtectHostname=yes is configured, but the kernel does not support UTS namespaces, ignoring namespace setup.
Mar 05 00:49:03 localhost (qbalance)[546]: irqbalance.service: Failed to set up user namespacing: Invalid argument
Mar 05 00:49:03 localhost systemd[1]: irqbalance.service: Main process exited, code=exited, status=217/USER
Mar 05 00:49:03 localhost systemd[1]: irqbalance.service: Failed with result 'exit-code'.
```

## 看看那个脚本说的这个东西
make modules install 的时候有这个东西:
```txt
kdump: For kernel=/boot/vmlinuz-6.14.2, crashkernel=1G-4G:192M,4G-64G:256M,64G-:512M now. Please reboot the system for the change to take effect. Note if you don't want kexec-tools to manage the crashkernel kernel parameter, please set auto_reset_crashkernel=no in /etc/kdump.conf.
```

1. 也许有用的的命令
- make listnewconfig
- scripts/diffconfig .config.old .config | less.
  - 这个可以用来看，哪些模块多了
2. pv mode 和 native mode 似乎是互斥的，真的可以一个 config ，不同的环境都使用吗?


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
