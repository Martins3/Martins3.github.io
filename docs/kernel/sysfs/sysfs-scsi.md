# sysfs scsi
## sysfs 的观察 scsi 的内容
```txt
➜  ~ tree /sys/bus/scsi
/sys/bus/scsi
├── devices
│   ├── 0:0:0:0 -> ../../../devices/pci0000:00/0000:00:0c.0/host0/target0:0:0/0:0:0:0
│   ├── 0:0:1:0 -> ../../../devices/pci0000:00/0000:00:0c.0/host0/target0:0:1/0:0:1:0
│   ├── 1:0:0:0 -> ../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0/1:0:0:0
│   ├── 1:0:2:3 -> ../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:2/1:0:2:3
│   ├── 2:0:0:0 -> ../../../devices/pseudo_0/adapter0/host2/target2:0:0/2:0:0:0
│   ├── 3:0:0:0 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:0/3:0:0:0
│   ├── 3:0:1:0 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:1/3:0:1:0
│   ├── host0 -> ../../../devices/pci0000:00/0000:00:0c.0/host0
│   ├── host1 -> ../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1
│   ├── host2 -> ../../../devices/pseudo_0/adapter0/host2
│   ├── host3 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host3
│   ├── host4 -> ../../../devices/pci0000:00/0000:00:01.1/ata2/host4
│   ├── target0:0:0 -> ../../../devices/pci0000:00/0000:00:0c.0/host0/target0:0:0
│   ├── target0:0:1 -> ../../../devices/pci0000:00/0000:00:0c.0/host0/target0:0:1
│   ├── target1:0:0 -> ../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0
│   ├── target1:0:2 -> ../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:2
│   ├── target2:0:0 -> ../../../devices/pseudo_0/adapter0/host2/target2:0:0
│   ├── target3:0:0 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:0
│   └── target3:0:1 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:1
├── drivers
│   ├── sd
│   │   ├── 0:0:0:0 -> ../../../../devices/pci0000:00/0000:00:0c.0/host0/target0:0:0/0:0:0:0
│   │   ├── 0:0:1:0 -> ../../../../devices/pci0000:00/0000:00:0c.0/host0/target0:0:1/0:0:1:0
│   │   ├── 1:0:0:0 -> ../../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0/1:0:0:0
│   │   ├── 1:0:2:3 -> ../../../../devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:2/1:0:2:3
│   │   ├── 2:0:0:0 -> ../../../../devices/pseudo_0/adapter0/host2/target2:0:0/2:0:0:0
│   │   ├── 3:0:0:0 -> ../../../../devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:0/3:0:0:0
│   │   ├── 3:0:1:0 -> ../../../../devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:1/3:0:1:0
│   │   ├── bind
│   │   ├── uevent
│   │   └── unbind
│   └── sr
│       ├── bind
│       ├── uevent
│       └── unbind
├── drivers_autoprobe
├── drivers_probe
└── uevent
```

```txt
[root@localhost scsi]# tree
.
├── devices
│   ├── 0:0:0:0 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0/target0:0:0/0:0:0:0
│   ├── 24:0:0:0 -> ../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24/target24:0:0/24:0:0:0
│   ├── 49:0:0:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:1/end_device-49:1/target49:0:0/49:0:0:0
│   ├── 49:0:1:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:2/end_device-49:2/target49:0:1/49:0:1:0
│   ├── 49:0:2:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:3/end_device-49:3/target49:0:2/49:0:2:0
│   ├── 49:0:3:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:4/end_device-49:4/target49:0:3/49:0:3:0
│   ├── 49:0:4:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:5/end_device-49:5/target49:0:4/49:0:4:0
│   ├── 49:2:0:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/target49:2:0/49:2:0:0
│   ├── host0 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0
│   ├── host1 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata2/host1
│   ├── host10 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata11/host10
│   ├── host11 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata12/host11
│   ├── host12 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata13/host12
│   ├── host13 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata14/host13
│   ├── host14 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata15/host14
│   ├── host15 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata16/host15
│   ├── host16 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata17/host16
│   ├── host17 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata18/host17
│   ├── host18 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata19/host18
│   ├── host19 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata20/host19
│   ├── host2 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata3/host2
│   ├── host20 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata21/host20
│   ├── host21 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata22/host21
│   ├── host22 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata23/host22
│   ├── host23 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata24/host23
│   ├── host24 -> ../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24
│   ├── host25 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata25/host25
│   ├── host26 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata26/host26
│   ├── host27 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata27/host27
│   ├── host28 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata28/host28
│   ├── host29 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata29/host29
│   ├── host3 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata4/host3
│   ├── host30 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata30/host30
│   ├── host31 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata31/host31
│   ├── host32 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata32/host32
│   ├── host33 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata33/host33
│   ├── host34 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata34/host34
│   ├── host35 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata35/host35
│   ├── host36 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata36/host36
│   ├── host37 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata37/host37
│   ├── host38 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata38/host38
│   ├── host39 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata39/host39
│   ├── host4 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata5/host4
│   ├── host40 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata40/host40
│   ├── host41 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata41/host41
│   ├── host42 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata42/host42
│   ├── host43 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata43/host43
│   ├── host44 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata44/host44
│   ├── host45 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata45/host45
│   ├── host46 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata46/host46
│   ├── host47 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata47/host47
│   ├── host48 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata48/host48
│   ├── host49 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49
│   ├── host5 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata6/host5
│   ├── host6 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata7/host6
│   ├── host7 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata8/host7
│   ├── host8 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata9/host8
│   ├── host9 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata10/host9
│   ├── target0:0:0 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0/target0:0:0
│   ├── target24:0:0 -> ../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24/target24:0:0
│   ├── target49:0:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:1/end_device-49:1/target49:0:0
│   ├── target49:0:1 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:2/end_device-49:2/target49:0:1
│   ├── target49:0:2 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:3/end_device-49:3/target49:0:2
│   ├── target49:0:3 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:4/end_device-49:4/target49:0:3
│   ├── target49:0:4 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:5/end_device-49:5/target49:0:4
│   └── target49:2:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/target49:2:0
├── drivers
│   ├── sd
│   │   ├── 0:0:0:0 -> ../../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0/target0:0:0/0:0:0:0
│   │   ├── 49:0:0:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:1/end_device-49:1/target49:0:0/49:0:0:0
│   │   ├── 49:0:1:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:2/end_device-49:2/target49:0:1/49:0:1:0
│   │   ├── 49:0:2:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:3/end_device-49:3/target49:0:2/49:0:2:0
│   │   ├── 49:0:3:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:4/end_device-49:4/target49:0:3/49:0:3:0
│   │   ├── bind
│   │   ├── module -> ../../../../module/sd_mod
│   │   ├── uevent
│   │   └── unbind
│   ├── ses
│   │   ├── 49:0:4:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:5/end_device-49:5/target49:0:4/49:0:4:0
│   │   ├── bind
│   │   ├── module -> ../../../../module/ses
│   │   ├── uevent
│   │   └── unbind
│   └── sr
│       ├── 24:0:0:0 -> ../../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24/target24:0:0/24:0:0:0
│       ├── bind
│       ├── module -> ../../../../module/sr_mod
│       ├── uevent
│       └── unbind
├── drivers_autoprobe
├── drivers_probe
└── uevent
```

## cat /proc/scsi/scsi
1. QEMU
```txt
Attached devices:
Host: scsi0 Channel: 00 Id: 00 Lun: 00
  Vendor: QEMU     Model: QEMU DVD-ROM     Rev: 2.5+
  Type:   CD-ROM                           ANSI  SCSI revision: 05
```

2. 小米笔记本


3. amd
```txt
[root@localhost ~]# cat /proc/scsi/scsi
Attached devices:
Host: scsi0 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: INTEL SSDSC2KB24 Rev: 0132
  Type:   Direct-Access                    ANSI  SCSI revision: 05
Host: scsi49 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 01 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 02 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 03 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 04 Lun: 00
  Vendor: INSPUR   Model: Smart Adapter    Rev: 3.53
  Type:   Enclosure                        ANSI  SCSI revision: 05
Host: scsi49 Channel: 02 Id: 00 Lun: 00
  Vendor: INSPUR   Model: 8222-SHBA        Rev: 3.53
  Type:   RAID                             ANSI  SCSI revision: 05
Host: scsi24 Channel: 00 Id: 00 Lun: 00
  Vendor: Byosoft  Model: Virtual CDROM    Rev: 0502
  Type:   CD-ROM                           ANSI  SCSI revision: 02
```

4. 13900k

```txt
Attached devices:
Host: scsi6 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: ZHITAI SC001 Act Rev: 6200
  Type:   Direct-Access                    ANSI  SCSI revision: 05  # 固态盘
Host: scsi7 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: WDC WD20EZBX-00A Rev: 1A01
  Type:   Direct-Access                    ANSI  SCSI revision: 05  # 机械盘
Host: scsi8 Channel: 00 Id: 00 Lun: 00                              # usb
  Vendor: VendorCo Model: ProductCode      Rev: 2.00
  Type:   Direct-Access                    ANSI  SCSI revision: 04
```

## lsscsi

1. QEMU

```txt
[nix-shell:/sys/bus/scsi]$ lsscsi
[0:0:0:0]    cd/dvd  QEMU     QEMU DVD-ROM     2.5+  /dev/sr0
```

2. mi

```txt
[N:0:0:1] disk ZHITAI TiPlus5000 512GB__1 /dev/nvme0n1
[N:1:2:1] disk SAMSUNG MZVLW256HEHP-00000__1 /dev/nvme1n1
```

3. surplus

```txt
[root@localhost ~]# lsscsi
[0:0:0:0]    disk    ATA      INTEL SSDSC2KB24 0132  /dev/sda
[24:0:0:0]   cd/dvd  Byosoft  Virtual CDROM    0502  /dev/sr0
[49:0:0:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sdb
[49:0:1:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sdc
[49:0:2:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sdd
[49:0:3:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sde
[49:0:4:0]   enclosu INSPUR   Smart Adapter    3.53  -
[49:2:0:0]   storage INSPUR   8222-SHBA        3.53  -
[N:0:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme0n1
[N:1:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme1n1
```

4. 13900k

```txt
🧀  lsscsi
[6:0:0:0]    disk    ATA      ZHITAI SC001 Act 6200  /dev/sdb
[7:0:0:0]    disk    ATA      WDC WD20EZBX-00A 1A01  /dev/sda
[8:0:0:0]    disk    VendorCo ProductCode      2.00  /dev/sdc  # 这是个 360 U 盘
[N:0:0:1]    disk    ZHITAI TiPlus7100 1TB__1                   /dev/nvme0n1
[N:1:0:1]    disk    ZHITAI TiPro7000 1TB__1                    /dev/nvme1n1
[N:2:0:1]    disk    Fanxiang S790 4TB__1                       /dev/nvme2n1
```

## scsi 盘的状态控制

```txt
/sys/block/sdc/device/state
```

1. 如果 open 之后，然后将 state 设置为 offline ，那么最后可以将
2. 先 offline 掉，然后拔掉盘，会出现如下日志，这个日志和 offline 没有关系，总是会出现。
```txt
[   32.503229] sd 0:0:3:4: [sdc] Synchronizing SCSI cache
[   32.503667] sd 0:0:3:4: [sdc] Synchronize Cache(10) failed: Result: hostbyte=DID_BAD_TARGET driverbyte=DRIVER_OK
```

可见，offline 之后，对于热拔是没有影响的

## scsi 额外提供了一个 timeout 接口，但是是一样的
```c
/*
 * TODO: can we make these symlinks to the block layer ones?
 */
static ssize_t
sdev_show_timeout (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev;
	sdev = to_scsi_device(dev);
	return snprintf(buf, 20, "%d\n", sdev->request_queue->rq_timeout / HZ);
}

static ssize_t
sdev_store_timeout (struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	struct scsi_device *sdev;
	int timeout;
	sdev = to_scsi_device(dev);
	sscanf (buf, "%d\n", &timeout);
	blk_queue_rq_timeout(sdev->request_queue, timeout * HZ);
	return count;
}
static DEVICE_ATTR(timeout, S_IRUGO | S_IWUSR, sdev_show_timeout, sdev_store_timeout);
```

靠，这个就是展示的是 block layer 的 timeout 机制。

```c
static ssize_t queue_io_timeout_show(struct request_queue *q, char *page)
{
	return sprintf(page, "%u\n", jiffies_to_msecs(q->rq_timeout));
}
```

/sys/devices/pci0000:00/0000:00:1a.0/0000:03:00.0/nvme/nvme0/nvme0n1/queue/io_timeout



nvme 是后来设计，所以直接复用 block layer 的 timeout 机制就可以了

```txt
➜  ~ find /sys -name "wwid"
/sys/devices/pseudo_0/adapter0/host2/target2:0:0/2:0:0:0/wwid
/sys/devices/pci0000:00/0000:00:08.0/nvme/nvme0/nvme0n1/wwid
/sys/devices/pci0000:00/0000:00:04.0/0000:01:01.0/nvme/nvme1/nvme1n1/wwid
/sys/devices/pci0000:00/0000:00:0c.0/host0/target0:0:0/0:0:0:0/wwid
/sys/devices/pci0000:00/0000:00:0c.0/host0/target0:0:1/0:0:1:0/wwid
/sys/devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:1/3:0:1:0/wwid
/sys/devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:0/3:0:0:0/wwid
/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0/1:0:0:0/wwid
/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:1/1:0:1:0/wwid
➜  ~ find /sys -name "timeout"
/sys/devices/pseudo_0/adapter0/host2/target2:0:0/2:0:0:0/timeout
/sys/devices/pci0000:00/0000:00:0c.0/host0/target0:0:0/0:0:0:0/timeout
/sys/devices/pci0000:00/0000:00:0c.0/host0/target0:0:1/0:0:1:0/timeout
/sys/devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:1/3:0:1:0/timeout
/sys/devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:0/3:0:0:0/timeout
/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0/1:0:0:0/timeout
/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:1/1:0:1:0/timeout
/sys/module/ipmi_watchdog/parameters/timeout
```

```txt
➜  ~ find /sys -name io_timeout
/sys/devices/pseudo_0/adapter0/host2/target2:0:0/2:0:0:0/block/sde/queue/io_timeout
/sys/devices/pci0000:00/0000:00:08.0/nvme/nvme0/nvme0n1/queue/io_timeout
/sys/devices/pci0000:00/0000:00:04.0/0000:01:01.0/nvme/nvme1/nvme1n1/queue/io_timeout
/sys/devices/pci0000:00/0000:00:0c.0/host0/target0:0:0/0:0:0:0/block/sdc/queue/io_timeout
/sys/devices/pci0000:00/0000:00:0c.0/host0/target0:0:1/0:0:1:0/block/sda/queue/io_timeout
/sys/devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:1/3:0:1:0/block/sdg/queue/io_timeout
/sys/devices/pci0000:00/0000:00:01.1/ata1/host3/target3:0:0/3:0:0:0/block/sdf/queue/io_timeout
/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:2/1:0:2:3/block/sdd/queue/io_timeout
/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0/1:0:0:0/block/sdb/queue/io_timeout
/sys/module/nvme_core/parameters/io_timeout
```

## scsi 提供了每一个盘统计信息
scsi_timeout 中会增加 sysfs 中 iotmo_cnt ，其描述了总共出现 timeout 的数量:
```c
show_sdev_iostat(iorequest_cnt);
show_sdev_iostat(iodone_cnt);
show_sdev_iostat(ioerr_cnt);
show_sdev_iostat(iotmo_cnt);
```


## 可以分析下 scsi_host 下内容

这是设备的

/sys/devices/pci0000:00/0000:00:01.6/0000:04:00.0/host0/scsi_host/host0/eh_deadline


## /proc/scsi

### /proc/scsi/scsi

```txt
🧀  cat
Attached devices:
Host: scsi6 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: ZHITAI SC001 Act Rev: 6200
  Type:   Direct-Access                    ANSI  SCSI revision: 05
Host: scsi7 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: WDC WD20EZBX-00A Rev: 1A01
  Type:   Direct-Access                    ANSI  SCSI revision: 05
```

- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/deployment_guide/s2-proc-dir-scsi

### [ ] /proc/scsi/sg

### [ ] /proc/scsi/device_info

莫明其妙的鸡儿玩意儿。

```txt
🧀  cat /proc/scsi/device_info
'Aashima' 'IMAGERY 2400SP' 0x1
'CHINON' 'CD-ROM CDS-431' 0x1
'CHINON' 'CD-ROM CDS-535' 0x1
'DENON' 'DRD-25X' 0x1
'HITACHI' 'DK312C' 0x1
'HITACHI' 'DK314C' 0x1
'IBM' '2104-DU3' 0x1
'IBM' '2104-TU3' 0x1
'IMS' 'CDD521/10' 0x1
'MAXTOR' 'XT-3280' 0x1
'MAXTOR' 'XT-4380S' 0x1
'MAXTOR' 'MXT-1240S' 0x1
'MAXTOR' 'XT-4170S' 0x1
'MAXTOR' 'XT-8760S' 0x1
'MEDIAVIS' 'RENO CD-ROMX2A' 0x1
'MICROTEK' 'ScanMakerIII' 0x1
'NEC' 'CD-ROM DRIVE:841' 0x1
'PHILIPS' 'PCA80SC' 0x1
'RODIME' 'RO3000S' 0x1
'SUN' 'SENA' 0x1
'SANYO' 'CRD-250S' 0x1
'SEAGATE' 'ST157N' 0x1
'SEAGATE' 'ST296' 0x1
'SEAGATE' 'ST1581' 0x1
'SONY' 'CD-ROM CDU-541' 0x1
'SONY' 'CD-ROM CDU-55S' 0x1
'SONY' 'CD-ROM CDU-561' 0x1
'SONY' 'CD-ROM CDU-8012' 0x1
'SONY' 'SDT-5000' 0x200000
'TANDBERG' 'TDC 3600' 0x1
'TEAC' 'CD-R55S' 0x1
'TEAC' 'CD-ROM' 0x1
'TEAC' 'MT-2ST/45S2-27' 0x1
'HP' 'C1750A' 0x1
'HP' 'C1790A' 0x1
'HP' 'C2500A' 0x1
'MEDIAVIS' 'CDR-H93MV' 0x1
'MICROTEK' 'ScanMaker II' 0x1
'MITSUMI' 'CD-R CR-2201CS' 0x1
'NEC' 'D3856' 0x1
'QUANTUM' 'LPS525S' 0x1
'QUANTUM' 'PD1225S' 0x1
'QUANTUM' 'FIREBALL ST4.3S' 0x1
'RELISYS' 'Scorpio' 0x1
'SANKYO' 'CP525' 0x1
'TEXEL' 'CD-ROM' 0x5
'transtec' 'T5008' 0x40000
'YAMAHA' 'CDR100' 0x1
'YAMAHA' 'CDR102' 0x1
'YAMAHA' 'CRW8424S' 0x1
'YAMAHA' 'CRW6416S' 0x1
'' 'Scanner' 0x1
'3PARdata' 'VV' 0x20000
'ADAPTEC' 'AACRAID' 0x2
'ADAPTEC' 'Adaptec 5400S' 0x2
'AIX' 'VDASD' 0x10002000
'AFT PRO' '-IX CF' 0x2
'BELKIN' 'USB 2 HS-CF' 0x402
'BROWNIE' '1200U3P' 0x40000
'BROWNIE' '1600U3P' 0x40000
'CANON' 'IPUBJD' 0x40
'CBOX3' 'USB Storage-SMC' 0x402
'CMD' 'CRA-7280' 0x40
'CNSI' 'G7324' 0x40
'CNSi' 'G8324' 0x40
'COMPAQ' 'ARRAY CONTROLLER' 0x820240
'COMPAQ' 'LOGICAL VOLUME' 0x800002
'COMPAQ' 'CR3500' 0x2
'COMPAQ' 'MSA1000' 0x1040
'COMPAQ' 'MSA1000 VOLUME' 0x1040
'COMPAQ' 'HSV110' 0x21000
'DDN' 'SAN DataDirector' 0x40
'DEC' 'HSG80' 0x21000
'DELL' 'PV660F' 0x40
'DELL' 'PV660F   PSEUDO' 0x40
'DELL' 'PSEUDO DEVICE .' 0x40
'DELL' 'PV530F' 0x40
'DELL' 'PERCRAID' 0x2
'DGC' 'RAID' 0x40
'DGC' 'DISK' 0x40
'EMC' 'Invista' 0x240
'EMC' 'SYMMETRIX' 0x100020240
'EMULEX' 'MD21/S2     ESDI' 0x10
'easyRAID' '16P' 0x40000
'easyRAID' 'X6P' 0x40000
'easyRAID' 'F8' 0x40000
'FSC' 'CentricStor' 0x240
'FUJITSU' 'ETERNUS_DXM' 0x200000000
'Generic' 'USB SD Reader' 0x402
'Generic' 'USB Storage-SMC' 0x402
'Generic' 'Ultra HS-SD/MMC' 0xc00
'HITACHI' 'DF400' 0x20000
'HITACHI' 'DF500' 0x20000
'HITACHI' 'DISK-SUBSYSTEM' 0x20000
'HITACHI' 'HUS1530' 0x2000000
'HITACHI' 'OPEN-' 0x10020000
'HP' 'A6189A' 0x240
'HP' 'OPEN-' 0x10020000
'HP' 'NetRAID-4M' 0x2
'HP' 'HSV100' 0x21000
'HP' 'C1557A' 0x2
'HP' 'C3323-300' 0x20
'HP' 'C5713A' 0x40000
'HP' 'DISK-SUBSYSTEM' 0x20000
'HPE' 'OPEN-' 0x10020000
'IBM' 'AuSaV1S2' 0x2
'IBM' 'ProFibre 4000R' 0x240
'IBM' '2076' 0x2000
'IBM' '2105' 0x400000
'iomega' 'jaz 1GB' 0x21
'IOMEGA' 'ZIP' 0x21
'IOMEGA' 'Io20S         *F' 0x8
'INSITE' 'Floptical   F*8I' 0x8
'INSITE' 'I325VM' 0x8
'Intel' 'Multi-Flex' 0x20000000
'iRiver' 'iFP Mass Driver' 0x80400
'LASOUND' 'CDX7405' 0x90
'Marvell' 'Console' 0x4000000
'Marvell' '91xx Config' 0x4000000
'MATSHITA' 'PD-1' 0x12
'MATSHITA' 'DMC-LC5' 0x80400
'MATSHITA' 'DMC-LC40' 0x80400
'Medion' 'Flash XL  MMC/SD' 0x2
'MegaRAID' 'LD' 0x2
'MICROP' '4110' 0x20
'MSFT' 'Virtual HD' 0x60000000
'MYLEX' 'DACARMRB' 0x20000
'nCipher' 'Fastness Crypto' 0x2
'NAKAMICH' 'MJ-4.8S' 0x12
'NAKAMICH' 'MJ-5.16S' 0x12
'NEC' 'PD-1 ODX654P' 0x12
'NEC' 'iStorage' 0x20000
'NRC' 'MBR-7' 0x12
'NRC' 'MBR-7.4' 0x12
'PIONEER' 'CD-ROM DRM-600' 0x12
'PIONEER' 'CD-ROM DRM-602X' 0x12
'PIONEER' 'CD-ROM DRM-604X' 0x12
'PIONEER' 'CD-ROM DRM-624X' 0x12
'Promise' 'VTrak E610f' 0x20000040
'Promise' '' 0x40
'QEMU' 'QEMU CD-ROM' 0x4000000
'QNAP' 'iSCSI Storage' 0x40000000
'SYNOLOGY' 'iSCSI Storage' 0x40000000
'QUANTUM' 'XP34301' 0x20
'REGAL' 'CDC-4X' 0x90
'SanDisk' 'ImageMate CF-SD1' 0x2
'SEAGATE' 'ST34555N' 0x20
'SEAGATE' 'ST3390N' 0x20
'SEAGATE' 'ST900MM0006' 0x4000000
'SGI' 'RAID3' 0x40
'SGI' 'RAID5' 0x40
'SGI' 'TP9100' 0x20000
'SGI' 'Universal Xport' 0x100000
'SKhynix' 'H28U74301AMR' 0x4000000
'IBM' 'Universal Xport' 0x100000
'SUN' 'Universal Xport' 0x100000
'DELL' 'Universal Xport' 0x100000
'STK' 'Universal Xport' 0x100000
'NETAPP' 'Universal Xport' 0x100000
'LSI' 'Universal Xport' 0x100000
'ENGENIO' 'Universal Xport' 0x100000
'LENOVO' 'Universal Xport' 0x100000
'FUJITSU' 'Universal Xport' 0x100000
'SanDisk' 'Cruzer Blade' 0x10000400
'SMSC' 'USB 2 HS-CF' 0x440
'SONY' 'CD-ROM CDU-8001' 0x4
'SONY' 'TSL' 0x2
'ST650211' 'CF' 0x400000
'SUN' 'T300' 0x40
'SUN' 'T4' 0x40
'Tornado-' 'F4' 0x40000
'TOSHIBA' 'CDROM' 0x100
'TOSHIBA' 'CD-ROM' 0x100
'Traxdata' 'CDR4120' 0x1
'USB2.0' 'SMARTMEDIA/XD' 0x402
'WangDAT' 'Model 2600' 0x200000
'WangDAT' 'Model 3200' 0x200000
'WangDAT' 'Model 1300' 0x200000
'WDC WD25' '00JB-00FUA0' 0x40000
'XYRATEX' 'RS' 0x240
'Zzyzx' 'RocketStor 500S' 0x40
'Zzyzx' 'RocketStor 2000' 0x40
```

## /sys/devices/pci0000:00/0000:00:0a.0/virtio5/host0/scsi_host/host0/

drivers/scsi/scsi_sysfs.c 中 scsi_sysfs_shost_attrs

## 源码
scsi_sysfs.c

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
