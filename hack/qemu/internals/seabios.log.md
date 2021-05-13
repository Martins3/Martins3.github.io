```
SeaBIOS (version rel-1.14.0-14-g748d619)
BUILD: gcc: (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0 binutils: (GNU Binutils for Ubuntu) 2.34
No Xen hypervisor found.
enabling shadow ram
Running on QEMU (i440fx)
Running on KVM Found QEMU fw_cfg
QEMU fw_cfg DMA interface supported
qemu/e820: addr 0x00000000feffc000 len 0x0000000000004000 [reserved]
qemu/e820: addr 0x0000000000000000 len 0x00000000c0000000 [RAM]
qemu/e820: addr 0x0000000100000000 len 0x0000000140000000 [RAM]
qemu/e820: RamSize: 0xc0000000
qemu/e820: RamSizeOver4G: 0x0000000140000000
malloc preinit
Relocating init from 0x000d38e0 to 0xbffaad60 (size 86528)
malloc init
Add romfile: etc/show-boot-menu (size=2)
Add romfile: etc/irq0-override (size=1)
Add romfile: etc/max-cpus (size=2)
Add romfile: etc/numa-cpu-map (size=8)
Add romfile: etc/numa-nodes (size=0)
Add romfile: bios-geometry (size=0)
Add romfile: bootorder (size=31)
Add romfile: etc/acpi/rsdp (size=20)
Add romfile: etc/acpi/tables (size=131072)
Add romfile: etc/boot-fail-wait (size=4)
Add romfile: etc/e820 (size=60)
Add romfile: etc/msr_feature_control (size=8)
Add romfile: etc/smbios/smbios-anchor (size=31)
Add romfile: etc/smbios/smbios-tables (size=354)
Add romfile: etc/system-states (size=6)
Add romfile: etc/table-loader (size=4096)
Add romfile: etc/tpm/log (size=0)
Add romfile: genroms/kvmvapic.bin (size=9216)
Add romfile: genroms/linuxboot_dma.bin (size=1536)
Moving pm_base to 0x600
init ivt
init bda
boot order:
1: /rom@genroms/linuxboot_dma.bin
init bios32
init PMM
init PNPBIOS table
init keyboard
init mouse
init pic
math cp init
kvmclock: at 0xe8200 (msr 0x4b564d01)
kvmclock: stable tsc, 1992 MHz
CPU Mhz=1992 (kvmclock)
pci setup
=== PCI bus & bridge init ===
PCI: pci_bios_init_bus_rec bus = 0x0
=== PCI device probing ===
PCI probe
PCI device 00:00.0 (vd=8086:1237 c=0600)
PCI device 00:01.0 (vd=8086:7000 c=0601)
PCI device 00:01.1 (vd=8086:7010 c=0101)
PCI device 00:01.3 (vd=8086:7113 c=0680)
PCI device 00:02.0 (vd=1af4:1050 c=0300)
PCI device 00:03.0 (vd=8086:100e c=0200)
Found 6 PCI devices (max PCI bus is 00)
=== PCI new allocation pass #1 ===
PCI: check devices
=== PCI new allocation pass #2 ===
PCI: IO: c000 - c04f
PCI: 32: 00000000c0000000 - 00000000fec00000
PCI: map device bdf=00:03.0  bar 1, addr 0000c000, size 00000040 [io]
PCI: map device bdf=00:01.1  bar 4, addr 0000c040, size 00000010 [io]
PCI: map device bdf=00:03.0  bar 6, addr feb80000, size 00040000 [mem]
PCI: map device bdf=00:03.0  bar 0, addr febc0000, size 00020000 [mem]
PCI: map device bdf=00:02.0  bar 6, addr febe0000, size 00010000 [mem]
PCI: map device bdf=00:02.0  bar 4, addr febf0000, size 00001000 [mem]
PCI: map device bdf=00:02.0  bar 0, addr fe000000, size 00800000 [prefmem]
PCI: map device bdf=00:02.0  bar 2, addr fe800000, size 00004000 [prefmem]
PCI: init bdf=00:00.0 id=8086:1237
PCI: init bdf=00:01.0 id=8086:7000
PIIX3/PIIX4 init: elcr=00 0c
PCI: init bdf=00:01.1 id=8086:7010
PCI: init bdf=00:01.3 id=8086:7113
PCI: init bdf=00:02.0 id=1af4:1050
PCI: init bdf=00:03.0 id=8086:100e
PCI: Using 00:02.0 for primary VGA
init smm
init mtrr
Found 1 cpu(s) max supported 1 cpu(s)
init PIR table
Copying PIR from 0xbffbfc40 to 0x000f5040
init MPTable
Copying MPTABLE from 0x00006e14/bffa1aa0 to 0x000f4f50
Copying SMBIOS entry point from 0x00006e14 to 0x000f4d80
load ACPI tables
rsdp=0x000f4d60
rsdt=0xbffe18fe
xsdt=0x00000000
table(50434146)=0xbffe17b2 (via rsdt)
ACPI: parse DSDT at 0xbffe0040 (len 6002)
init timer
rsdp=0x000f4d60
rsdt=0xbffe18fe
xsdt=0x00000000
no table 324d5054 found
rsdp=0x000f4d60
rsdt=0xbffe18fe
xsdt=0x00000000
no table 41504354 found
Scan for VGA option rom
Attempting to init PCI bdf 00:02.0 (vd 1af4:1050)
Copying option rom (size 39424) from 0xfebe0000 to 0x000c0000
Running option rom at c000:0003
Start SeaVGABIOS (version rel-1.14.0-0-g155821a1990b-prebuilt.qemu.org)
VGABUILD: gcc: (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39) binutils: version 2.27-43.base.el7_8.1
enter vga_post:
   a=00000010  b=0000ffff  c=00000000  d=0000ffff ds=0000 es=f000 ss=0000
  si=00000000 di=00005420 bp=00000000 sp=00006d62 cs=f000 ip=cb15  f=0000
VBE DISPI: bdf 00:02.0, bar 0
VBE DISPI: lfb_addr=fe000000, size 8 MB
Removing mode 189
Removing mode 18b
Removing mode 18c
Removing mode 197
Removing mode 198
Attempting to allocate 512 bytes lowmem via pmm call to f000:cbcf
pmm call arg1=0
pmm00: length=20 handle=ffffffff flags=9
VGA stack allocated at e8000
Turning on vga text mode console
set VGA mode 3
SeaBIOS (version rel-1.14.0-14-g748d619)
init usb
init ps2port
/bffa7000\ Start thread
init floppy drives
Searching bootorder for: /pci@i0cf8/isa@1/fdc@03f0/floppy@0
Registering bootable: Floppy [drive A] (type:1 prio:102 data:f4d30)
init hard drives
ATA controller 1 at 1f0/3f4/0 (irq 14 dev 9)
/bffa6000\ Start thread
ATA controller 2 at 170/374/0 (irq 15 dev 9)
/bffa5000\ Start thread
init ahci
Searching bootorder for: HALT
init virtio-blk
init virtio-scsi
init lsi53c895a
init esp
init megasas
init pvscsi
init MPT
init nvme
init lpt
Found 1 lpt ports
init serial
Found 1 serial ports
|bffa6000| ata0-0: QEMU HARDDISK ATA-7 Hard-Disk (1024 GiBytes)
|bffa6000| Searching bootorder for: /pci@i0cf8/*@1,1/drive@0/disk@0
|bffa6000| Searching bios-geometry for: /pci@i0cf8/*@1,1/drive@0/disk@0
|bffa6000| Registering bootable: ata0-0: QEMU HARDDISK ATA-7 Hard-Disk (1024 GiBytes) (type:2 prio:101 data:f4cc0)
\bffa6000/ End thread
|bffa5000| DVD/CD [ata1-0: QEMU DVD-ROM ATAPI-4 DVD/CD]
|bffa5000| Searching bootorder for: /pci@i0cf8/*@1,1/drive@1/disk@0
|bffa5000| Searching bios-geometry for: /pci@i0cf8/*@1,1/drive@1/disk@0
|bffa5000| Device reports MEDIUM NOT PRESENT
|bffa5000| Registering bootable: DVD/CD [ata1-0: QEMU DVD-ROM ATAPI-4 DVD/CD] (type:3 prio:103 data:f4c90)
\bffa5000/ End thread
|bffa7000| PS2 keyboard initialized
\bffa7000/ End thread
All threads complete.
Scan for option roms
Attempting to init PCI bdf 00:00.0 (vd 8086:1237)
Attempting to init PCI bdf 00:01.0 (vd 8086:7000)
Attempting to init PCI bdf 00:01.1 (vd 8086:7010)
Attempting to init PCI bdf 00:01.3 (vd 8086:7113)
Attempting to init PCI bdf 00:03.0 (vd 8086:100e)
Copying option rom (size 68608) from 0xfeb80000 to 0x000ca000
Running option rom at ca00:0003
pmm call arg1=1
pmm01: handle=18ae1000
pmm call arg1=0
pmm00: length=1100 handle=18ae1000 flags=2
pmm call arg1=1
pmm01: handle=18ae200a
pmm call arg1=0
pmm00: length=a000 handle=18ae200a flags=2
Running option rom at cb00:0003
Searching bootorder for: /pci@i0cf8/*@3
Registering bootable: iPXE (PCI 00:03.0) (type:128 prio:9999 data:ca000385)
Searching bootorder for: /rom@genroms/linuxboot_dma.bin
Registering bootable: Linux loader DMA (type:128 prio:1 data:cb000054)
Searching bootorder for: /rom@genroms/kvmvapic.bin
Registering bootable: Legacy option rom (type:129 prio:101 data:cb800003)
Searching bootorder for: HALT
Mapping hd drive 0x000f4cc0 to 0
drive 0x000f4cc0: PCHS=16383/16/63 translation=lba LCHS=1024/255/63 s=2147483648
Running option rom at cb80:0003
Mapping floppy drive 0x000f4d30
Mapping cd drive 0x000f4c90
finalize PMM
malloc finalize
Space available for UMB: ce000-e7000, f48c0-f4c60
Returned 131072 bytes of ZoneHigh
e820 map has 8 items:
  0: 0000000000000000 - 000000000009fc00 = 1 RAM
  1: 000000000009fc00 - 00000000000a0000 = 2 RESERVED
  2: 00000000000f0000 - 0000000000100000 = 2 RESERVED
  3: 0000000000100000 - 00000000bffe0000 = 1 RAM
  4: 00000000bffe0000 - 00000000c0000000 = 2 RESERVED
  5: 00000000feffc000 - 00000000ff000000 = 2 RESERVED
  6: 00000000fffc0000 - 0000000100000000 = 2 RESERVED
  7: 0000000100000000 - 0000000240000000 = 1 RAM
locking shadow ram
Jump to int19
enter handle_19:
  NULL
Booting from ROM...
Booting from cb00:0054
unimplemented handle_15XX:330:
   a=0000ec00  b=00000002  c=00000000  d=00000000 ds=1000 es=1000 ss=d900
  si=00000000 di=00000000 bp=00000000 sp=0000faae cs=1000 ip=0300  f=0003
unimplemented handle_16XX:224:
   a=00000305  b=00000000  c=00000000  d=00000000 ds=1000 es=1000 ss=d900
  si=00000000 di=00000000 bp=00000000 sp=0000faae cs=1000 ip=0300  f=0003
unimplemented handle_15XX:330:
   a=0000e980  b=00000000  c=00000000  d=47534943 ds=1000 es=1000 ss=d900
  si=00000000 di=00000000 bp=00000000 sp=0000faae cs=1000 ip=0300  f=0003
VBE mode info request: 100
VBE mode info request: 101
VBE mode info request: 102
VBE mode info request: 103
VBE mode info request: 104
VBE mode info request: 105
VBE mode info request: 106
VBE mode info request: 107
VBE mode info request: 10d
VBE mode info request: 10e
VBE mode info request: 10f
VBE mode info request: 110
VBE mode info request: 111
VBE mode info request: 112
VBE mode info request: 113
VBE mode info request: 114
VBE mode info request: 115
VBE mode info request: 116
VBE mode info request: 117
VBE mode info request: 118
VBE mode info request: 119
VBE mode info request: 11a
VBE mode info request: 11b
VBE mode info request: 11c
VBE mode info request: 11d
VBE mode info request: 11e
VBE mode info request: 11f
VBE mode info request: 140
VBE mode info request: 141
VBE mode info request: 142
VBE mode info request: 143
VBE mode info request: 144
VBE mode info request: 145
VBE mode info request: 146
VBE mode info request: 147
VBE mode info request: 148
VBE mode info request: 149
VBE mode info request: 14a
VBE mode info request: 14b
VBE mode info request: 14c
VBE mode info request: 175
VBE mode info request: 176
VBE mode info request: 177
VBE mode info request: 178
VBE mode info request: 179
VBE mode info request: 17a
VBE mode info request: 17b
VBE mode info request: 17c
VBE mode info request: 17d
VBE mode info request: 17e
VBE mode info request: 17f
VBE mode info request: 180
VBE mode info request: 181
VBE mode info request: 182
VBE mode info request: 183
VBE mode info request: 184
VBE mode info request: 185
VBE mode info request: 186
VBE mode info request: 187
VBE mode info request: 188
VBE mode info request: 18a
VBE mode info request: 18d
VBE mode info request: 18e
VBE mode info request: 18f
VBE mode info request: 190
VBE mode info request: 191
VBE mode info request: 192
VBE mode info request: 193
VBE mode info request: 194
VBE mode info request: 195
VBE mode info request: 196
VBE mode info request: 0
VBE mode info request: 1
VBE mode info request: 2
VBE mode info request: 3
VBE mode info request: 4
VBE mode info request: 5
VBE mode info request: 6
VBE mode info request: 7
VBE mode info request: d
VBE mode info request: e
VBE mode info request: f
VBE mode info request: 10
VBE mode info request: 11
VBE mode info request: 12
VBE mode info request: 13
VBE mode info request: 6a
set VGA mode 3
```
