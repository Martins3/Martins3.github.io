## intel 集成显卡如何测试 sriov
<!-- c0bbc3de-ac47-4196-89f2-4fd45b0ca038 -->

切换为 xe 后，仅仅需要
echo 7 | sudo tee /sys/devices/pci0000:00/0000:00:02.0/sriov_numvfs

然后就可以看到:
```txt
00:02.0 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.1 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.2 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.3 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.4 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.5 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.6 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
00:02.7 VGA compatible controller [0300]: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] [8086:a780] (rev 04)
```

然后就像是普通的 pci 直通，选择其中的任何一个

然后就可以在虚拟机中看到:
```txt
lspci | grep -i vga

00:08.0 VGA compatible controller: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] (rev 04)
```

虚拟机也必须使用 xe 驱动，然后就可以看到:
```txt
[    1.315116] xe 0000:00:08.0: vgaarb: deactivate vga console
[    1.315915] xe 0000:00:08.0: [drm] Running in SR-IOV VF mode
[    1.315918] xe 0000:00:08.0: [drm] VF: migration disabled: experimental feature not available on production builds
[    1.329403] xe 0000:00:08.0: [drm] Tile0: GT0: vcs1 fused off
[    1.329406] xe 0000:00:08.0: [drm] Tile0: GT0: vcs3 fused off
[    1.329406] xe 0000:00:08.0: [drm] Tile0: GT0: vcs4 fused off
[    1.329407] xe 0000:00:08.0: [drm] Tile0: GT0: vcs5 fused off
[    1.329409] xe 0000:00:08.0: [drm] Tile0: GT0: vcs6 fused off
[    1.329410] xe 0000:00:08.0: [drm] Tile0: GT0: vcs7 fused off
[    1.329410] xe 0000:00:08.0: [drm] Tile0: GT0: vecs1 fused off
[    1.329411] xe 0000:00:08.0: [drm] Tile0: GT0: vecs2 fused off
[    1.329411] xe 0000:00:08.0: [drm] Tile0: GT0: vecs3 fused off
[    1.335276] [drm] Initialized xe 1.1.0 for 0000:00:08.0 on minor 1
```

## 实验 1 : 使用 xe 来输出

## 实验 2 : 使用 xe 渲染
