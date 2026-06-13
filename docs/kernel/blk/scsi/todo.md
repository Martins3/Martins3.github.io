## DECLARE_TRANSPORT_CLASS

记录一个问题:
如何理解:
DECLARE_TRANSPORT_CLASS

观察一下 DECLARE_TRANSPORT_CLASS 的定义的位置，我们有这几个问题:

1. 让人疑惑的 drivers/scsi/raid_class.c
2. 原来 drivers/ata/libata-transport.c 是和 scsi 并列的东西啊
3. 为什么一个类型总是会定义多个 DECLARE_TRANSPORT_CLASS
例如 drivers/scsi/scsi_transport_fc.c ?

```txt
include/linux/transport_class.h
27:#define DECLARE_TRANSPORT_CLASS(cls, nm, su, rm, cfg)                        \

drivers/base/transport_class.c
46: * DECLARE_TRANSPORT_CLASS() to do this (declared classes still must

drivers/ata/libata-transport.c
217:static DECLARE_TRANSPORT_CLASS(ata_port_class,
499:static DECLARE_TRANSPORT_CLASS(ata_dev_class,
629:static DECLARE_TRANSPORT_CLASS(ata_link_class,

drivers/scsi/scsi_transport_srp.c
114:static DECLARE_TRANSPORT_CLASS(srp_host_class, "srp_host", srp_host_setup,
117:static DECLARE_TRANSPORT_CLASS(srp_rport_class, "srp_remote_ports",

drivers/scsi/scsi_transport_spi.c
184:static DECLARE_TRANSPORT_CLASS(spi_host_class,
1426:static DECLARE_TRANSPORT_CLASS(spi_transport_class,

drivers/scsi/scsi_transport_sas.c
260:static DECLARE_TRANSPORT_CLASS(sas_host_class,
664:static DECLARE_TRANSPORT_CLASS(sas_phy_class,
835:static DECLARE_TRANSPORT_CLASS(sas_port_class,
1299:static DECLARE_TRANSPORT_CLASS(sas_end_dev_class,
1328:static DECLARE_TRANSPORT_CLASS(sas_expander_class,
1357:static DECLARE_TRANSPORT_CLASS(sas_rphy_class,

drivers/scsi/scsi_transport_iscsi.c
1584:static DECLARE_TRANSPORT_CLASS(iscsi_host_class,
1590:static DECLARE_TRANSPORT_CLASS(iscsi_session_class,
1596:static DECLARE_TRANSPORT_CLASS(iscsi_connection_class,

drivers/scsi/scsi_transport_fc.c
380:static DECLARE_TRANSPORT_CLASS(fc_transport_class,
466:static DECLARE_TRANSPORT_CLASS(fc_host_class,
476:static DECLARE_TRANSPORT_CLASS(fc_rport_class,
486:static DECLARE_TRANSPORT_CLASS(fc_vport_class,

drivers/scsi/raid_class.c
111:static DECLARE_TRANSPORT_CLASS(raid_class,
```

这个 class 就是 sysfs 下的 class 了，其实并不难理解
用 ata 为例子，也就是:

```txt
ata_device
├── dev1.0 -> ../../devices/pci0000:00/0000:00:17.0/ata1/link1/dev1.0/ata_device/dev1.0
├── dev2.0 -> ../../devices/pci0000:00/0000:00:17.0/ata2/link2/dev2.0/ata_device/dev2.0
├── dev3.0 -> ../../devices/pci0000:00/0000:00:17.0/ata3/link3/dev3.0/ata_device/dev3.0
├── dev4.0 -> ../../devices/pci0000:00/0000:00:17.0/ata4/link4/dev4.0/ata_device/dev4.0
├── dev5.0 -> ../../devices/pci0000:00/0000:00:17.0/ata5/link5/dev5.0/ata_device/dev5.0
├── dev6.0 -> ../../devices/pci0000:00/0000:00:17.0/ata6/link6/dev6.0/ata_device/dev6.0
├── dev7.0 -> ../../devices/pci0000:00/0000:00:17.0/ata7/link7/dev7.0/ata_device/dev7.0
└── dev8.0 -> ../../devices/pci0000:00/0000:00:17.0/ata8/link8/dev8.0/ata_device/dev8.0
ata_link
├── link1 -> ../../devices/pci0000:00/0000:00:17.0/ata1/link1/ata_link/link1
├── link2 -> ../../devices/pci0000:00/0000:00:17.0/ata2/link2/ata_link/link2
├── link3 -> ../../devices/pci0000:00/0000:00:17.0/ata3/link3/ata_link/link3
├── link4 -> ../../devices/pci0000:00/0000:00:17.0/ata4/link4/ata_link/link4
├── link5 -> ../../devices/pci0000:00/0000:00:17.0/ata5/link5/ata_link/link5
├── link6 -> ../../devices/pci0000:00/0000:00:17.0/ata6/link6/ata_link/link6
├── link7 -> ../../devices/pci0000:00/0000:00:17.0/ata7/link7/ata_link/link7
└── link8 -> ../../devices/pci0000:00/0000:00:17.0/ata8/link8/ata_link/link8
ata_port
├── ata1 -> ../../devices/pci0000:00/0000:00:17.0/ata1/ata_port/ata1
├── ata2 -> ../../devices/pci0000:00/0000:00:17.0/ata2/ata_port/ata2
├── ata3 -> ../../devices/pci0000:00/0000:00:17.0/ata3/ata_port/ata3
├── ata4 -> ../../devices/pci0000:00/0000:00:17.0/ata4/ata_port/ata4
├── ata5 -> ../../devices/pci0000:00/0000:00:17.0/ata5/ata_port/ata5
├── ata6 -> ../../devices/pci0000:00/0000:00:17.0/ata6/ata_port/ata6
├── ata7 -> ../../devices/pci0000:00/0000:00:17.0/ata7/ata_port/ata7
└── ata8 -> ../../devices/pci0000:00/0000:00:17.0/ata8/ata_port/ata8
```


drivers/scsi/scsi_transport_fc.c:380-490 定义了四个传输类：

```c
// 1. fc_transport_class - 对应 SCSI Target
static DECLARE_TRANSPORT_CLASS(fc_transport_class,
                               "fc_transport",
                               fc_target_setup,
                               NULL,
                               NULL);

// 2. fc_host_class - 对应 SCSI Host (HBA)
static DECLARE_TRANSPORT_CLASS(fc_host_class,
                               "fc_host",
                               fc_host_setup,
                               fc_host_remove,
                               NULL);

// 3. fc_rport_class - 对应 Remote Port
static DECLARE_TRANSPORT_CLASS(fc_rport_class,
                               "fc_remote_ports",
                               NULL,
                               NULL,
                               NULL);

// 4. fc_vport_class - 对应 Virtual Port
static DECLARE_TRANSPORT_CLASS(fc_vport_class,
                               "fc_vports",
                               NULL,
                               NULL,
                               NULL);
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
