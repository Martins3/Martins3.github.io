## alpine iso 可以直接启动使用
https://alpinelinux.org/downloads/

wget https://dl-cdn.alpinelinux.org/alpine/v3.18/releases/x86_64/alpine-virt-3.18.3-x86_64.iso
qemu-system-x86_64 --enable-kvm -cpu host --nographic -serial mon:stdio -boot d -cdrom alpine-virt-3.18.3-x86_64.iso
输入 root 登录

# QEMU 的基本使用方法

总体来说，QEMU 直接使用还是非常复杂的，如果只是想要使用虚拟化功能，可以使用如下两个软件，其将 QEMU 进行了封装:

- https://github.com/quickemu-project/quickemu
- https://mac.getutm.app/

但是，如果想要学习 Linux 内核或者 QEMU 本身，熟悉 QEMU 的命令行使用还是必须的。

这里大致介绍一下如何使用 QEMU 。

## 如何编译 QEMU

QEMU 支持的选项很多，通过下面的命令来查看：

```sh
../configure --help
```

想要运行 32 位的 x86 操作系统，编译参数为:

```sh
mkdir 32bit
cd 32bit || exit 1
../configure --target-list=i386-softmmu
make -j
```

编译运行在 ARM 上的 QEMU:

```sh
../configure --target-list=aarch64-softmmu --disable-werror
```

- https://ibugone.com/blog/2019/04/os-lab-1/
- https://www.kernel.org/doc/html/latest/filesystems/ramfs-rootfs-initramfs.html

其中，init 不仅仅可以是 elf 格式的程序，还是可以是 shell 的，例如

需要注意的是，其运行参数为

```bash
arg_kernel_args="console=ttyS0 root=/dev/ram rdinit=/hello.out"
```

这是因为我们的程序名称为 hello.out ，如果将 hello.out 修改为 init，那么就无需使用额外的参数了。

## 其他
- https://ubuntu.com/kernel/lifecycle

## vnc

有的系统是必须使用图形界面才可以安装的，例如 Ubuntu[^1]，此时只能使用 vnc 在远程打开:

参考 man qemu(1)

```txt
              host:d TCP connections will only be allowed from host on display d. By convention the TCP port is 5900+d. Optionally, host can be  omitted
                     in which case the server will accept connections from any host.

```

给启动参数添加上

```sh
-vnc :0,password=on
```

在 mac 上可以使用连接:

```sh
open vnc://192.168.23.126:5900
```

注意：这里又一个小坑，那就是密码是必须的，然后在 QEMU 的 monitor 中设置密码:

```txt
(qemu) set_password vnc 1
```

## TODO

- [ ] 利用 QEMU 给一个分区安装操作系统

## root=/dev/sda3

內核启动参数中需要指定 root=/dev/sda3 但是这种方法存在问题，因为切换 /dev/sda3 这种名称不是固定的
切换内核之后，该名称就可能发生改变:

```txt
[  177.906625] dracut-initqueue[515]: Warning: /lib/dracut/hooks/initqueue/finished/devexists-\x2fdev\x2fvdb2.sh: "if ! grep -q After=remote-fs-pre.target /run/systemd/generator/systemd-cryptsetup@*.service 2>/dev/null; then
[  177.907758] dracut-initqueue[515]:     [ -e "/dev/vdb2" ]
[  177.908078] dracut-initqueue[515]: fi"
[  177.908312] dracut-initqueue[515]: Warning: dracut-initqueue: starting timeout scripts
[  178.423651] dracut-initqueue[515]: Warning: dracut-initqueue: timeout, still waiting for following initqueue hooks:
[  178.424660] dracut-initqueue[515]: Warning: /lib/dracut/hooks/initqueue/finished/devexists-\x2fdev\x2fvdb2.sh: "if ! grep -q After=remote-fs-pre.ta
```

正常启动之后，获取名称:

```txt
blkid -s PARTUUID -o value /dev/vda2
```

然后:

```txt
root="PARTUUID=2c4eb5c7-02"
```

但是 crash 报错为:

```txt
[  133.418477] dracut-initqueue[171]: Warning: /lib/dracut/hooks/initqueue/finished/devexists-\x2fdev\x2fdisk\x2fby-uuid\x2fae225bc6-e617-4956-82f8-b9b3db3f113a.sh: "[ -e "/dev/disk/by-uuid/ae225bc6-e617-4956-82f8-b9b3db3f113a" ]"
```

## 构建各个 distribution 的内核 rpm

- centos : https://git.centos.org/rpms/kernel/releases

- [ ] ubuntu 之类的整理过来

[^1]: https://askubuntu.com/questions/1108334/how-to-boot-and-install-the-ubuntu-server-image-on-qemu-nographic-without-the-g
[^2]: https://unix.stackexchange.com/questions/124681/how-to-ssh-from-host-to-guest-using-qemu

## 调试内核模块模块
- https://stackoverflow.com/questions/28607538/how-to-debug-linux-kernel-modules-with-qemu
- https://github.com/0xricksanchez/like-dbg


## 相关项目
- https://github.com/kimchi-project/kimchi

## review 一下这里的内容
https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html


## qemu

参考这个:

1. https://powersj.io/posts/ubuntu-qemu-cli/

参考: https://stackoverflow.com/questions/29137679/login-credentials-of-ubuntu-cloud-server-image

不知道为什么, 这种方法是没有办法正常找到 boot device 的

```sh
qemu-system-x86_64  \
  -machine accel=kvm,type=q35 \
  -cpu host \
  -m 2G \
  -nographic \
  -drive if=virtio,format=qcow2,file=mantic-server-cloudimg-amd64.img \
  -drive file=user-data.img,format=raw \
```

看了评论区才知道有这个方法:

```sh
qemu-system-x86_64  \
  -machine accel=kvm,type=q35 \
  -cpu host \
  -m 2G \
  -nographic \
 -drive file=mantic-server-cloudimg-amd64.img,index=0,media=disk  \
 -drive file=user-data.img,index=1,media=disk  \
```

但是似乎也是不行的，

-machine accel=kvm \

/usr/libexec/qemu-kvm \
-cpu host \
-m 12G \
-nographic \
 -drive if=virtio,format=qcow2,file=CentOS-8-GenericCloud-8.4.2105-20210603.0.x86_64.qcow2,index=0 \
 -drive if=virtio,format=raw,file=user-data.img,index=1 \

- alpine.sh 总也是饱受 boot index 困扰啊
- media=disk 和 if=virtio 有啥区别?

## 这个链接很好，关于 qemu 的模拟

https://blogs.oracle.com/linux/post/how-to-emulate-block-devices-with-qemu

## openeuler 上也是可以图形的，非常不错

https://docs.openeuler.org/zh/docs/22.03_LTS_SP2/docs/desktop/Install_GNOME.html

## TODO
- [ ] 编译系统 : https://qemu.readthedocs.io/en/latest/devel/build-system.html

- `-vga virtio` 为什么不需要在 kernel 那一侧的支持 ?
  - 不太可能吧

## compile
--target-list 最好不要同时支持多个，否则会出现一些诡异的问题。

## 使用 Qemu 的参数
1. 调试内核:
  - https://blahcat.github.io/2018/01/07/building-a-debian-stretch-qemu-image-for-aarch64/
  - https://kennedy-han.github.io/2015/06/15/QEMU-arm64-guide.html
  - https://dev.to/alexeyden/quick-qemu-setup-for-linux-kernel-module-debugging-2nde

# 资源
- https://news.ycombinator.com/item?id=26941744
  - https://zserge.com/posts/kvm/ : 小文章
  - [qemu blog](https://airbus-seclab.github.io/qemu_blog/)
  - 可能有用的书 和 文章:
    - Foundations of Libvirt Development: How to Set Up and Maintain a Virtual Machine Environment with Python
    - https://www.morganclaypool.com/doi/abs/10.2200/S00754ED1V01Y201701CAC038
    - https://dl.acm.org/doi/abs/10.1145/2382553.2382554

- [ ] https://www.qemu-advent-calendar.org/2020/ : pulished every two year, adventure for dune

## debug kernel with qemu
- http://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/
- https://www.kernel.org/doc/html/latest/dev-tools/gdb-kernel-debugging.html

- https://stefano-garzarella.github.io/posts/2019-08-23-qemu-linux-kernel-pvh/
  - 让内核不要被压缩, 根本不是一个东西

## 调试 qemu
https://wiki.qemu.org/Documentation/Debugging

## related project
- [Unicorn](https://github.com/unicorn-engine/unicorn) is a lightweight, multi-platform, multi-architecture CPU emulator framework based on QEMU.

## 参考资料
[主板 INTEL 440FX PCISET](https://wiki.qemu.org/images/b/bb/29054901.pdf)

## 补充 qemu 启动
https://news.ycombinator.com/item?id=37460614
https://futurewei-cloud.github.io/ARM-Datacenter/posts/

## virt-manager

安装的:
```sh
/run/libvirt/nix-emulators/qemu-system-x86_64 \
-name guest=fedora,debug-threads=on \
-S \
-object {"qom-type":"secret","id":"masterKey0","format":"raw","file":"/var/lib/libvirt/qemu/domain-1-fedora/master-key.aes"} \
-machine pc-q35-8.1,usb=off,vmport=off,dump-guest-core=off,memory-backend=pc.ram,hpet=off,acpi=on \
-accel kvm \
-cpu host,migratable=on \
-m size=4145152k \
-object {"qom-type":"memory-backend-ram","id":"pc.ram","size":4244635648} \
-overcommit mem-lock=off \
-smp 4,sockets=4,cores=1,threads=1 \
-uuid 8c0b7e20-2ae9-4db0-8002-655c77886638 \
-no-user-config \
-nodefaults \
-chardev socket,id=charmonitor,fd=33,server=on,wait=off \
-mon chardev=charmonitor,id=monitor,mode=control \
-rtc base=utc,driftfix=slew \
-global kvm-pit.lost_tick_policy=delay \
-no-shutdown \
-global ICH9-LPC.disable_s3=1 \
-global ICH9-LPC.disable_s4=1 \
-boot strict=on \
-device {"driver":"pcie-root-port","port":16,"chassis":1,"id":"pci.1","bus":"pcie.0","multifunction":true,"addr":"0x2"} \
-device {"driver":"pcie-root-port","port":17,"chassis":2,"id":"pci.2","bus":"pcie.0","addr":"0x2.0x1"} \
-device {"driver":"pcie-root-port","port":18,"chassis":3,"id":"pci.3","bus":"pcie.0","addr":"0x2.0x2"} \
-device {"driver":"pcie-root-port","port":19,"chassis":4,"id":"pci.4","bus":"pcie.0","addr":"0x2.0x3"} \
-device {"driver":"pcie-root-port","port":20,"chassis":5,"id":"pci.5","bus":"pcie.0","addr":"0x2.0x4"} \
-device {"driver":"pcie-root-port","port":21,"chassis":6,"id":"pci.6","bus":"pcie.0","addr":"0x2.0x5"} \
-device {"driver":"pcie-root-port","port":22,"chassis":7,"id":"pci.7","bus":"pcie.0","addr":"0x2.0x6"} \
-device {"driver":"pcie-root-port","port":23,"chassis":8,"id":"pci.8","bus":"pcie.0","addr":"0x2.0x7"} \
-device {"driver":"pcie-root-port","port":24,"chassis":9,"id":"pci.9","bus":"pcie.0","multifunction":true,"addr":"0x3"} \
-device {"driver":"pcie-root-port","port":25,"chassis":10,"id":"pci.10","bus":"pcie.0","addr":"0x3.0x1"} \
-device {"driver":"pcie-root-port","port":26,"chassis":11,"id":"pci.11","bus":"pcie.0","addr":"0x3.0x2"} \
-device {"driver":"pcie-root-port","port":27,"chassis":12,"id":"pci.12","bus":"pcie.0","addr":"0x3.0x3"} \
-device {"driver":"pcie-root-port","port":28,"chassis":13,"id":"pci.13","bus":"pcie.0","addr":"0x3.0x4"} \
-device {"driver":"pcie-root-port","port":29,"chassis":14,"id":"pci.14","bus":"pcie.0","addr":"0x3.0x5"} \
-device {"driver":"qemu-xhci","p2":15,"p3":15,"id":"usb","bus":"pci.2","addr":"0x0"} \
-device {"driver":"virtio-serial-pci","id":"virtio-serial0","bus":"pci.3","addr":"0x0"} \
-blockdev {"driver":"file","filename":"/var/lib/libvirt/images/fedora.qcow2","node-name":"libvirt-2-storage","auto-read-only":true,"discard":"unmap"} \
-blockdev {"node-name":"libvirt-2-format","read-only":false,"discard":"unmap","driver":"qcow2","file":"libvirt-2-storage","backing":null} \
-device {"driver":"virtio-blk-pci","bus":"pci.4","addr":"0x0","drive":"libvirt-2-format","id":"virtio-disk0","bootindex":2} \
-blockdev {"driver":"file","filename":"/home/martins3/hack/iso/Fedora-Server-dvd-x86_64-39-1.5.iso","node-name":"libvirt-1-storage","auto-read-only":true,"discard":"unmap"} \
-blockdev {"node-name":"libvirt-1-format","read-only":true,"driver":"raw","file":"libvirt-1-storage"} \
-device {"driver":"ide-cd","bus":"ide.0","drive":"libvirt-1-format","id":"sata0-0-0","bootindex":1} \
-netdev {"type":"tap","fd":"34","vhost":true,"vhostfd":"36","id":"hostnet0"} \
-device {"driver":"virtio-net-pci","netdev":"hostnet0","id":"net0","mac":"52:54:00:b5:91:e4","bus":"pci.1","addr":"0x0"} \
-chardev pty,id=charserial0 \
-device {"driver":"isa-serial","chardev":"charserial0","id":"serial0","index":0} \
-chardev socket,id=charchannel0,fd=32,server=on,wait=off \
-device {"driver":"virtserialport","bus":"virtio-serial0.0","nr":1,"chardev":"charchannel0","id":"channel0","name":"org.qemu.guest_agent.0"} \
-chardev spicevmc,id=charchannel1,name=vdagent \
-device {"driver":"virtserialport","bus":"virtio-serial0.0","nr":2,"chardev":"charchannel1","id":"channel1","name":"com.redhat.spice.0"} \
-device {"driver":"usb-tablet","id":"input0","bus":"usb.0","port":"1"} \
-audiodev {"id":"audio1","driver":"spice"} \
-spice port=5900,addr=127.0.0.1,disable-ticketing=on,image-compression=off,seamless-migration=on \
-device {"driver":"virtio-vga","id":"video0","max_outputs":1,"bus":"pcie.0","addr":"0x1"} \
-device {"driver":"ich9-intel-hda","id":"sound0","bus":"pcie.0","addr":"0x1b"} \
-device {"driver":"hda-duplex","id":"sound0-codec0","bus":"sound0.0","cad":0,"audiodev":"audio1"} \
-global ICH9-LPC.noreboot=off \
-watchdog-action reset \
-chardev spicevmc,id=charredir0,name=usbredir \
-device {"driver":"usb-redir","chardev":"charredir0","id":"redir0","bus":"usb.0","port":"2"} \
-chardev spicevmc,id=charredir1,name=usbredir \
-device {"driver":"usb-redir","chardev":"charredir1","id":"redir1","bus":"usb.0","port":"3"} \
-device {"driver":"virtio-balloon-pci","id":"balloon0","bus":"pci.5","addr":"0x0"} \
-object {"qom-type":"rng-random","id":"objrng0","filename":"/dev/urandom"} \
-device {"driver":"virtio-rng-pci","rng":"objrng0","id":"rng0","bus":"pci.6","addr":"0x0"} \
-sandbox on,obsolete=deny,elevateprivileges=deny,spawn=deny,resourcecontrol=deny \
-msg timestamp=on
```
使用 libvirt 封装的, 才知道是可以写成

```sh
/run/libvirt/nix-emulators/qemu-system-x86_64 \
-name guest=fedora,debug-threads=on \
-S \
-object {"qom-type":"secret","id":"masterKey0","format":"raw","file":"/var/lib/libvirt/qemu/domain-2-fedora/master-key.aes"} \
-machine pc-q35-8.1,usb=off,vmport=off,dump-guest-core=off,memory-backend=pc.ram,hpet=off,acpi=on \
-accel kvm \
-cpu host,migratable=on \
-m size=4145152k \
-object {"qom-type":"memory-backend-ram","id":"pc.ram","size":4244635648} \
-overcommit mem-lock=off \
-smp 4,sockets=4,cores=1,threads=1 \
-uuid 8c0b7e20-2ae9-4db0-8002-655c77886638 \
-no-user-config \
-nodefaults \
-chardev socket,id=charmonitor,fd=33,server=on,wait=off \
-mon chardev=charmonitor,id=monitor,mode=control \
-rtc base=utc,driftfix=slew \
-global kvm-pit.lost_tick_policy=delay \
-no-shutdown \
-global ICH9-LPC.disable_s3=1 \
-global ICH9-LPC.disable_s4=1 \
-boot strict=on \
-device {"driver":"pcie-root-port","port":16,"chassis":1,"id":"pci.1","bus":"pcie.0","multifunction":true,"addr":"0x2"} \
-device {"driver":"pcie-root-port","port":17,"chassis":2,"id":"pci.2","bus":"pcie.0","addr":"0x2.0x1"} \
-device {"driver":"pcie-root-port","port":18,"chassis":3,"id":"pci.3","bus":"pcie.0","addr":"0x2.0x2"} \
-device {"driver":"pcie-root-port","port":19,"chassis":4,"id":"pci.4","bus":"pcie.0","addr":"0x2.0x3"} \
-device {"driver":"pcie-root-port","port":20,"chassis":5,"id":"pci.5","bus":"pcie.0","addr":"0x2.0x4"} \
-device {"driver":"pcie-root-port","port":21,"chassis":6,"id":"pci.6","bus":"pcie.0","addr":"0x2.0x5"} \
-device {"driver":"pcie-root-port","port":22,"chassis":7,"id":"pci.7","bus":"pcie.0","addr":"0x2.0x6"} \
-device {"driver":"pcie-root-port","port":23,"chassis":8,"id":"pci.8","bus":"pcie.0","addr":"0x2.0x7"} \
-device {"driver":"pcie-root-port","port":24,"chassis":9,"id":"pci.9","bus":"pcie.0","multifunction":true,"addr":"0x3"} \
-device {"driver":"pcie-root-port","port":25,"chassis":10,"id":"pci.10","bus":"pcie.0","addr":"0x3.0x1"} \
-device {"driver":"pcie-root-port","port":26,"chassis":11,"id":"pci.11","bus":"pcie.0","addr":"0x3.0x2"} \
-device {"driver":"pcie-root-port","port":27,"chassis":12,"id":"pci.12","bus":"pcie.0","addr":"0x3.0x3"} \
-device {"driver":"pcie-root-port","port":28,"chassis":13,"id":"pci.13","bus":"pcie.0","addr":"0x3.0x4"} \
-device {"driver":"pcie-root-port","port":29,"chassis":14,"id":"pci.14","bus":"pcie.0","addr":"0x3.0x5"} \
-device {"driver":"qemu-xhci","p2":15,"p3":15,"id":"usb","bus":"pci.2","addr":"0x0"} \
-device {"driver":"virtio-serial-pci","id":"virtio-serial0","bus":"pci.3","addr":"0x0"} \
-blockdev {"driver":"file","filename":"/var/lib/libvirt/images/fedora.qcow2","node-name":"libvirt-2-storage","auto-read-only":true,"discard":"unmap"} \
-blockdev {"node-name":"libvirt-2-format","read-only":false,"discard":"unmap","driver":"qcow2","file":"libvirt-2-storage","backing":null} \
-device {"driver":"virtio-blk-pci","bus":"pci.4","addr":"0x0","drive":"libvirt-2-format","id":"virtio-disk0","bootindex":1} \
-device {"driver":"ide-cd","bus":"ide.0","id":"sata0-0-0"} \
-netdev {"type":"tap","fd":"34","vhost":true,"vhostfd":"36","id":"hostnet0"} \
-device {"driver":"virtio-net-pci","netdev":"hostnet0","id":"net0","mac":"52:54:00:b5:91:e4","bus":"pci.1","addr":"0x0"} \
-chardev pty,id=charserial0 \
-device {"driver":"isa-serial","chardev":"charserial0","id":"serial0","index":0} \
-chardev socket,id=charchannel0,fd=32,server=on,wait=off \
-device {"driver":"virtserialport","bus":"virtio-serial0.0","nr":1,"chardev":"charchannel0","id":"channel0","name":"org.qemu.guest_agent.0"} \
-chardev spicevmc,id=charchannel1,name=vdagent \
-device {"driver":"virtserialport","bus":"virtio-serial0.0","nr":2,"chardev":"charchannel1","id":"channel1","name":"com.redhat.spice.0"} \
-device {"driver":"usb-tablet","id":"input0","bus":"usb.0","port":"1"} \
-audiodev {"id":"audio1","driver":"spice"} \
-spice port=5900,addr=127.0.0.1,disable-ticketing=on,image-compression=off,seamless-migration=on \
-device {"driver":"virtio-vga","id":"video0","max_outputs":1,"bus":"pcie.0","addr":"0x1"} \
-device {"driver":"ich9-intel-hda","id":"sound0","bus":"pcie.0","addr":"0x1b"} \
-device {"driver":"hda-duplex","id":"sound0-codec0","bus":"sound0.0","cad":0,"audiodev":"audio1"} \
-global ICH9-LPC.noreboot=off \
-watchdog-action reset \
-chardev spicevmc,id=charredir0,name=usbredir \
-device {"driver":"usb-redir","chardev":"charredir0","id":"redir0","bus":"usb.0","port":"2"} \
-chardev spicevmc,id=charredir1,name=usbredir \
-device {"driver":"usb-redir","chardev":"charredir1","id":"redir1","bus":"usb.0","port":"3"} \
-device {"driver":"virtio-balloon-pci","id":"balloon0","bus":"pci.5","addr":"0x0"} \
-object {"qom-type":"rng-random","id":"objrng0","filename":"/dev/urandom"} \
-device {"driver":"virtio-rng-pci","rng":"objrng0","id":"rng0","bus":"pci.6","addr":"0x0"} \
-sandbox on,obsolete=deny,elevateprivileges=deny,spawn=deny,resourcecontrol=deny \
-msg timestamp=on \
```

## lima

```sh
/nix/store/dgkq92w07fy90qpsqak0i325i1fcvwz7-qemu-8.1.5/bin/qemu-system-x86_64 \
	-m 4096 \
	-cpu host \
	-machine q35,accel=kvm \
	-smp 4,sockets=1,cores=4,threads=1 \
	-drive if=pflash,format=raw,readonly=on,file=/nix/store/dgkq92w07fy90qpsqak0i325i1fcvwz7-qemu-8.1.5/share/qemu/edk2-x86_64-code.fd \
	-boot order=c,splash-time=0,menu=on \
	-drive file=/home/martins3/.lima/default/diffdisk,if=virtio,discard=on \
	-drive id=cdrom0,if=none,format=raw,readonly=on,file=/home/martins3/.lima/default/cidata.iso \
	-device virtio-scsi-pci,id=scsi0 \
	-device scsi-cd,bus=scsi0.0,drive=cdrom0 \
	-netdev user,id=net0,net=192.168.5.0/24,dhcpstart=192.168.5.15,hostfwd=tcp:127.0.0.1:60022-:22 \
	-device virtio-net-pci,netdev=net0,mac=52:55:55:e2:4d:e5 \
	-device virtio-rng-pci \
	-display none \
	-device virtio-vga \
	-device virtio-keyboard-pci \
	-device virtio-mouse-pci \
	-device qemu-xhci,id=usb-bus \
	-parallel none \
	-chardev socket,id=char-serial,path=/home/martins3/.lima/default/serial.sock,server=on,wait=off,logfile=/home/martins3/.lima/default/serial.log \
	-serial chardev:char-serial \
	-chardev socket,id=char-serial-virtio,path=/home/martins3/.lima/default/serialv.sock,server=on,wait=off,logfile=/home/martins3/.lima/default/serialv.log \
	-device virtio-serial-pci,id=virtio-serial0,max_ports=1 \
	-device virtconsole,chardev=char-serial-virtio,id=console0 \
	-chardev socket,id=char-qmp,path=/home/martins3/.lima/default/qmp.sock,server=on,wait=off \
	-qmp chardev:char-qmp \
	-name lima-default \
	-pidfile /home/martins3/.lima/default/qemu.pid
```
没有看懂他这个将 host fs 只读给 guest 是如何实现的!

## 哈哈，这也算是一种启动 qemu 的方式吧
- https://github.com/google/syzkaller/blob/master/vm/qemu/qemu.go

## 这个是做啥的?
https://github.com/qemus/qemu-docker

使用 k8s 或者容器 封装 qemu ，非常好!

有一堆这种项目，https://github.com/qemus

## https://github.com/kholia/OSX-KVM


## 对于 windows 的一个有趣的封装
git clone https://github.com/dockur/windows

## 看看
https://github.com/hugelgupf/vmtest

## 通过 qemu 中的 info mtree 逐个看看都是里面都是有什么
用于理解 aarch64 是极好的

## 你必须精通 QEMU
我认为任何操作系统工程师必须深入理解硬件，
所以必须精通 qemu 。

搞操作系统不懂 qemu ，如同搞芯片不懂 gem5
搞网络不懂 wireshare

## UTM
https://rkiselenko.dev/blog/development-on-mac-with-utm/development-on-mac-with-lima/

## 这个是极好的
https://www.qemu.org/docs/master/specs/edu.html


## 这个是终极解决方案吗？
https://github.com/winapps-org/winapps
https://github.com/TibixDev/winboat

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
