# aarch64

### 顶层
- drivers/iommu/arm/arm-smmu/
- drivers/iommu/arm/arm-smmu-v3
- drivers/iommu/apple-dart.c


```c
static const struct iommu_ops apple_dart_iommu_ops ;
```
定义 : apple_dart_map_pages

### page table

- drivers/iommu/io-pgtable-arm.c
- drivers/iommu/io-pgtable-dart.c : 基本和 drivers/iommu/io-pgtable-arm.c 相同，for apple


## 文档
- https://developer.arm.com/documentation/109242/0100/Operation-of-an-SMMU/Translation-process-overview?lang=en

## openeuler 的文档
- https://www.openeuler.org/zh/blog/wxggg/2020-11-21-iommu-smmu-intro.html


## smmu page table 解释地很清晰
- https://www.cnblogs.com/zshiter/articles/17609477.html


## TODO
- 看看这个解释:
  - https://gitee.com/openeuler/kernel/issues/I8RJAT?from=project-issue

## 如何知道当前的 smmu 用的是那个版本 v3

在 apple 中:
- /sys/bus/platform/drivers/apple-dart

```txt
# ls -la /sys/bus/platform/drivers/arm-smmu
total 0
drwxr-xr-x  2 root root     0 Jun 12 16:35 .
drwxr-xr-x 70 root root     0 Jun 12 16:35 ..
lrwxrwxrwx  1 root root     0 Jun 12 16:57 arm-smmu.0.auto -> ../../../../devices/platform/arm-smmu.0.auto
lrwxrwxrwx  1 root root     0 Jun 12 16:57 arm-smmu.1.auto -> ../../../../devices/platform/arm-smmu.1.auto
```

## 参考一下这个
https://github.com/wangzhou/notes/blob/master/iommu/PASID_analysis


## 卧龙凤雏 kunpeng + megaraid_sas

第一次尝试:
如果让 megaraid_sas 开机就绑定到 vfio 上
```txt
echo "options vfio-pci ids=8086:158b" | sudo tee /etc/modprobe.d/vfio.conf
echo "add_drivers+=\" vfio vfio-pci \"" | sudo tee /etc/dracut.conf.d/vfio.conf
sudo dracut -f
```
然后就无法开机，不断的抛出错误，似乎不难理解，就是这个 arm iommu 和 megaraid 卡不兼容
```txt
[  420.979049] arm-smmu-v3 arm-smmu-v3.4.auto: event: F_TRANSLATION client: 0000:87:00.0 sid: 0x8700 ssid: 0x0 iova: 0x272f7900 ipa: 0x272f7000
[  421.129512] arm-smmu-v3 arm-smmu-v3.4.auto: unpriv data write s1 "Input address caused fault" stag: 0x0
[  421.241596] arm-smmu-v3 arm-smmu-v3.4.auto: event 0x10 received:
[  421.313241] arm-smmu-v3 arm-smmu-v3.4.auto:  0x0000870000000010
[  421.383856] arm-smmu-v3 arm-smmu-v3.4.auto:  0x0000020000000000
[  421.454470] arm-smmu-v3 arm-smmu-v3.4.auto:  0x00000000272f7800
[  421.525091] arm-smmu-v3 arm-smmu-v3.4.auto:  0x00000000272f7000
```

第二次尝试: 让不要加载
```txt
echo "blacklist megaraid_sas" | sudo tee /etc/modprobe.d/blacklist-megaraid.conf
sudo dracut -f -v
```

```txt
sudo lsinitrd /boot/initramfs-6.17.1-300.fc43.aarch64.img | grep mega
-rw-r--r--   1 root     root           23 Oct 14 08:00 etc/modprobe.d/blacklist-megaraid.conf
-rw-r--r--   1 root     root         3240 Oct  6 08:00 usr/lib/modules/6.17.1-300.fc43.aarch64/kernel/drivers/hid/hid-megaworld.ko.xz
drwxr-xr-x   2 root     root            0 Oct 14 08:00 usr/lib/modules/6.17.1-300.fc43.aarch64/kernel/drivers/scsi/megaraid
-rw-r--r--   1 root     root       100912 Oct  6 08:00 usr/lib/modules/6.17.1-300.fc43.aarch64/kernel/drivers/scsi/megaraid/megaraid_sas.ko.xz
```

第三次尝试: 直接移除掉 megaraid 驱动

```txt
sudo rm /usr/lib/modules/6.17.1-300.fc43.aarch64/kernel/drivers/scsi/megaraid/megaraid_sas.ko.xz
sudo depmod
sudo dracut -f -v
```

也没有作用，这个错误和驱动没有关系，总是会报错的:
```txt
[  128.670689] pci 0000:87:00.0: AER: can't recover (no error_detected callback)
[  128.755886] pci 0000:80:10.0: AER: device recovery failed
[  128.820289] pci 0000:80:10.0: aer_status: 0x00008000, aer_mask: 0x04500000
[  128.902303] pci 0000:80:10.0:    [15] CmpltAbrt              (First)
[  128.978106] pci 0000:80:10.0: aer_layer=Transaction Layer, aer_agent=Completer ID
[  129.067381] pci 0000:80:10.0: aer_uncor_severity: 0x00462030
[  129.134888] pci 0000:80:10.0: AER:   TLP Header: 0x40000010 0x870000ff 0x27af9700 0x00000000
```
## arm iommu 也许可以通过 ai 快速掌握吧

oe 的 workaround 技术就是这个了:
```c
static void arm_smmu_install_bypass_ste_for_dev(struct arm_smmu_device *smmu,
				    u32 sid)
{
	u64 val;
	__le64 *step = arm_smmu_get_step_for_sid(smmu, sid);

	if (!step)
		return;

	val = STRTAB_STE_0_V;
	val |= FIELD_PREP(STRTAB_STE_0_CFG, STRTAB_STE_0_CFG_BYPASS);
	step[0] = cpu_to_le64(val);
	step[1] = cpu_to_le64(FIELD_PREP(STRTAB_STE_1_SHCFG,
	STRTAB_STE_1_SHCFG_INCOMING));
	step[2] = 0;
}
```

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
