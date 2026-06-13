## rust-hypervisor-firmware

```sh
git clone https://news.ycombinator.com/item?id=19883626
# ins rust-best.nix
cargo build --release --target x86_64-unknown-none.json -Zbuild-std=core -Zbuild-std-features=compiler-builtins-mem

qemu-system-x86_64 -machine q35,accel=kvm -cpu host,-vmx -m 1G\
    -kernel ./target/x86_64-unknown-none/release/hypervisor-fw \
    -display none -nodefaults \
    -serial stdio \
    -drive id=os,file=/home/martins3/data/hack/vm/fedora-firecracker/img/boot1,if=none \
    -device virtio-blk-pci,drive=os,disable-legacy=on
```

似乎没有完全走通，不过没关系，这个的确是有趣的想法，一个介于 -kernel 直接启动内核和 edk2 之间的方案:
```txt
WARNING: Image format was not specified for '/home/martins3/data/hack/vm/fedora-firecracker/img/boot1' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
[INFO] Setting up 4 GiB identity mapping
[INFO] Page tables setup
[INFO] Booting with PVH Boot Protocol
[INFO] Found PCI device vendor=8086 device=29c0 in slot=0
[INFO] Found PCI device vendor=1af4 device=1042 in slot=1
[INFO] Found PCI device vendor=8086 device=2918 in slot=31
[INFO] PCI Device: 0:1.0 1af4:1042
[INFO] Bar: type=MemorySpace32 address=0x0 size=0x0
[INFO] Bar: type=MemorySpace32 address=0xfebfe000 size=0x1000
[INFO] Bar: type=MemorySpace32 address=0x0 size=0x0
[INFO] Bar: type=MemorySpace32 address=0x0 size=0x0
[INFO] Bar: type=MemorySpace64 address=0xfebf8000 size=0x4000
[INFO] Bar: type=Unused address=0x0 size=0x0
[INFO] Updated BARs: type=MemorySpace32 address=0 size=0
[INFO] Updated BARs: type=MemorySpace32 address=febfe000 size=1000
[INFO] Updated BARs: type=MemorySpace32 address=0 size=0
[INFO] Updated BARs: type=MemorySpace32 address=0 size=0
[INFO] Updated BARs: type=MemorySpace64 address=febf8000 size=4000
[INFO] Updated BARs: type=Unused address=0 size=0
[INFO] Virtio block device configured. Capacity: 83886080 sectors
[ERROR] Failed to find EFI partition: NoEFIPartition
PANIC: panicked at src/main.rs:286:5:
Unable to boot from any virtio-blk device
qemu-system-x86_64: terminating on signal 2
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
