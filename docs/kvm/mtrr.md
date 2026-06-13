# mtrr

- [ ] 在 docs/qemu/seabios.md 遇到过

https://wiki.gentoo.org/wiki/MTRR_and_PAT

### /proc/mtrr

### 13900k
```txt
reg00: base=0x000000000 (    0MB), size=131072MB, count=1: write-back
reg01: base=0x07c000000 ( 1984MB), size=   64MB, count=1: uncachable
reg02: base=0x080000000 ( 2048MB), size= 2048MB, count=1: uncachable
```

### xiaomi
```txt
🧀  sudo cat /proc/mtrr
reg00: base=0x0c0000000 ( 3072MB), size= 1024MB, count=1: uncachable
reg01: base=0x0a0000000 ( 2560MB), size=  512MB, count=1: uncachable
reg02: base=0x090000000 ( 2304MB), size=  256MB, count=1: uncachable
reg03: base=0x08e000000 ( 2272MB), size=   32MB, count=1: uncachable
reg04: base=0x08d800000 ( 2264MB), size=    8MB, count=1: uncachable
```

### n100
```txt
🤒  sudo cat /proc/mtrr
reg00: base=0x080000000 ( 2048MB), size= 2048MB, count=1: uncachable
reg01: base=0x078000000 ( 1920MB), size=  128MB, count=1: uncachable
reg02: base=0x2000000000 (131072MB), size=131072MB, count=1: uncachable
reg03: base=0x1000000000 (65536MB), size=65536MB, count=1: uncachable
reg04: base=0x800000000 (32768MB), size=32768MB, count=1: uncachable
reg05: base=0x4000000000 (262144MB), size=262144MB, count=1: uncachable
reg06: base=0x483000000 (18480MB), size=    8MB, count=1: uncachable
reg07: base=0x482000000 (18464MB), size=   16MB, count=1: uncachable
reg08: base=0x480000000 (18432MB), size=   32MB, count=1: uncachable
reg09: base=0x47f800000 (18424MB), size=    8MB, count=1: uncachable
```

## /sys/kernel/debug/x86/pat_memtype_list

### n100
```txt
PAT memtype list:
PAT: [mem 0x0000000068144000-0x0000000068145000] write-back
PAT: [mem 0x000000006f3d5000-0x000000006f3d6000] write-back
PAT: [mem 0x000000006f3d6000-0x000000006f3d7000] write-back
PAT: [mem 0x000000006f3d7000-0x000000006f3d8000] write-back
PAT: [mem 0x000000006f3d8000-0x000000006f3dc000] write-back
PAT: [mem 0x000000006f3dc000-0x000000006f3e0000] write-back
PAT: [mem 0x000000006f3e0000-0x000000006f3e1000] write-back
PAT: [mem 0x000000006f3e1000-0x000000006f3e2000] write-back
PAT: [mem 0x000000006f3e2000-0x000000006f3e3000] write-back
PAT: [mem 0x000000006f3e3000-0x000000006f3e5000] write-back
PAT: [mem 0x000000006f3e5000-0x000000006f3e6000] write-back
PAT: [mem 0x000000006f3e6000-0x000000006f3e7000] write-back
PAT: [mem 0x000000006f3e7000-0x000000006f3ea000] write-back
PAT: [mem 0x000000006f3ea000-0x000000006f3ed000] write-back
PAT: [mem 0x000000006f3ed000-0x000000006f3ee000] write-back
PAT: [mem 0x000000006f3ee000-0x000000006f3ef000] write-back
PAT: [mem 0x000000006f3ef000-0x000000006f3f0000] write-back
PAT: [mem 0x000000006f3f0000-0x000000006f3f2000] write-back
PAT: [mem 0x000000006f3f2000-0x000000006f3f3000] write-back
PAT: [mem 0x000000006f3f3000-0x000000006f3f4000] write-back
PAT: [mem 0x000000006f3f4000-0x000000006f3f8000] write-back
PAT: [mem 0x000000006f3f8000-0x000000006f3fb000] write-back
PAT: [mem 0x000000006f3fb000-0x000000006f401000] write-back
PAT: [mem 0x000000006f401000-0x000000006f402000] write-back
PAT: [mem 0x000000006f402000-0x000000006f478000] write-back
PAT: [mem 0x000000006f478000-0x000000006f479000] write-back
PAT: [mem 0x000000006f479000-0x000000006f47a000] write-back
PAT: [mem 0x000000006f47a000-0x000000006f47b000] write-back
PAT: [mem 0x000000006f4cd000-0x000000006f4ce000] write-back
PAT: [mem 0x000000006f4de000-0x000000006f4df000] write-back
PAT: [mem 0x000000006f4df000-0x000000006f4e1000] write-back
PAT: [mem 0x000000006f4df000-0x000000006f4e2000] write-back
PAT: [mem 0x000000006f4e1000-0x000000006f4e4000] write-back
PAT: [mem 0x000000006f4e4000-0x000000006f4e5000] write-back
PAT: [mem 0x000000006f518000-0x000000006f519000] write-back
PAT: [mem 0x000000006f539000-0x000000006f53a000] write-back
PAT: [mem 0x000000006f53a000-0x000000006f53b000] write-back
PAT: [mem 0x000000006fbb3000-0x000000006fbb5000] write-back
PAT: [mem 0x0000000080800000-0x0000000080900000] uncached-minus
PAT: [mem 0x0000000080900000-0x0000000080901000] uncached-minus
PAT: [mem 0x0000000080a00000-0x0000000080b00000] uncached-minus
PAT: [mem 0x0000000080b00000-0x0000000080b01000] uncached-minus
PAT: [mem 0x0000000080c00000-0x0000000080c02000] uncached-minus
PAT: [mem 0x0000000080c03000-0x0000000080c04000] uncached-minus
PAT: [mem 0x0000000080d02000-0x0000000080d03000] uncached-minus
PAT: [mem 0x00000000c0000000-0x00000000d0000000] uncached-minus
PAT: [mem 0x00000000c00a3000-0x00000000c00a4000] uncached-minus
PAT: [mem 0x00000000c00d8000-0x00000000c00d9000] uncached-minus
PAT: [mem 0x00000000c00d9000-0x00000000c00da000] uncached-minus
PAT: [mem 0x00000000c00da000-0x00000000c00db000] uncached-minus
PAT: [mem 0x00000000c00db000-0x00000000c00dc000] uncached-minus
PAT: [mem 0x00000000c00dc000-0x00000000c00dd000] uncached-minus
PAT: [mem 0x00000000c00dd000-0x00000000c00de000] uncached-minus
PAT: [mem 0x00000000c00de000-0x00000000c00df000] uncached-minus
PAT: [mem 0x00000000c00df000-0x00000000c00e0000] uncached-minus
PAT: [mem 0x00000000c00e0000-0x00000000c00e1000] uncached-minus
PAT: [mem 0x00000000c00e1000-0x00000000c00e2000] uncached-minus
PAT: [mem 0x00000000c00e2000-0x00000000c00e3000] uncached-minus
PAT: [mem 0x00000000c00e3000-0x00000000c00e4000] uncached-minus
PAT: [mem 0x00000000c00e4000-0x00000000c00e5000] uncached-minus
PAT: [mem 0x00000000c00e5000-0x00000000c00e6000] uncached-minus
PAT: [mem 0x00000000c00e6000-0x00000000c00e7000] uncached-minus
PAT: [mem 0x00000000c00e7000-0x00000000c00e8000] uncached-minus
PAT: [mem 0x00000000c00e8000-0x00000000c00e9000] uncached-minus
PAT: [mem 0x00000000c00e9000-0x00000000c00ea000] uncached-minus
PAT: [mem 0x00000000c00ea000-0x00000000c00eb000] uncached-minus
PAT: [mem 0x00000000c00eb000-0x00000000c00ec000] uncached-minus
PAT: [mem 0x00000000c00ec000-0x00000000c00ed000] uncached-minus
PAT: [mem 0x00000000c00ed000-0x00000000c00ee000] uncached-minus
PAT: [mem 0x00000000c00ee000-0x00000000c00ef000] uncached-minus
PAT: [mem 0x00000000c00ef000-0x00000000c00f0000] uncached-minus
PAT: [mem 0x00000000c0100000-0x00000000c0101000] uncached-minus
PAT: [mem 0x00000000c0200000-0x00000000c0201000] uncached-minus
PAT: [mem 0x00000000c0300000-0x00000000c0301000] uncached-minus
PAT: [mem 0x00000000cff00000-0x00000000cff01000] uncached-minus
PAT: [mem 0x00000000fe001000-0x00000000fe002000] uncached-minus
PAT: [mem 0x00000000fedcd000-0x00000000fedce000] uncached-minus
PAT: [mem 0x00000000fedcd000-0x00000000fedce000] uncached-minus
PAT: [mem 0x00000000feddd000-0x00000000fedde000] uncached-minus
PAT: [mem 0x00000000feddd000-0x00000000fedde000] uncached-minus
PAT: [mem 0x00000000ffff0000-0x00000000ffff2000] uncached-minus
PAT: [mem 0x0000004000000000-0x0000004010000000] write-combining
PAT: [mem 0x0000006000000000-0x0000006000200000] uncached-minus
PAT: [mem 0x0000006000800000-0x0000006001000000] uncached-minus
PAT: [mem 0x0000006001100000-0x0000006001110000] uncached-minus
PAT: [mem 0x0000006001110000-0x0000006001120000] uncached-minus
PAT: [mem 0x0000006001130000-0x0000006001134000] uncached-minus
PAT: [mem 0x0000006001134000-0x0000006001138000] uncached-minus
PAT: [mem 0x0000006001136000-0x0000006001137000] uncached-minus
PAT: [mem 0x000000600113d000-0x000000600113e000] uncached-minus
```

### 13900k
```txt
PAT memtype list:
PAT: [mem 0x0000000063e5e000-0x0000000063e5f000] write-back
PAT: [mem 0x0000000063e9c000-0x0000000063e9d000] write-back
PAT: [mem 0x000000006b524000-0x000000006b8f1000] write-back
PAT: [mem 0x000000006dc61000-0x000000006dc62000] write-back
PAT: [mem 0x00000000742d9000-0x00000000742da000] write-back
PAT: [mem 0x00000000742da000-0x00000000742db000] write-back
PAT: [mem 0x00000000742db000-0x00000000742dc000] write-back
PAT: [mem 0x00000000742dc000-0x00000000742dd000] write-back
PAT: [mem 0x0000000074405000-0x0000000074406000] write-back
PAT: [mem 0x0000000074406000-0x0000000074407000] write-back
PAT: [mem 0x0000000074407000-0x000000007440b000] write-back
PAT: [mem 0x000000007440b000-0x000000007440f000] write-back
PAT: [mem 0x000000007440f000-0x0000000074411000] write-back
PAT: [mem 0x0000000074411000-0x0000000074412000] write-back
PAT: [mem 0x0000000074412000-0x0000000074413000] write-back
PAT: [mem 0x0000000074413000-0x0000000074414000] write-back
PAT: [mem 0x0000000074414000-0x0000000074415000] write-back
PAT: [mem 0x0000000074415000-0x0000000074418000] write-back
PAT: [mem 0x0000000074418000-0x000000007441b000] write-back
PAT: [mem 0x000000007441b000-0x000000007441c000] write-back
PAT: [mem 0x000000007441c000-0x000000007441d000] write-back
PAT: [mem 0x000000007441d000-0x000000007441e000] write-back
PAT: [mem 0x000000007441f000-0x0000000074421000] write-back
PAT: [mem 0x0000000074421000-0x0000000074422000] write-back
PAT: [mem 0x0000000074422000-0x0000000074423000] write-back
PAT: [mem 0x0000000074423000-0x0000000074424000] write-back
PAT: [mem 0x0000000074424000-0x0000000074428000] write-back
PAT: [mem 0x0000000074428000-0x000000007442b000] write-back
PAT: [mem 0x000000007442b000-0x0000000074431000] write-back
PAT: [mem 0x0000000074431000-0x0000000074432000] write-back
PAT: [mem 0x0000000074433000-0x0000000074434000] write-back
PAT: [mem 0x0000000074434000-0x00000000744ad000] write-back
PAT: [mem 0x00000000744ad000-0x00000000744ae000] write-back
PAT: [mem 0x00000000744ae000-0x00000000744af000] write-back
PAT: [mem 0x00000000745d3000-0x00000000745d4000] write-back
PAT: [mem 0x00000000745d4000-0x00000000745d6000] write-back
PAT: [mem 0x00000000745d4000-0x00000000745d7000] write-back
PAT: [mem 0x00000000745d6000-0x00000000745d9000] write-back
PAT: [mem 0x00000000745ea000-0x00000000745eb000] write-back
PAT: [mem 0x000000007461c000-0x000000007461d000] write-back
PAT: [mem 0x000000007461d000-0x000000007461e000] write-back
PAT: [mem 0x000000007471e000-0x000000007471f000] write-back
PAT: [mem 0x0000000075d19000-0x0000000075d1b000] write-back
PAT: [mem 0x0000000080800000-0x0000000080801000] uncached-minus
PAT: [mem 0x0000000082080000-0x0000000082084000] uncached-minus
PAT: [mem 0x0000000082b00000-0x0000000082c00000] uncached-minus
PAT: [mem 0x0000000082c00000-0x0000000082c01000] uncached-minus
PAT: [mem 0x0000000082d00000-0x0000000082d02000] uncached-minus
PAT: [mem 0x0000000082d03000-0x0000000082d04000] uncached-minus
PAT: [mem 0x0000000082e00000-0x0000000082e10000] uncached-minus
PAT: [mem 0x0000000082e10000-0x0000000082e11000] uncached-minus
PAT: [mem 0x0000000082f00000-0x0000000082f02000] uncached-minus
PAT: [mem 0x0000000082f03000-0x0000000082f04000] uncached-minus
PAT: [mem 0x0000000083030000-0x0000000083032000] uncached-minus
PAT: [mem 0x0000000083032000-0x0000000083033000] uncached-minus
PAT: [mem 0x0000000083102000-0x0000000083103000] uncached-minus
PAT: [mem 0x00000000c0000000-0x00000000d0000000] uncached-minus
PAT: [mem 0x00000000c0008000-0x00000000c0009000] uncached-minus
PAT: [mem 0x00000000c0030000-0x00000000c0031000] uncached-minus
PAT: [mem 0x00000000c00a3000-0x00000000c00a4000] uncached-minus
PAT: [mem 0x00000000c00d0000-0x00000000c00d1000] uncached-minus
PAT: [mem 0x00000000c00d1000-0x00000000c00d2000] uncached-minus
PAT: [mem 0x00000000c00d2000-0x00000000c00d3000] uncached-minus
PAT: [mem 0x00000000c00d3000-0x00000000c00d4000] uncached-minus
PAT: [mem 0x00000000c00d8000-0x00000000c00d9000] uncached-minus
PAT: [mem 0x00000000c00d9000-0x00000000c00da000] uncached-minus
PAT: [mem 0x00000000c00da000-0x00000000c00db000] uncached-minus
PAT: [mem 0x00000000c00db000-0x00000000c00dc000] uncached-minus
PAT: [mem 0x00000000c00dc000-0x00000000c00dd000] uncached-minus
PAT: [mem 0x00000000c00dd000-0x00000000c00de000] uncached-minus
PAT: [mem 0x00000000c00de000-0x00000000c00df000] uncached-minus
PAT: [mem 0x00000000c00df000-0x00000000c00e0000] uncached-minus
PAT: [mem 0x00000000c00e0000-0x00000000c00e1000] uncached-minus
PAT: [mem 0x00000000c00e1000-0x00000000c00e2000] uncached-minus
PAT: [mem 0x00000000c00e2000-0x00000000c00e3000] uncached-minus
PAT: [mem 0x00000000c00e3000-0x00000000c00e4000] uncached-minus
PAT: [mem 0x00000000c00e4000-0x00000000c00e5000] uncached-minus
PAT: [mem 0x00000000c00e5000-0x00000000c00e6000] uncached-minus
PAT: [mem 0x00000000c00e6000-0x00000000c00e7000] uncached-minus
PAT: [mem 0x00000000c00e7000-0x00000000c00e8000] uncached-minus
PAT: [mem 0x00000000c00e8000-0x00000000c00e9000] uncached-minus
PAT: [mem 0x00000000c00e9000-0x00000000c00ea000] uncached-minus
PAT: [mem 0x00000000c00ea000-0x00000000c00eb000] uncached-minus
PAT: [mem 0x00000000c00eb000-0x00000000c00ec000] uncached-minus
PAT: [mem 0x00000000c00ec000-0x00000000c00ed000] uncached-minus
PAT: [mem 0x00000000c00ed000-0x00000000c00ee000] uncached-minus
PAT: [mem 0x00000000c00ee000-0x00000000c00ef000] uncached-minus
PAT: [mem 0x00000000c00ef000-0x00000000c00f0000] uncached-minus
PAT: [mem 0x00000000c0300000-0x00000000c0301000] uncached-minus
PAT: [mem 0x00000000c0400000-0x00000000c0401000] uncached-minus
PAT: [mem 0x00000000c0500000-0x00000000c0501000] uncached-minus
PAT: [mem 0x00000000c0600000-0x00000000c0601000] uncached-minus
PAT: [mem 0x00000000c0700000-0x00000000c0701000] uncached-minus
PAT: [mem 0x00000000cff00000-0x00000000cff01000] uncached-minus
PAT: [mem 0x00000000e0690000-0x00000000e06a0000] uncached-minus
PAT: [mem 0x00000000e06a0000-0x00000000e06b0000] uncached-minus
PAT: [mem 0x00000000e06b0000-0x00000000e06c0000] uncached-minus
PAT: [mem 0x00000000e06d0000-0x00000000e06e0000] uncached-minus
PAT: [mem 0x00000000e06e0000-0x00000000e06f0000] uncached-minus
PAT: [mem 0x00000000fe000000-0x00000000fe002000] uncached-minus
PAT: [mem 0x00000000fe001000-0x00000000fe002000] uncached-minus
PAT: [mem 0x00000000fed40000-0x00000000fed45000] uncached-minus
PAT: [mem 0x00000000fed90000-0x00000000fed91000] uncached-minus
PAT: [mem 0x00000000fed91000-0x00000000fed92000] uncached-minus
PAT: [mem 0x00000000fedcd000-0x00000000fedce000] uncached-minus
PAT: [mem 0x00000000fedcd000-0x00000000fedce000] uncached-minus
PAT: [mem 0x00000000feddd000-0x00000000fedde000] uncached-minus
PAT: [mem 0x00000000feddd000-0x00000000fedde000] uncached-minus
PAT: [mem 0x00000000ffff0000-0x00000000ffff2000] uncached-minus
PAT: [mem 0x0000004000000000-0x0000004010000000] write-combining
PAT: [mem 0x0000004017200000-0x0000004017201000] uncached
PAT: [mem 0x0000004017200000-0x0000004017201000] uncached
PAT: [mem 0x0000004017200000-0x0000004017201000] uncached
PAT: [mem 0x0000004017201000-0x0000004017202000] uncached
PAT: [mem 0x0000004017201000-0x0000004017202000] uncached
PAT: [mem 0x0000004017201000-0x0000004017202000] uncached
PAT: [mem 0x0000004017202000-0x0000004017203000] uncached
PAT: [mem 0x0000004017202000-0x0000004017203000] uncached
PAT: [mem 0x0000004017202000-0x0000004017203000] uncached
PAT: [mem 0x0000006012000000-0x0000006012200000] uncached-minus
PAT: [mem 0x0000006012800000-0x0000006013000000] uncached-minus
PAT: [mem 0x0000006013100000-0x0000006013110000] uncached-minus
PAT: [mem 0x0000006013114000-0x0000006013115000] uncached-minus
PAT: [mem 0x0000006013114000-0x0000006013115000] uncached-minus
PAT: [mem 0x0000006013116000-0x0000006013117000] uncached-minus
PAT: [mem 0x0000006013116000-0x0000006013117000] uncached-minus
PAT: [mem 0x0000006013116000-0x0000006013117000] uncached-minus
PAT: [mem 0x0000006013116000-0x0000006013117000] uncached-minus
PAT: [mem 0x0000006013118000-0x000000601311c000] uncached-minus
PAT: [mem 0x000000601311c000-0x0000006013120000] uncached-minus
PAT: [mem 0x000000601311e000-0x000000601311f000] uncached-minus
PAT: [mem 0x0000006013125000-0x0000006013126000] uncached-minus
```
### 2403-nix (kvm x86)

```txt
PAT memtype list:
PAT: [mem 0x00000000bffe0000-0x00000000bffe3000] write-back
PAT: [mem 0x00000000bffe2000-0x00000000bffe3000] write-back
PAT: [mem 0x00000000fe0c0000-0x00000000fe0c1000] uncached-minus
PAT: [mem 0x00000000fe0c1000-0x00000000fe0c2000] uncached-minus
PAT: [mem 0x00000000fe0c2000-0x00000000fe0c3000] uncached-minus
PAT: [mem 0x00000000fe0c3000-0x00000000fe0c4000] uncached-minus
PAT: [mem 0x00000000fe0c4000-0x00000000fe0c5000] uncached-minus
PAT: [mem 0x00000000fe0c5000-0x00000000fe0c6000] uncached-minus
PAT: [mem 0x00000000fe0c6000-0x00000000fe0c7000] uncached-minus
PAT: [mem 0x00000000fe0c7000-0x00000000fe0c8000] uncached-minus
PAT: [mem 0x00000000fe0c8000-0x00000000fe0c9000] uncached-minus
PAT: [mem 0x00000000fed00000-0x00000000fed01000] uncached-minus
PAT: [mem 0x00000000fed00000-0x00000000fed01000] uncached-minus
PAT: [mem 0x0000380000000000-0x0000380000001000] uncached-minus
PAT: [mem 0x0000380000001000-0x0000380000002000] uncached-minus
PAT: [mem 0x0000380000002000-0x0000380000003000] uncached-minus
PAT: [mem 0x0000380000003000-0x0000380000004000] uncached-minus
PAT: [mem 0x0000380000004000-0x0000380000005000] uncached-minus
PAT: [mem 0x0000380000005000-0x0000380000006000] uncached-minus
PAT: [mem 0x0000380000006000-0x0000380000007000] uncached-minus
PAT: [mem 0x0000380000007000-0x0000380000008000] uncached-minus
PAT: [mem 0x0000380000008000-0x0000380000009000] uncached-minus
PAT: [mem 0x0000380000009000-0x000038000000a000] uncached-minus
PAT: [mem 0x000038000000a000-0x000038000000b000] uncached-minus
PAT: [mem 0x000038000000b000-0x000038000000c000] uncached-minus
PAT: [mem 0x000038000000c000-0x000038000000d000] uncached-minus
PAT: [mem 0x000038000000d000-0x000038000000e000] uncached-minus
PAT: [mem 0x000038000000e000-0x000038000000f000] uncached-minus
PAT: [mem 0x000038000000f000-0x0000380000010000] uncached-minus
PAT: [mem 0x0000380000010000-0x0000380000011000] uncached-minus
PAT: [mem 0x0000380000011000-0x0000380000012000] uncached-minus
PAT: [mem 0x0000380000012000-0x0000380000013000] uncached-minus
PAT: [mem 0x0000380000013000-0x0000380000014000] uncached-minus
PAT: [mem 0x0000380000014000-0x0000380000015000] uncached-minus
PAT: [mem 0x0000380000015000-0x0000380000016000] uncached-minus
PAT: [mem 0x0000380000016000-0x0000380000017000] uncached-minus
PAT: [mem 0x0000380000017000-0x0000380000018000] uncached-minus
PAT: [mem 0x0000380000018000-0x0000380000019000] uncached-minus
PAT: [mem 0x0000380000019000-0x000038000001a000] uncached-minus
PAT: [mem 0x000038000001a000-0x000038000001b000] uncached-minus
PAT: [mem 0x000038000001b000-0x000038000001c000] uncached-minus
PAT: [mem 0x000038000001c000-0x000038000001d000] uncached-minus
PAT: [mem 0x000038000001d000-0x000038000001e000] uncached-minus
PAT: [mem 0x000038000001e000-0x000038000001f000] uncached-minus
PAT: [mem 0x000038000001f000-0x0000380000020000] uncached-minus
PAT: [mem 0x0000380000020000-0x0000380000021000] uncached-minus
PAT: [mem 0x0000380000021000-0x0000380000022000] uncached-minus
PAT: [mem 0x0000380000022000-0x0000380000023000] uncached-minus
PAT: [mem 0x0000380000023000-0x0000380000024000] uncached-minus
PAT: [mem 0x0000380000024000-0x0000380000025000] uncached-minus
PAT: [mem 0x0000380000025000-0x0000380000026000] uncached-minus
PAT: [mem 0x0000380000026000-0x0000380000027000] uncached-minus
PAT: [mem 0x0000380000027000-0x0000380000028000] uncached-minus
PAT: [mem 0x0000380000028000-0x0000380000029000] uncached-minus
PAT: [mem 0x0000380000029000-0x000038000002a000] uncached-minus
PAT: [mem 0x000038000002a000-0x000038000002b000] uncached-minus
PAT: [mem 0x000038000002b000-0x000038000002c000] uncached-minus
```

## MTRR
MTRR use is replaced on modern x86 hardware with PAT. [^2]

Typically, the BIOS (basic input/output system) software configures the MTRRs.[^3]

section 11.3 "Methods of Caching Available"

The MTRRs are useful for statically
describing memory types for physical ranges, and are typically set up by the system BIOS. The PAT extends the
functions of the PCD and PWT bits in page tables to allow all five of the memory types that can be assigned with the
MTRRs (plus one additional memory type) to also be assigned dynamically to pages of the linear address space.[^4]


[^2]: https://www.kernel.org/doc/html/latest/x86/mtrr.html
[^3]: intel manual volume 11.11
[^4]: intel manual volume 11.12

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
