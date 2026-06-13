# 简单分析下 GPU 驱动
说不定之后要分析 virtio-gpu 的。

首先，即使是集成显卡，也是有 GPU 的:
```txt
[114174.876779] i915 0000:00:02.0: [drm] GPU HANG: ecode 12:1:859ffffb, in slack [19356]
[114174.877344] i915 0000:00:02.0: [drm] Resetting chip for stopped heartbeat on rcs0
[114174.978503] i915 0000:00:02.0: [drm] slack[19356] context reset due to GPU hang
[114174.978542] i915 0000:00:02.0: [drm] GuC firmware i915/tgl_guc_70.1.1.bin version 70.1
[114174.978544] i915 0000:00:02.0: [drm] HuC firmware i915/tgl_huc_7.9.3.bin version 7.9
[114174.982370] i915 0000:00:02.0: [drm] HuC authenticated
[114174.982603] i915 0000:00:02.0: [drm] GuC submission enabled
[114174.982604] i915 0000:00:02.0: [drm] GuC SLPC enabled
```

- http://127.0.0.1:3434/gpu/introduction.html 这里存在很多文档，已经够了。


内核启动的时候的样子：
```txt
[    2.183239] systemd[1]: Starting Load Kernel Module drm...
[    2.201508] ACPI: bus type drm_connector registered
[    2.809626] i915 0000:00:02.0: vgaarb: deactivate vga console
[    2.809655] i915 0000:00:02.0: [drm] Using Transparent Hugepages
[    2.810215] i915 0000:00:02.0: vgaarb: changed VGA decodes: olddecodes=io+mem,decodes=io+mem:owns=io+mem
[    2.811237] i915 0000:00:02.0: [drm] Finished loading DMC firmware i915/adls_dmc_ver2_01.bin (v2.1)
[    2.824596] i915 0000:00:02.0: [drm] GuC firmware i915/tgl_guc_70.1.1.bin version 70.1
[    2.824598] i915 0000:00:02.0: [drm] HuC firmware i915/tgl_huc_7.9.3.bin version 7.9
[    2.827208] i915 0000:00:02.0: [drm] HuC authenticated
[    2.827470] i915 0000:00:02.0: [drm] GuC submission enabled
[    2.827471] i915 0000:00:02.0: [drm] GuC SLPC enabled
[    2.827881] i915 0000:00:02.0: [drm] GuC RC: enabled
[    2.833849] iwlwifi 0000:00:14.3: Detected Intel(R) Wi-Fi 6 AX201 160MHz, REV=0x430
[    2.833873] thermal thermal_zone1: failed to read out thermal zone (-61)
[    2.847202] [drm] Initialized i915 1.6.0 20201103 for 0000:00:02.0 on minor 0
[    2.847666] ACPI: video: Video Device [GFX0] (multi-head: yes  rom: no  post: no)
[    2.847903] input: Video Bus as /devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/LNXVIDEO:00/input/input8
[    2.847950] snd_hda_intel 0000:00:1f.3: bound 0000:00:02.0 (ops i915_audio_component_bind_ops [i915])
[    2.877281] fbcon: i915drmfb (fb0) is primary device
[    2.919750] Console: switching to colour frame buffer device 480x135
[    2.938955] i915 0000:00:02.0: [drm] fb0: i915drmfb frame buffer device
```

## drm

- https://embear.ch/blog/drm-framebuffer : 分析 drm 实现
