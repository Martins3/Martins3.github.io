## 构建 qemu guest 镜像

- https://nixos.mayflower.consulting/blog/2018/09/11/custom-images/

虽然执行有点问题，但是值得借鉴:

```sh
#! /nix/store/96ky1zdkpq871h2dlk198fz0zvklr1dr-bash-5.1-p16/bin/bash

export PATH=/nix/store/wxb674h6dp7h63na8z6jwpagps811jl7-coreutils-9.1/bin${PATH:+:}$PATH

set -e

NIX_DISK_IMAGE=$(readlink -f "${NIX_DISK_IMAGE:-./nixos.qcow2}")

if ! test -e "$NIX_DISK_IMAGE"; then
    /nix/store/zsf59dn5sak8pbq4l3g5kqp7adyv3fph-qemu-host-cpu-only-7.1.0/bin/qemu-img create -f qcow2 "$NIX_DISK_IMAGE" \
      1024M
fi

# Create a directory for storing temporary data of the running VM.
if [ -z "$TMPDIR" ] || [ -z "$USE_TMPDIR" ]; then
    TMPDIR=$(mktemp -d nix-vm.XXXXXXXXXX --tmpdir)
fi



# Create a directory for exchanging data with the VM.
mkdir -p "$TMPDIR/xchg"



cd "$TMPDIR"




# Start QEMU.
exec /nix/store/zsf59dn5sak8pbq4l3g5kqp7adyv3fph-qemu-host-cpu-only-7.1.0/bin/qemu-kvm -cpu max \
    -name nixos \
    -m 1024 \
    -smp 1 \
    -device virtio-rng-pci \
    -net nic,netdev=user.0,model=virtio -netdev user,id=user.0,"$QEMU_NET_OPTS" \
    -virtfs local,path=/nix/store,security_model=none,mount_tag=nix-store \
    -virtfs local,path="${SHARED_DIR:-$TMPDIR/xchg}",security_model=none,mount_tag=shared \
    -virtfs local,path="$TMPDIR"/xchg,security_model=none,mount_tag=xchg \
    -drive cache=writeback,file="$NIX_DISK_IMAGE",id=drive1,if=none,index=1,werror=report -device virtio-blk-pci,drive=drive1 \
    -device virtio-keyboard \
    -usb \
    -device usb-tablet,bus=usb-bus.0 \
    -kernel ${NIXPKGS_QEMU_KERNEL_nixos:-/nix/store/k9xnkgjs5dwjzww8n9c3dsx3hl7axl5k-nixos-system-nixos-22.11.2999.a7cc81913bb/kernel} \
    -initrd /nix/store/k9xnkgjs5dwjzww8n9c3dsx3hl7axl5k-nixos-system-nixos-22.11.2999.a7cc81913bb/initrd \
    -append "$(cat /nix/store/k9xnkgjs5dwjzww8n9c3dsx3hl7axl5k-nixos-system-nixos-22.11.2999.a7cc81913bb/kernel-params) init=/nix/store/k9xnkgjs5dwjzw
w8n9c3dsx3hl7axl5k-nixos-system-nixos-22.11.2999.a7cc81913bb/init regInfo=/nix/store/byyk6x729q54ys1dv8m852v5f7g39ssn-closure-info/registration consol
e=ttyS0,115200n8 console=tty0 $QEMU_KERNEL_PARAMS" \
    $QEMU_OPTS \
    "$@"
```

- [ ] [Kernel Debugging with QEMU](https://nixos.wiki/wiki/Kernel_Debugging_with_QEMU) : 看上去这就是我们需要的，但是实际上，还是差点意思

  - https://wiki.cont.run/kernel-development-with-nix/
  - https://jade.fyi/blog/nixos-disk-images-m1/

- https://hoverbear.org/blog/nix-flake-live-media/

- [ ] https://jade.fyi/blog/nixos-disk-images-m1/

- [ ] https://mattwidmann.net/notes/running-nixos-in-a-vm/
- [ ] https://nixos.mayflower.consulting/blog/2018/09/11/custom-images/

感觉目前的时机不成熟，或者我对于这个的理解有问题。

- 因为 nixos 的 initrd 如果和 kernel 不匹配的话，应该启动不了

  - 使用 execsnoop 看启动参数吧

- 确实提供过如何制作 make-disk-image.nix 的操作，但是还是远远不够
- https://github.com/NixOS/nixpkgs/blob/master/nixos/lib/make-disk-image.nix
- https://github.com/NixOS/nixpkgs/blob/master/nixos/modules/profiles/qemu-guest.nix

- 有很多人介绍 nixos 如何制作出来 iso 的，然后再去安装，其实也算是一个路径，但是 -kernel 问题必须解决。

总之，等我对于 nixos 理解在深入一点再来搞这个问题吧。

而且，无论如何，都是需要在 guest 中使用 crash 的。

在 guest 中使用 docker 环境？

不要把简单问题复杂化了！

使用 shell 初始化即可，遇到问题，以后再说。

而且导致无法 dracut

虽然尝试将其作为完全的测试的 Guest 是失败了，但是
使用 nixos 搭建一个和 host 机器完全相同的虚拟机，然后可以实现 host guest 环境对比

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
