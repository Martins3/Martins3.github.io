## drivers/base/firmware_loader 通用框架加载驱动
<!-- a670ebd1-e42d-4297-8a1c-edfa6412fb01 -->

```txt
drivers/base/firmware_loader/
├── main.c              # 核心固件加载逻辑
├── firmware.h          # 内部头文件，定义 fw_priv, fw_state 等结构
├── fallback.c          # sysfs fallback 机制（用户空间辅助加载）
├── fallback.h          # fallback 头文件
├── fallback_platform.c # 平台固件 fallback（EFI embedded firmware）
├── fallback_table.c    # fallback 表
├── sysfs.c/sysfs.h     # sysfs 接口
├── sysfs_upload.c/h    # 固件上传接口
├── builtin/            # 内置固件支持
│   └── main.c          # 内置固件加载逻辑
├── Makefile
└── Kconfig
```

固件加载机制提供以下主要 API（定义在 `include/linux/firmware.h`）：

| API                           | 说明                  |
|-------------------------------|-----------------------|
| `request_firmware()`          | 同步请求固件          |
| `request_firmware_nowait()`   | 异步请求固件          |
| `request_firmware_direct()`   | 直接加载，无 fallback |
| `request_firmware_into_buf()` | 加载到指定缓冲区      |
| `release_firmware()`          | 释放固件              |
| `firmware_request_builtin()`  | 请求内置固件          |

```
┌─────────────────────────────────────────────────────────────┐
│                     固件加载流程                             │
├─────────────────────────────────────────────────────────────┤
│ 1. 内置固件查找                                             │
│    └── 检查 CONFIG_EXTRA_FIRMWARE 编译进内核的固件          │
│                                                             │
│ 2. 文件系统查找（按优先级顺序）                              │
│    ├── /lib/firmware/updates/UTS_RELEASE                   │
│    ├── /lib/firmware/updates                               │
│    ├── /lib/firmware/UTS_RELEASE                           │
│    └── /lib/firmware                                       │
│                                                             │
│ 3. 平台固件 fallback (如果启用 CONFIG_EFI_EMBEDDED_FIRMWARE)│
│    └── 查找 EFI embedded firmware                          │
│                                                             │
│ 4. sysfs fallback (如果启用 CONFIG_FW_LOADER_USER_HELPER)   │
│    └── 通过 uevent 通知用户空间加载固件                    │
└─────────────────────────────────────────────────────────────┘
```

```bash
# 基本固件加载支持
CONFIG_FW_LOADER=y

# 调试日志（记录固件文件名和 SHA256 校验和）
CONFIG_FW_LOADER_DEBUG=y

# 用户空间 fallback 机制（当前禁用）
# CONFIG_FW_LOADER_USER_HELPER is not set

# 压缩固件支持（当前禁用）
# CONFIG_FW_LOADER_COMPRESS is not set

# 其他相关配置
CONFIG_FIRMWARE_MEMMAP=y
CONFIG_FIRMWARE_TABLE=y
```

GPU/加速卡驱动

| 驱动             | 文件路径                                        | 固件用途             |
|------------------|-------------------------------------------------|----------------------|
| **i915** (Intel) | `drivers/gpu/drm/i915/gt/uc/intel_uc_fw.c`      | GuC/HuC/DMC 固件     |
| **amdgpu**       | `drivers/gpu/drm/amd/amdgpu/amdgpu_ucode.c`     | 微代码/VCN/SDMA 固件 |
| **nouveau**      | `drivers/gpu/drm/nouveau/nvkm/core/firmware.c`  | NVIDIA GPU 固件      |
| **radeon**       | `drivers/gpu/drm/radeon/`                       | UVD/VCE/微代码固件   |
| **xe**           | `drivers/gpu/drm/xe/xe_uc_fw.c`                 | Intel Xe GPU 固件    |
| **panthor**      | `drivers/gpu/drm/panthor/panthor_fw.c`          | ARM Mali 固件        |
| **msm**          | `drivers/gpu/drm/msm/adreno/adreno_gpu.c`       | Adreno GPU 固件      |
| **habanalabs**   | `drivers/accel/habanalabs/common/firmware_if.c` | AI 加速器固件        |
| **ivpu**         | `drivers/accel/ivpu/ivpu_fw.c`                  | Intel VPU 固件       |
| **amdxdna**      | `drivers/accel/amdxdna/aie2_pci.c`              | AMD NPU 固件         |

无线网络驱动

| 驱动 | 文件路径 |
|------|----------|
| **iwlwifi** | `drivers/net/wireless/intel/iwlwifi/` |
| **ath10k/ath11k/ath12k** | `drivers/net/wireless/ath/` |
| **rtw88/rtw89** | `drivers/net/wireless/realtek/rtw88/`, `rtw89/` |
| **rtlwifi** | `drivers/net/wireless/realtek/rtlwifi/` |
| **mt76** | `drivers/net/wireless/mediatek/mt76/` |
| **brcmfmac** | `drivers/net/wireless/broadcom/brcm80211/brcmfmac/` |
| **wl1251/wl18xx** | `drivers/net/wireless/ti/` |

蓝牙驱动

| 驱动            | 文件路径                          |
|-----------------|-----------------------------------|
| **btintel**     | `drivers/bluetooth/btintel.c`     |
| **btrtl**       | `drivers/bluetooth/btrtl.c`       |
| **btusb**       | `drivers/bluetooth/btusb.c`       |
| **btqca**       | `drivers/bluetooth/btqca.c`       |
| **btmtk**       | `drivers/bluetooth/btmtk.c`       |
| **hci_bcm4377** | `drivers/bluetooth/hci_bcm4377.c` |

音频驱动

| 驱动                          | 文件路径                                  |
|-------------------------------|-------------------------------------------|
| **SOF (Sound Open Firmware)** | `sound/soc/sof/loader.c`, `ipc4-loader.c` |
| **Intel AVS**                 | `sound/soc/intel/avs/loader.c`            |
| **Intel SST**                 | `sound/soc/intel/atom/sst/sst_loader.c`   |
| **wm_adsp**                   | `sound/soc/codecs/wm_adsp.c`              |
| **hda**                       | `sound/hda/codecs/ca0132.c`               |

SCSI/存储驱动

| 驱动         | 文件路径                                      |
|--------------|-----------------------------------------------|
| **lpfc**     | `drivers/scsi/lpfc/lpfc_init.c`               |
| **qla2xxx**  | `drivers/scsi/qla2xxx/qla_init.c`, `qla_nx.c` |
| **mpt3sas**  | `drivers/scsi/mpt3sas/`                       |
| **smartpqi** | `drivers/scsi/smartpqi/smartpqi_init.c`       |
| **aic94xx**  | `drivers/scsi/aic94xx/aic94xx_init.c`         |
| **isci**     | `drivers/scsi/isci/init.c`                    |
| **elx/efct** | `drivers/scsi/elx/efct/efct_driver.c`         |
| **pm8001**   | `drivers/scsi/pm8001/pm8001_ctl.c`            |

5.6 其他重要驱动

| 类别       | 驱动             | 文件路径                  |
|------------|------------------|---------------------------|
| FPGA       | fpga-mgr         | `drivers/fpga/fpga-mgr.c` |
| Remoteproc | 各种 SoC         | `drivers/remoteproc/`     |
| TEE        | amdtee, optee    | `drivers/tee/`            |
| 加密       | ccp, qat, cavium | `drivers/crypto/`         |
| 网络       | netronome, qed   | `drivers/net/ethernet/`   |

### 内核如何读取文件的

测试方法:
sudo rmmod iwlmvm
sudo rmmod iwlwifi

利用 fs/kernel_read_file.c:kernel_read_file_from_path_initns

调用过程大致如下:
```txt
@[
        kernel_read_file_from_path_initns+5
        fw_get_filesystem_firmware+310
        _request_firmware+644
        request_firmware_work_func+66
        process_one_work+402
        worker_thread+602
        kthread+252
        ret_from_fork+244
        ret_from_fork_asm+26
]: 4
@[
        kernel_read_file_from_path_initns+5
        fw_get_filesystem_firmware+310
        _request_firmware+1021
        firmware_request_nowarn+61
        iwl_dbg_tlv_load_bin+113
        iwl_req_fw_callback+2041
        request_firmware_work_func+84
        process_one_work+402
        worker_thread+602
        kthread+252
        ret_from_fork+244
        ret_from_fork_asm+26
]: 4
```

## 实验 1 : 如果去掉 firmware
正常加载的时候:
```txt
🧀  dmesg | grep firmware
[1727438.224334] iwlwifi 0000:00:14.3: Loaded firmware version: 89.7207fc64.0 so-a0-hr-b0-89.ucode
```

如果我把 iwlwifi-mvm-firmware-20251125-1.fc42.noarch 给删掉，那么 wifi 驱动直接会加载失败:
```txt
[    4.641858] iwlwifi 0000:00:14.3: Detected crf-id 0x1300504, cnv-id 0x80401 wfpm id 0x80000030
[    4.641883] iwlwifi 0000:00:14.3: PCI dev 7af0/0074, rev=0x430, rfid=0x10a100
[    4.641884] iwlwifi 0000:00:14.3: Detected Intel(R) Wi-Fi 6 AX201 160MHz
[    4.642044] iwlwifi 0000:00:14.3: Direct firmware load for iwlwifi-so-a0-hr-b0-89.ucode failed with error -2
[    4.642046] iwlwifi 0000:00:14.3: no suitable firmware found!
[    4.642047] iwlwifi 0000:00:14.3: iwlwifi-so-a0-hr-b0-89 is required
[    4.642047] iwlwifi 0000:00:14.3: check git://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git
```
将驱动加载回来之后，继续 modprobe -r iwlwifi 然后 modprobe iwlwifi ，然后一切正常。


原来加载的驱动在:
```txt
/lib/firmware/intel/iwlwifi/iwlwifi-so-a0-hr-b0-89.ucode.xz
```

```txt
🤒  sudo rpm -qf /lib/firmware/intel/iwlwifi/iwlwifi-so-a0-hr-b0-89.ucode.xz
iwlwifi-mvm-firmware-20251125-1.fc42.noarch
```
而不是在 linux-firmware 中的。

另外的两个经典案例为:

i915 (Intel 核显)
```
[    2.212558] i915 0000:00:02.0: [drm] Finished loading DMC firmware i915/adls_dmc_ver2_01.bin (v2.1)
[    2.247427] i915 0000:00:02.0: [drm] GT0: GuC firmware i915/tgl_guc_70.bin version 70.49.4
[    2.247431] i915 0000:00:02.0: [drm] GT0: HuC firmware i915/tgl_huc.bin version 7.9.3
```
加载:
- `i915/adls_dmc_ver2_01.bin` - Display Micro-Controller
- `i915/tgl_guc_70.bin` - Graphics Micro-Controller (版本 70.49.4)
- `i915/tgl_huc.bin` - HuC 微控制器 (版本 7.9.3)

Bluetooth (Intel)
```
[    4.635995] Bluetooth: hci0: Minimum firmware build 1 week 10 2014
[    4.657241] Bluetooth: hci0: Found device firmware: intel/ibt-1040-4150.sfi
[    4.657248] Bluetooth: hci0: Firmware Version: 133-20.25
[    5.980974] Bluetooth: hci0: Waiting for firmware download to complete
[    5.981964] Bluetooth: hci0: Firmware loaded in 1293673 usecs
[    6.005989] Bluetooth: hci0: Firmware timestamp 2025.20 buildtype 1 build 82053
[    6.005991] Bluetooth: hci0: Firmware SHA1: 0x937bca4a
```

## 实验 2 : 如果将 wifi 直通到虚拟机中，那么虚拟机中必须安装 firmware rpm

```txt
sudo modprobe -r iwlmvm
sudo modprobe -r iwlwifi
```
直通到虚拟机中的确可以观察到:
```txt
[    3.479424] iwlwifi 0000:00:09.0: loaded firmware version 89.df9556fc.0 so-a0-hr-b0-89.ucode op_mode iwlmvm
```

在虚拟机中观察到的是，不知道为什么，没有 carrier :
```txt
4: wls9: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN group default qlen 1000
    link/ether 72:a4:ec:48:fa:26 brd ff:ff:ff:ff:ff:ff permaddr 70:a8:d3:66:73:bc
    altname wlp0s9
    altname wlx70a8d36673bc
```

## 课后习题
虚拟机中是否需要使用 firmware ?

简单来说，如果直通设备，那么是可能是需要的。否则不需要安装 linux-firmware.rpm 之类的东西。
参考: https://bugzilla.redhat.com/show_bug.cgi?id=1386202

虚拟机中可以看到的 option rom ，那是另外的一个问题。

## 参考资料
- https://github.com/NixOS/nixpkgs/blob/nixos-23.05/pkgs/os-specific/linux/firmware/linux-firmware/default.nix#L28
- https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git
- https://www.linuxfromscratch.org/blfs/view/stable/postlfs/firmware.html


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
