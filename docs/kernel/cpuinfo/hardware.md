

通过这个评论可以知道，
https://stackoverflow.com/questions/13965178/how-do-i-disable-avx-instructions-on-a-linux-computer

clearcpuid=156 来屏蔽 flags ，但是观察其实现，并没有什么特别的

的确是 invalid code 的错误:
```txt
[   22.507434] traps: wc[5360] trap invalid opcode ip:4d6b4b sp:7ffc29d5aaa0 error:0 in coreutils[408000+cf000]
[   28.486690] input: WH-1000XM3 (AVRCP) as /devices/virtual/input/input27
[   48.593359] traps: .epiphany-searc[5754] trap invalid opcode ip:7f8d0312b334 sp:7fffce48c438 error:0 in libatomic.so.1.2.0[7f8d03129000+3000]
[   48.632515] traps: .gnome-photos-w[5752] trap invalid opcode ip:7fa57966dde9 sp:7fff3c182808 error:0 in x86-64-v3-CIE.so[7fa579668000+8000]
[   48.737020] traps: .org.gnome.Char[5746] trap invalid opcode ip:7f615e4a5910 sp:7ffcaff140e8 error:0 in libgtk-4.so.1.800.2[7f615e0b4000+3f2000]
[   79.039518] traps: .epiphany-searc[7307] trap invalid opcode ip:7efd81803334 sp:7ffdfb8e6ad8 error:0 in libatomic.so.1.2.0[7efd81801000+3000]
[   79.087522] traps: .gnome-photos-w[7305] trap invalid opcode ip:7f99ce9b6de9 sp:7fffbebf8d08 error:0 in x86-64-v3-CIE.so[7f99ce9b1000+8000]
[   79.287229] traps: .org.gnome.Char[7299] trap invalid opcode ip:7f60296a5910 sp:7ffc27cecd38 error:0 in libgtk-4.so.1.800.2[7f60292b4000+3f2000]
[   79.294773] traps: .gnome-photos-w[7721] trap invalid opcode ip:7ff0618fbde9 sp:7fffb32fe118 error:0 in x86-64-v3-CIE.so[7ff0618f6000+8000]
[  116.673961] traps: wc[8899] trap invalid opcode ip:4d6b4b sp:7ffe671832c0 error:0 in coreutils[408000+cf000]
[  116.720285] traps: wc[8937] trap invalid opcode ip:4d6b4b sp:7ffd931c7820 error:0 in coreutils[408000+cf000]
[  116.769937] traps: wc[8990] trap invalid opcode ip:4d6b4b sp:7ffe8de0f480 error:0 in coreutils[408000+cf000]
[  116.796237] traps: wc[9122] trap invalid opcode ip:4d6b4b sp:7fffa70e69c0 error:0 in coreutils[408000+cf000]
[  129.641439] traps: 01-hello[9994] trap invalid opcode ip:401140 sp:7ffea850b140 error:0 in 01-hello[401000+1000]
[  140.471273] traps: .epiphany-searc[10105] trap invalid opcode ip:7f25f32d3334 sp:7fff84375ea8 error:0 in libatomic.so.1.2.0[7f25f32d1000+3000]
[  140.513184] traps: .gnome-photos-w[10103] trap invalid opcode ip:7fb1f485bde9 sp:7ffe944f3b18 error:0 in x86-64-v3-CIE.so[7fb1f4856000+8000]
[  140.729860] traps: .org.gnome.Char[10097] trap invalid opcode ip:7fa7424a5910 sp:7ffda737bcf8 error:0 in libgtk-4.so.1.800.2[7fa7420b4000+3f2000]
[  140.737712] traps: .gnome-photos-w[10476] trap invalid opcode ip:7f672d9aede9 sp:7ffd811f0818 error:0 in x86-64-v3-CIE.so[7f672d9a9000+8000]
[  341.282755] wlo1: Connection to AP 78:57:73:58:c8:30 lost
[  342.378856] wlo1: authenticate with 78:57:73:4d:5e:10
[  342.384498] wlo1: send auth to 78:57:73:4d:5e:10 (try 1/3)
[  342.410897] wlo1: authenticated
[  342.411937] wlo1: associate with 78:57:73:4d:5e:10 (try 1/3)
[  342.416201] wlo1: RX AssocResp from 78:57:73:4d:5e:10 (capab=0x1511 status=0 aid=5)
[  342.421935] wlo1: associated
[  342.470154] wlo1: Limiting TX power to 20 (23 - 3) dBm as advertised by 78:57:73:4d:5e:10
[ 1041.424378] traps: wc[20526] trap invalid opcode ip:4dee4b sp:7ffe1514cc60 error:0 in coreutils[407000+d9000]
[ 1073.208551] traps: wc[22373] trap invalid opcode ip:4d6b4b sp:7ffdb619ecc0 error:0 in coreutils[408000+cf000]
[ 1124.146359] traps: wc[23303] trap invalid opcode ip:4d6b4b sp:7ffe37ab8960 error:0 in coreutils[408000+cf000]
[ 1133.657698] traps: wc[24218] trap invalid opcode ip:4dee4b sp:7fffcdcaef40 error:0 in coreutils[407000+d9000]
[ 1139.262500] traps: 01-hello[24450] trap invalid opcode ip:401544 sp:7fff203c6a40 error:0 in 01-hello[401000+8c000]
[ 1195.931841] traps: wc[24863] trap invalid opcode ip:4d6b4b sp:7ffcca8f5060 error:0 in coreutils[408000+cf000]
[ 1375.392674] traps: 01-hello[25643] trap invalid opcode ip:40154c sp:7ffec69d1ae0 error:0 in 01-hello[401000+8c000]
[ 1404.288893] traps: 01-hello[25782] trap invalid opcode ip:40154c sp:7fff3d9592c0 error:0 in 01-hello[401000+8c000]
[ 1478.821069] traps: 01-hello[26143] trap invalid opcode ip:40154c sp:7ffed98bcda0 error:0 in 01-hello[401000+8c000]
```
