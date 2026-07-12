2. dpdk + ovs 环境

## 一些失败的想法

首先，引入的变量实在是太多了:

1. 存在哪些共享
  - host 共享 vn 到 /home/core/vn
    - 这个是为了，L1 方便的执行 mod 相关的代码
    - L1 可以容器可以方便的执行 env.sh (伪需求)
2. 让 podman 可以长期运行，开机自动启动，自动 ssh 到容器中
  - 可以有其他的什么无状态的方法吗?
3. https://github.com/psviderski/unregistry


## 其实在 x86 上调试 arm 的东西也不错的

似乎 tcg 性能没有想象的差

## kimi 教我如何安装虚拟机
### 4.2 完整示例：从 ISO 安装虚拟机

以下示例使用 Fedora 43 ISO 进行无人值守安装：

```bash
#!/bin/bash
set -E -e -u -o pipefail

QEMU=~/data/qemu/build/qemu-system-x86_64
ISO=~/data/hack/iso/Fedora-Server-dvd-x86_64-43-1.6.iso
DISK=~/data/hack/vm/fedora-mmoc/fedora-mmoc.raw
KS=~/data/hack/vm/fedora-mmoc/ks.cfg
SOCK=/tmp/connection-uds-fd
MEM_SIZE=8G

# 创建磁盘（如不存在）
[ -f "${DISK}" ] || qemu-img create -f raw "${DISK}" 60G

# 提取内核/initrd（用于 kickstart 安装）
mkdir -p ~/data/hack/vm/fedora-mmoc/boot
if [ ! -f ~/data/hack/vm/fedora-mmoc/boot/vmlinuz ]; then
  sudo mount -o loop "${ISO}" /mnt
  cp /mnt/images/pxeboot/vmlinuz ~/data/hack/vm/fedora-mmoc/boot/
  cp /mnt/images/pxeboot/initrd.img ~/data/hack/vm/fedora-mmoc/boot/
  sudo umount /mnt
fi

# 启动 http.server 提供 kickstart 文件（后台）
python3 -m http.server 8000 --directory ~/data/hack/vm/fedora-mmoc &
HTTP_PID=$!
trap "kill ${HTTP_PID}" EXIT

# 启动 QEMU
${QEMU} \
  -uuid 12345678-1234-1234-1234-123456789abc \
  -machine pc,hpet=off \
  -accel kvm \
  -cpu host \
  -smp 4 \
  -m ${MEM_SIZE},slots=8,maxmem=256G \
  -chardev socket,id=mmoc-chrdev,path=${SOCK},reconnect-ms=50 \
  -object memory-backend-memfd,id=mem0,size=${MEM_SIZE},share=on,mmoc=on,swap-storage=file://local/tmp/mmoc-swap \
  -numa node,nodeid=0,memdev=mem0 \
  -drive file="${DISK}",format=raw,if=virtio \
  -cdrom "${ISO}" \
  -kernel ~/data/hack/vm/fedora-mmoc/boot/vmlinuz \
  -initrd ~/data/hack/vm/fedora-mmoc/boot/initrd.img \
  -append "inst.ks=http://10.0.2.2:8000/ks.cfg console=ttyS0,115200n8" \
  -serial stdio \
  -netdev user,id=net0,hostfwd=tcp::2222-:22 \
  -device virtio-net-pci,netdev=net0
```

kickstart 示例（`ks.cfg`）：

```
lang en_US.UTF-8
keyboard us
timezone UTC --utc
rootpw --plaintext root
user --name=martin --password=martin --plaintext
text
reboot
cdrom
bootloader --location=mbr --append="console=ttyS0,115200n8"
zerombr
clearpart --all --initlabel
autopart --type=plain
network --bootproto=dhcp --device=link --activate
firewall --disabled
selinux --enforcing
services --enabled=sshd,serial-getty@ttyS0.service
skipx
%packages
@^server-product-environment
%end
%post
systemctl enable serial-getty@ttyS0.service
%end
```

## 为什么使用 cgroup 这么设计

```txt
systemd-run --user --scope --collect --unit="$vm_name" bash "$vm_dir/cmd.sh"
```

1. 现在的配置，启动脚本，默认在 cgroup 中

cat /proc/$$/cgroup
```txt
0::/user.slice/user-1000.slice/user@1000.service/app.slice/tmux-spawn-e6e3b687-d889-4e7e-b20e-86c3bd4d0977.scope

0::/user.slice/user-1000.slice/user@1000.service/app.slice/app-ghostty-surface-transient-1460744.scope
```

有点不可控，例如在 tmux nvim 的 terminal 中启动 qemu ，那么就是这样的:

```txt
cat /proc/410478/comm
cat /proc/429212/comm
cat /proc/429300/comm
cat /proc/429301/comm
cat /proc/429310/comm
cat /proc/429311/comm
cat /proc/436781/comm
cat /proc/436909/comm
cat /proc/437133/comm
cat /proc/437134/comm

zsh
.tig-wrapped
nvim
nvim
node
efm-langserver
zsh
bash
bash
qemu-system-x86
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
