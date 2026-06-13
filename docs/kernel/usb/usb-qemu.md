# qemu 模型


在 windows 中是可以识别的，但是在虚拟机中无法识别:

物理机测试:
```txt
[888118.673614] usb 1-7: new high-speed USB device number 51 using xhci_hcd
[888118.800105] usb 1-7: New USB device found, idVendor=152d, idProduct=0583, bcdDevice= 2.05
[888118.800108] usb 1-7: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[888118.800109] usb 1-7: Product: FF520
[888118.800110] usb 1-7: Manufacturer: Fanxiang
[888118.800111] usb 1-7: SerialNumber: DD56419883B8B
[888118.822635] usbcore: registered new interface driver usb-storage
[888118.835683] scsi host9: uas
[888118.835753] usbcore: registered new interface driver uas
[888118.846042] scsi 9:0:0:0: Direct-Access     Fanxiang FF520            0205 PQ: 0 ANSI: 6
[888134.073917] sd 9:0:0:0: [sdd] Unit Not Ready
[888134.073921] sd 9:0:0:0: [sdd] Sense Key : Hardware Error [current]
[888134.073922] sd 9:0:0:0: [sdd] ASC=0x44 <<vendor>>ASCQ=0x81
[888134.074224] sd 9:0:0:0: [sdd] Read Capacity(16) failed: Result: hostbyte=DID_OK driverbyte=DRIVER_OK
[888134.074225] sd 9:0:0:0: [sdd] Sense Key : Hardware Error [current]
[888134.074226] sd 9:0:0:0: [sdd] ASC=0x44 <<vendor>>ASCQ=0x81
[888134.074557] sd 9:0:0:0: [sdd] Read Capacity(10) failed: Result: hostbyte=DID_OK driverbyte=DRIVER_OK
[888134.074557] sd 9:0:0:0: [sdd] Sense Key : Hardware Error [current]
[888134.074558] sd 9:0:0:0: [sdd] ASC=0x44 <<vendor>>ASCQ=0x81
[888134.074632] sd 9:0:0:0: [sdd] 0 512-byte logical blocks: (0 B/0 B)
[888134.074633] sd 9:0:0:0: [sdd] 0-byte physical blocks
[888134.074891] sd 9:0:0:0: [sdd] Test WP failed, assume Write Enabled
[888134.074969] sd 9:0:0:0: [sdd] Asking for cache data failed
[888134.074972] sd 9:0:0:0: [sdd] Assuming drive cache: write through
[888134.074973] sd 9:0:0:0: [sdd] Preferred minimum I/O size 4096 bytes not a multiple of physical block size (0 bytes)
[888134.074975] sd 9:0:0:0: [sdd] Optimal transfer size 33553920 bytes not a multiple of physical block size (0 bytes)
[888134.075207] sd 9:0:0:0: [sdd] Attached SCSI disk
```

cyme 的识别:
  1  53  0x152d 0x0583 FF520                DD56419883B8B 480.0 Mb/s

给 windows 插上过一次之后，之后可以正常识别了:
```txt
[888350.687504] usb 1-7: USB disconnect, device number 52
[888685.136542] usb 1-7: new high-speed USB device number 53 using xhci_hcd
[888685.263318] usb 1-7: New USB device found, idVendor=152d, idProduct=0583, bcdDevice= 2.05
[888685.263321] usb 1-7: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[888685.263322] usb 1-7: Product: FF520
[888685.263323] usb 1-7: Manufacturer: Fanxiang
[888685.263324] usb 1-7: SerialNumber: DD56419883B8B
[888685.276189] scsi host9: uas
[888685.286801] scsi 9:0:0:0: Direct-Access     Fanxiang FF520            0205 PQ: 0 ANSI: 6
[888689.963941] sd 9:0:0:0: [sdd] 1000215216 512-byte logical blocks: (512 GB/477 GiB)
[888689.963946] sd 9:0:0:0: [sdd] 4096-byte physical blocks
[888689.964078] sd 9:0:0:0: [sdd] Write Protect is off
[888689.964079] sd 9:0:0:0: [sdd] Mode Sense: 5f 00 00 08
[888689.964346] sd 9:0:0:0: [sdd] Write cache: enabled, read cache: enabled, doesn't support DPO or FUA
[888689.964350] sd 9:0:0:0: [sdd] Preferred minimum I/O size 4096 bytes
[888689.964351] sd 9:0:0:0: [sdd] Optimal transfer size 33553920 bytes not a multiple of preferred minimum block size (4096 bytes)
[888689.965713]  sdd: sdd1 sdd2
[888689.965848] sd 9:0:0:0: [sdd] Attached SCSI disk
```

神奇的地方就是，时间居然是对应的:


## qemu 模拟 usb camera 这个是我实在是没有想到的
https://mp.weixin.qq.com/s?__biz=MzkzMTk4OTIwNg==&mid=2247484156&idx=1&sn=eb5468e97a888cb5b76ec2e216559761&scene=21&poc_token=HFT_jWijzafMTqbRV_lKWSfkHzVwTktRLtUaphDb

## 装机好工具
ventor

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
