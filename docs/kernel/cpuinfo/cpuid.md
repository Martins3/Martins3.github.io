简化 https://github.com/tycho/cpuid 的实现

仅仅实现 `./cpuid -d -c 1` 这个功能

```txt
🧀  ./cpuid -d -c 1
CPU 1:
CPUID 00000000:00 = 00000020 756e6547 6c65746e 49656e69 |  ...GenuntelineI
CPUID 00000001:00 = 000b0671 01800800 7ffafbff bfebfbff | q...............
CPUID 00000002:00 = 00feff01 000000f0 00000000 00000000 | ................
CPUID 00000003:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000004:00 = fc004121 02c0003f 0000003f 00000000 | !A..?...?.......
CPUID 00000004:01 = fc004122 01c0003f 0000003f 00000000 | "A..?...?.......
CPUID 00000004:02 = fc01c143 03c0003f 000007ff 00000000 | C...?...........
CPUID 00000004:03 = fc1fc163 02c0003f 0000bfff 00000004 | c...?...........
CPUID 00000004:04 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000005:00 = 00000040 00000040 00000003 10102020 | @...@.......  ..
CPUID 00000006:00 = 00dfcff7 00000002 00000409 00000003 | ................
CPUID 00000007:00 = 00000002 239c27eb 98c027bc fc1cc410 | .....'.#.'......
CPUID 00000007:01 = 00400810 00000000 00000000 00000000 | ..@.............
CPUID 00000007:02 = 00000000 00000000 00000000 0000001f | ................
CPUID 00000008:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000009:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000a:00 = 07300605 00000000 00000007 00008603 | ..0.............
CPUID 0000000b:00 = 00000001 00000002 00000100 00000001 | ................
CPUID 0000000b:01 = 00000007 00000020 00000201 00000001 | .... ...........
CPUID 0000000c:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000d:00 = 00000207 00000a88 00000a88 00000000 | ................
CPUID 0000000d:01 = 0000000f 00000570 00019900 00000000 | ....p...........
CPUID 0000000d:02 = 00000100 00000240 00000000 00000000 | ....@...........
CPUID 0000000e:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000f:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000010:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000011:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000012:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000013:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000014:00 = 00000001 0000005f 00000007 00000000 | ...._...........
CPUID 00000014:01 = 02490002 003f003f 00000000 00000000 | ..I.?.?.........
CPUID 00000015:00 = 00000002 0000009c 0249f000 00000000 | ..........I.....
CPUID 00000016:00 = 00000bb8 000016a8 00000064 00000000 | ........d.......
CPUID 00000017:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000018:00 = 00000008 00000000 00000000 00000000 | ................
CPUID 00000018:01 = 00000000 00080001 00000020 00004022 | ........ ..."@..
CPUID 00000018:02 = 00000000 00080006 00000004 00004022 | ............"@..
CPUID 00000018:03 = 00000000 0010000f 00000001 00004125 | ............%A..
CPUID 00000018:04 = 00000000 00040001 00000010 00004024 | ............$@..
CPUID 00000018:05 = 00000000 00040006 00000008 00004024 | ............$@..
CPUID 00000018:06 = 00000000 00080008 00000001 00004124 | ............$A..
CPUID 00000018:07 = 00000000 00080007 00000080 00004043 | ............C@..
CPUID 00000018:08 = 00000000 00080009 00000080 00004043 | ............C@..
CPUID 00000019:00 = 00000007 00000014 00000003 00000000 | ................
CPUID 0000001a:00 = 40000001 00000000 00000000 00000000 | ...@............
CPUID 0000001b:00 = 00000001 00000001 00000000 00000000 | ................
CPUID 0000001b:01 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001c:00 = 4000000b 00000007 00000007 00000000 | ...@............
CPUID 0000001d:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001e:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001f:00 = 00000001 00000002 00000100 00000001 | ................
CPUID 0000001f:01 = 00000007 00000020 00000201 00000001 | .... ...........
CPUID 00000020:00 = 00000000 00000001 00000000 00000000 | ................
CPUID 40000000:00 = 00000000 00000001 00000000 00000000 | ................
CPUID 80000000:00 = 80000008 00000000 00000000 00000000 | ................
CPUID 80000001:00 = 00000000 00000000 00000121 2c100800 | ........!......,
CPUID 80000002:00 = 68743331 6e654720 746e4920 52286c65 | 13th Gen Intel(R
CPUID 80000003:00 = 6f432029 54286572 6920294d 33312d39 | ) Core(TM) i9-13
CPUID 80000004:00 = 4b303039 00000000 00000000 00000000 | 900K............
CPUID 80000005:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 80000006:00 = 00000000 00000000 08007040 00000000 | ........@p......
CPUID 80000007:00 = 00000000 00000000 00000000 00000100 | ................
CPUID 80000008:00 = 0000302e 00000000 00000000 00000000 | .0..............
CPUID 80860000:00 = 00000000 00000001 00000000 00000000 | ................
CPUID c0000000:00 = 00000000 00000001 00000000 00000000 | ................
```

```txt
➜  share ./cpuid -d -c 1
CPU 1:
CPUID 00000000:00 = 0000001f 756e6547 6c65746e 49656e69 | ....GenuntelineI
CPUID 00000001:00 = 000b0671 011f0800 fffa3223 1f8bfbff | q.......#2......
CPUID 00000002:00 = 00000001 00000000 0000004d 002c307d | ........M...}0,.
CPUID 00000003:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000004:00 = 78000121 01c0003f 0000003f 00000001 | !..x?...?.......
CPUID 00000004:01 = 78000122 01c0003f 0000003f 00000001 | "..x?...?.......
CPUID 00000004:02 = 78000143 03c0003f 00000fff 00000001 | C..x?...........
CPUID 00000004:03 = 7807c163 03c0003f 00003fff 00000006 | c..x?....?......
CPUID 00000004:04 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000005:00 = 00000000 00000000 00000003 00000000 | ................
CPUID 00000006:00 = 00000004 00000000 00000000 00000000 | ................
CPUID 00000007:00 = 00000001 219c07ab 1840073c ac004410 | .......!<.@..D..
CPUID 00000007:01 = 00000810 00000000 00000000 00000000 | ................
CPUID 00000008:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000009:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000a:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000b:00 = 00000000 00000001 00000100 00000001 | ................
CPUID 0000000b:01 = 00000005 0000001f 00000201 00000001 | ................
CPUID 0000000c:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000d:00 = 00000207 00000a88 00000a88 00000000 | ................
CPUID 0000000d:01 = 0000000f 00000348 00000000 00000000 | ....H...........
CPUID 0000000d:02 = 00000100 00000240 00000000 00000000 | ....@...........
CPUID 0000000e:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000000f:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000010:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000011:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000012:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000013:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000014:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000015:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000016:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000017:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000018:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 00000019:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001a:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001b:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001c:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001d:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001e:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 0000001f:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 40000000:00 = 40000001 4b4d564b 564b4d56 0000004d | ...@KVMKVMKVM...
CPUID 40000001:00 = 01007afb 00000000 00000000 00000000 | .z..............
CPUID 80000000:00 = 80000008 756e6547 6c65746e 49656e69 | ....GenuntelineI
CPUID 80000001:00 = 000b0671 00000000 00000121 2c100800 | q.......!......,
CPUID 80000002:00 = 68743331 6e654720 746e4920 52286c65 | 13th Gen Intel(R
CPUID 80000003:00 = 6f432029 54286572 6920294d 33312d39 | ) Core(TM) i9-13
CPUID 80000004:00 = 4b303039 00000000 00000000 00000000 | 900K............
CPUID 80000005:00 = 01ff01ff 01ff01ff 40020140 40020140 | ........@..@@..@
CPUID 80000006:00 = 00000000 42004200 02008140 00808140 | .....B.B@...@...
CPUID 80000007:00 = 00000000 00000000 00000000 00000000 | ................
CPUID 80000008:00 = 0000302e 0100d000 0000501e 00000000 | .0.......P......
CPUID 80860000:00 = 00000000 00000000 00000000 00000000 | ................
CPUID c0000000:00 = 00000000 00000000 00000000 00000000 | ................
```
虚拟机中观测到的内容。

nixos 中的 cpuid 中似乎总是用的这个: http://www.etallen.com/cpuid.html

使用这个命令也是类似的效果:
cpuid -1 -r

## 似乎 clearcpuid=156 带来的变化是什么?
```diff
diff --git a/docs/kernel/cpuinfo/cpuid.md b/docs/kernel/cpuinfo/cpuid.md
index 5dcf882..4970bc9 100644
--- a/docs/kernel/cpuinfo/cpuid.md
+++ b/docs/kernel/cpuinfo/cpuid.md
@@ -26,7 +26,7 @@ CPUID 0000000b:00 = 00000001 00000002 00000100 00000001 | ................
 CPUID 0000000b:01 = 00000007 00000020 00000201 00000001 | .... ...........
 CPUID 0000000c:00 = 00000000 00000000 00000000 00000000 | ................
 CPUID 0000000d:00 = 00000207 00000a88 00000a88 00000000 | ................
-CPUID 0000000d:01 = 0000000f 00000670 00019900 00000000 | ....p...........
+CPUID 0000000d:01 = 0000000f 00000570 00019900 00000000 | ....p...........
 CPUID 0000000d:02 = 00000100 00000240 00000000 00000000 | ....@...........
 CPUID 0000000e:00 = 00000000 00000000 00000000 00000000 | ................
 CPUID 0000000f:00 = 00000000 00000000 00000000 00000000 | ................
@@ -132,3 +132,6 @@ CPUID c0000000:00 = 00000000 00000000 00000000 00000000 | ................
```

## 实际上，不是体现在 cpuid 上的差别，而是 xsave 大小的差别
```diff
🧀  diff after-clearcpuid before-clearcpuid
14c14
<       process local APIC physical ID = 0x0 (0)
---
>       process local APIC physical ID = 0x8 (8)
196c196
<       index of CPU's row in feedback struct   = 0x0 (0)
---
>       index of CPU's row in feedback struct   = 0x1 (1)
353c353
<       extended APIC ID                      = 0
---
>       extended APIC ID                      = 8
397c397
<       SAVE area size in bytes                     = 0x00000570 (1392)
---
>       SAVE area size in bytes                     = 0x00000670 (1648)
673c673
<       x2APIC ID of logical processor = 0x0 (0)
---
>       x2APIC ID of logical processor = 0x8 (8)
807c807
<    (APIC synth): PKG_ID=0 CORE_ID=0 SMT_ID=0
---
>    (APIC synth): PKG_ID=0 CORE_ID=4 SMT_ID=0
```
所以，是不是可以通过操作系统来修改的。