- https://developers.redhat.com/blog/2015/03/24/live-migrating-qemu-kvm-virtual-machines#
- https://www.qemu.org/docs/master/devel/migration.html

## 无论如何，gdb 没有调试 source 端，gdb 调试的是 target 端，结果 target softlock 了

无法使用的状态，因为内存已经发送完成了，所以 ？？？

- 这是什么意思？
```txt
(qemu) info status
VM status: paused (postmigrate)
```

## 在自己的机器上测试一下
https://balamuruhans.github.io/2019/01/15/kvm-migration-with-qemu.html

qemu-system-ppc64 --enable-kvm --nographic -vga none -machine pseries -m 4G,slots=32,maxmem=32G -smp 16,maxcpus=32 -device virtio-blk-pci,drive=rootdisk -drive file=/var/lib/libvirt/images/avocado/data/avocado-vt/images/rhel72-ppc64le.qcow2,if=none,cache=none,format=qcow2,id=rootdisk -monitor telnet:127.0.0.1:1234,server,nowait -net nic,model=virtio -net user -redir tcp:2000::22

## 我靠，为什么 nvme 不是一个可以迁移的项目？
```txt
(qemu)  migrate -d tcp:localhost:4444
Error: State blocked by non-migratable device '0000:00:03.0:01.0/nvme'
```

## 似乎操作 vhost user 是有问题的
- loadvm_postcopy_handle_advise 中触发了这个错误：
```txt
qemu-system-x86_64: RAM postcopy is disabled but have 16 byte advise
qemu-system-x86_64: load of migration failed: Invalid argument
```

## 在一个机器上操作的时候

- 观测 host swap 的处理
- 观测 qcow2 的处理
- 迁移的时候

## 参考这个
- https://balamuruhans.github.io/2019/01/15/kvm-migration-with-qemu.html

```txt
migrate_set_capability postcopy-ram off
migrate -d tcp:localhost:4000
```