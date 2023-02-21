# udevadm

## 发现
https://unix.stackexchange.com/questions/97676/how-to-find-the-driver-module-associated-with-a-device-on-linux
通过 /sys 查找一个 device 对应的 driver

https://unix.stackexchange.com/questions/248494/how-to-find-the-driver-module-associated-with-sata-device-on-linux
使用 udevadm 来找设备的 driver
```plain
root@n8-030-171:~# udevadm info -a -n /dev/nvme0n1 | egrep 'looking|DRIVER'
  looking at device '/devices/pci0000:80/0000:80:02.1/0000:82:00.0/nvme/nvme1/nvme0n1':
    DRIVER==""
  looking at parent device '/devices/pci0000:80/0000:80:02.1/0000:82:00.0/nvme/nvme1':
    DRIVERS==""
  looking at parent device '/devices/pci0000:80/0000:80:02.1/0000:82:00.0':
    DRIVERS=="nvme"
  looking at parent device '/devices/pci0000:80/0000:80:02.1':
    DRIVERS=="pcieport"
  looking at parent device '/devices/pci0000:80':
    DRIVERS==""
```
发现 ssd 的 driver 就是 nvme

## udevadm monitor --property 可以监控这个这个网络设备的产生

- /devices/virtual/net/veth8384c0f

- [  ] 这个设备是做啥的
