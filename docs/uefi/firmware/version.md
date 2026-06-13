## 其他
### mpt3sas 的两个固件版本信息

```txt
[    3.228243] mpt3sas_cm0: FW Package Version(13.17.03.05)
[    3.229457] mpt3sas_cm0: LSISAS3008: FWVersion(13.15.02.00), ChipRevision(0x02)
```

的确有两个获取的方法:
```
_base_display_fwpkg_version()
  ├── dma_alloc_coherent()           // 分配 DMA 缓冲区
  ├── mpt3sas_base_get_msg_frame()   // 获取消息帧
  ├── ioc->put_smid_default()        // 发送请求
  │     └── _base_writeq()           // MMIO 写入 0xC0
  ├── wait_for_completion_timeout()  // 等待 DMA 完成
  └── le32_to_cpu(fw_img_hdr->PackageVersion.Word)
```

```
_base_get_ioc_facts()
  ├── _base_handshake_req_reply_wait()
  │     ├── base_readl(Doorbell)     // 检查 Doorbell 状态
  │     ├── writel(Doorbell)         // 发送握手
  │     ├── _base_spin_on_doorbell_int() // 等待中断
  │     └── base_readl(Doorbell)     // 读取响应
  └── le32_to_cpu(mpi_reply.FWVersion.Word)

_base_display_ioc_capabilities()
  └── 打印 FWVersion
```

## 如何获取到 firmware 版本
<!-- 2a93fd88-4c4e-40e7-8fea-7f1f5b60c600 -->

### 网卡: ethtool
```txt
🧀  ethtool -i enp4s0
driver: r8169
version: 6.18.5-100.fc42.x86_64
firmware-version: rtl8125b-2_0.0.2 07/13/20
expansion-rom-version:
bus-info: 0000:04:00.0
supports-statistics: yes
supports-test: no
supports-eeprom-access: no
supports-register-dump: yes
supports-priv-flags: no
```

### nvme 盘
```txt
🤒  cat /sys/devices/pci0000:00/*/*/nvme/*/firmware_rev
ZTA32F46
ZTA22001
SN12237
```
也可以用 sudo nvme id-ctrl /dev/nvme0

### CPU microcode

```txt
cat /proc/cpuinfo | grep -m 1 microcode
```

### GPU
```sh
nvidia-smi -q | grep -i vbios
```

其他的基本不出问题，就不用管了:

### HBA

#### mpt3sas
```txt
# lspci -s 0000:3b:00.0
3b:00.0 Serial Attached SCSI controller: LSI Logic / Symbios Logic SAS3008 PCI-Express Fusion-MPT SAS-3 (rev 02)

# cat /sys/devices/pci0000:3a/0000:3a:00.0/0000:3b:00.0/host5/scsi_host/host5/version_fw
16.00.11.00
```

有问题的版本
```txt
cat /sys/devices/pci0000:00/0000:00:16.0/0000:0b:00.0/host6/scsi_host/host6/version_fw
13.15.02.00
```

#### megaraid

```txt
/opt/MegaRAID/storcli/storcli64 /c0 show
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
