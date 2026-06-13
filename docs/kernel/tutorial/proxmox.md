# proxmox 基本使用

## 如何上传 iso
https://www.thomas-krenn.com/en/wiki/Proxmox_upload_ISO_image

## 启动的 qemu 的效果
```txt
/usr/bin/kvm
-id 100
-name s,debug-threads=on
-no-shutdown
-chardev socket,id=qmp,path=/var/run/qemu-server/100.qmp,server=on,wait=off
-mon chardev=qmp,mode=control
-chardev socket,id=qmp-event,path=/var/run/qmeventd.sock,reconnect=5
-mon chardev=qmp-event,mode=control
-pidfile /var/run/qemu-server/100.pid
-daemonize
-smbios type=1,uuid=b77a12f3-0292-4197-a692-246fd9ba4caf
-smp 16,sockets=1,cores=16,maxcpus=16
-nodefaults
-boot menu=on,strict=on,reboot-timeout=1000,splash=/usr/share/qemu-server/bootsplash.jpg
-vnc unix:/var/run/qemu-server/100.vnc,password=on
-cpu qemu64,+aes,enforce,+kvm_pv_eoi,+kvm_pv_unhalt,+pni,+popcnt,+sse4.1,+sse4.2,+ssse3
-m 2048
-object iothread,id=iothread-virtioscsi0
-device pci-bridge,id=pci.1,chassis_nr=1,bus=pci.0,addr=0x1e
-device pci-bridge,id=pci.2,chassis_nr=2,bus=pci.0,addr=0x1f
-device pci-bridge,id=pci.3,chassis_nr=3,bus=pci.0,addr=0x5
-device vmgenid,guid=949e7f0c-ff7f-45e8-b176-4fd4311cda06
-device piix3-usb-uhci,id=uhci,bus=pci.0,addr=0x1.0x2
-device usb-tablet,id=tablet,bus=uhci.0,port=1
-device VGA,id=vga,bus=pci.0,addr=0x2
-device virtio-balloon-pci,id=balloon0,bus=pci.0,addr=0x3,free-page-reporting=on
-iscsi initiator-name=iqn.1993-08.org.debian:01:db2cde2a35a7
-drive file=/var/lib/vz/template/iso/Fedora-Server-dvd-x86_64-39-1.5.iso,if=none,id=drive-ide2,media=cdrom,aio=io_uring
-device ide-cd,bus=ide.1,unit=0,drive=drive-ide2,id=ide2,bootindex=101
-device virtio-scsi-pci,id=virtioscsi0,bus=pci.3,addr=0x1,iothread=iothread-virtioscsi0
-drive file=/dev/pve/vm-100-disk-0,if=none,id=drive-scsi0,format=raw,cache=none,aio=io_uring,detect-zeroes=on
-device scsi-hd,bus=virtioscsi0.0,channel=0,scsi-id=0,lun=0,drive=drive-scsi0,id=scsi0,bootindex=100
-netdev type=tap,id=net0,ifname=tap100i0,script=/var/lib/qemu-server/pve-bridge,downscript=/var/lib/qemu-server/pve-bridgedown,vhost=on
-device virtio-net-pci,mac=BC:24:11:5F:A8:91,netdev=net0,bus=pci.0,addr=0x12,id=net0,rx_queue_size=1024,tx_queue_size=256,bootindex=102
-machine type=pc+pve0


/usr/bin/kvm
-id 100
-name s,debug-threads=on
-no-shutdown
-chardev socket,id=qmp,path=/var/run/qemu-server/100.qmp,server=on,wait=off
-mon chardev=qmp,mode=control
-chardev socket,id=qmp-event,path=/var/run/qmeventd.sock,reconnect=5
-mon chardev=qmp-event,mode=control
-pidfile /var/run/qemu-server/100.pid
-daemonize
-smbios type=1,uuid=b77a12f3-0292-4197-a692-246fd9ba4caf
-smp 16,sockets=1,cores=16,maxcpus=16
-nodefaults
-boot menu=on,strict=on,reboot-timeout=1000,splash=/usr/share/qemu-server/bootsplash.jpg
-vnc unix:/var/run/qemu-server/100.vnc,password=on
-cpu qemu64,+aes,enforce,+kvm_pv_eoi,+kvm_pv_unhalt,+pni,+popcnt,+sse4.1,+sse4.2,+ssse3
-m 2048
-object iothread,id=iothread-virtioscsi0
-device pci-bridge,id=pci.1,chassis_nr=1,bus=pci.0,addr=0x1e
-device pci-bridge,id=pci.2,chassis_nr=2,bus=pci.0,addr=0x1f
-device pci-bridge,id=pci.3,chassis_nr=3,bus=pci.0,addr=0x5
-device vmgenid,guid=949e7f0c-ff7f-45e8-b176-4fd4311cda06
-device piix3-usb-uhci,id=uhci,bus=pci.0,addr=0x1.0x2
-device usb-tablet,id=tablet,bus=uhci.0,port=1
-device VGA,id=vga,bus=pci.0,addr=0x2
-device virtio-balloon-pci,id=balloon0,bus=pci.0,addr=0x3,free-page-reporting=on
-iscsi initiator-name=iqn.1993-08.org.debian:01:db2cde2a35a7
-drive file=/var/lib/vz/template/iso/Fedora-Server-dvd-x86_64-39-1.5.iso,if=none,id=drive-ide2,media=cdrom,aio=io_uring
-device ide-cd,bus=ide.1,unit=0,drive=drive-ide2,id=ide2,bootindex=101
-device virtio-scsi-pci,id=virtioscsi0,bus=pci.3,addr=0x1,iothread=iothread-virtioscsi0
-drive file=/dev/pve/vm-100-disk-0,if=none,id=drive-scsi0,format=raw,cache=none,aio=io_uring,detect-zeroes=on
-device scsi-hd,bus=virtioscsi0.0,channel=0,scsi-id=0,lun=0,drive=drive-scsi0,id=scsi0,bootindex=100
-netdev type=tap,id=net0,ifname=tap100i0,script=/var/lib/qemu-server/pve-bridge,downscript=/var/lib/qemu-server/pve-bridgedown,vhost=on
-device virtio-net-pci,mac=BC:24:11:5F:A8:91,netdev=net0,bus=pci.0,addr=0x12,id=net0,rx_queue_size=1024,tx_queue_size=256,bootindex=102
-machine type=pc+pve0
```

## host 的 内核版本
```txt
root@martins3:~# uname
-r
6.8.4-2-pve
```

## qemu 的配置的源代码
- https://github.com/proxmox/qemu-server

## Running docker
https://danthesalmon.com/running-docker-on-proxmox/

## 这个真的有趣

https://pve.proxmox.com/wiki/Main_Page

## unraid : 也许也尝试下这个

## proxmox 的各种辅助脚本
https://news.ycombinator.com/item?id=42118286

## 基于 proxmox 的项目
https://www.xda-developers.com/projects-i-host-on-my-proxmox-home-lab/

https://github.com/meienberger/runtipi

## nixos 的 proxmox

https://github.com/SaumonNet/proxmox-nixos

这就真的很有趣了。

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
