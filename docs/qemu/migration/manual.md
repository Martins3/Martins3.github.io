# how to migrate with latest QEMU

## 使用 libvirt
https://libvirt.org/migration.html#native-migration-client-to-and-peer2peer-between-two-libvirtd-servers
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/sect-kvm_live_migration-live_kvm_migration_with_virsh
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/virtualization_administration_guide/sect-virtualization-kvm_live_migration-live_kvm_migration_with_virsh

- 详细 : https://www.ibm.com/docs/en/linux-on-systems?topic=migration-migrate
- 简单 ：https://docs.fedoraproject.org/en-US/Fedora/13/html/Virtualization_Guide/sect-Virtualization-KVM_live_migration-Live_KVM_migration_with_virsh.html

## 直接使用 QEMU
并没有找到直接的操作的方法

## 似乎使用 unsafe 可以绕过
Unsafe migration: Migration without shared storage is unsafe

## 两个 disk 是需要拷贝的
但是，disk 是在动态的被修改的，how to handle this ?

## 使用这个命令

两边的路径需要保持一致:
virsh migrate --live nixos qemu+ssh://martins3@192.168.30.16/system --unsafe

但是:

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/virtualization_host_configuration_and_guest_installation_guide/app_tcp_ports

<!-- 看上去的确是这种修复的方法，但是遇到你新的问题。 -->

migration successfully aborted : 这里进行了一些解释 https://kubevirt.io/user-guide/operations/live_migration/#canceling-a-live-migration


使用这个
virsh -c qemu+ssh://martins3@192.168.30.16/system 是可以链接上的

真正修好 nix 的问题之后:

error: End of file while reading data: virt-ssh-helper: could not proxy traffic: Cannot recv data: Connection reset by peer: Input/output error

--------------> 可以从 B 中的 journal 中看看。

> 好的，现在虚拟机挂掉了。

## 其他的有趣尝试

没有办法自己热迁移自己的
```sh
virsh migrate --live b60d27f7-a198-45d4-8a7d-d2b816bc4bd6 qemu+ssh://martins3@127.0.0.1/system
```

到某一台机器上:
```txt
error: authentication unavailable: no polkit agent available to authenticate action 'org.libvirt.unix.manage'
```
