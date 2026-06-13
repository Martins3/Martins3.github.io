# kexec

## 利用 kexec 实现快速重启
<!-- ce1d8a0a-715e-4b0e-aae7-f8ad7cc80668 -->

```sh
sudo kexec -l /boot/vmlinuz-$(uname -r) --initrd=/boot/initramfs-$(uname -r).img --reuse-cmdline
sudo systemctl kexec
```
参考:
- https://wiki.archlinux.org/title/Kexec
- https://blogs.oracle.com/linux/post/reboot-faster-with-kexec

sudo systemctl kexec 和 sudo kexec -e 有什么区别:
- systemd 会先关闭各种服务


使用 ASUS 测试，没有任何错误，而使用 kunpeng 服务器测试
的确观察到了异常日志，但是勉强可以接受的:
```txt
[   35.266472] irq 24: nobody cared (try booting with the "irqpoll" option)
[   35.273846] CPU: 0 UID: 0 PID: 0 Comm: swapper/0 Tainted: G            E       6.16.4 #10 NONE
[   35.273850] Tainted: [E]=UNSIGNED_MODULE
[   35.273851] Hardware name: Yunke China KunTai R722/BC82AMDDRA, BIOS 1.89 05/20/2022
[   35.273852] Call trace:
[   35.273854]  show_stack+0x34/0x98 (C)
[   35.273860]  dump_stack_lvl+0x7c/0xa0
[   35.273863]  dump_stack+0x18/0x2c
[   35.273865]  __report_bad_irq+0x58/0x170
[   35.273869]  note_interrupt+0x23c/0x2a0
[   35.273872]  handle_irq_event+0xe0/0x110
[   35.273874]  handle_fasteoi_irq+0xb4/0x210
[   35.273876]  handle_irq_desc+0x3c/0x68
[   35.273881]  generic_handle_domain_irq+0x24/0x40
[   35.273884]  __gic_handle_irq_from_irqson.isra.0+0x154/0x288
[   35.273889]  gic_handle_irq+0x28/0x70
[   35.273891]  do_interrupt_handler+0x58/0x98
[   35.273895]  el1_interrupt+0x44/0xa0
[   35.273899]  el1h_64_irq_handler+0x18/0x28
[   35.273902]  el1h_64_irq+0x80/0x88
[   35.273904]  handle_softirqs+0xb8/0x330 (P)
[   35.273909]  __do_softirq+0x1c/0x28
[   35.273911]  ____do_softirq+0x18/0x30
[   35.273913]  call_on_irq_stack+0x30/0x48
[   35.273915]  do_softirq_own_stack+0x24/0x50
[   35.273918]  __irq_exit_rcu+0x13c/0x168
[   35.273921]  irq_exit_rcu+0x18/0x30
[   35.273924]  el1_interrupt+0x48/0xa0
[   35.273926]  el1h_64_irq_handler+0x18/0x28
[   35.273929]  el1h_64_irq+0x80/0x88
[   35.273930]  default_idle_call+0x38/0xf0 (P)
[   35.273934]  cpuidle_idle_call+0x17c/0x1d0
[   35.273939]  do_idle+0xf4/0x100
[   35.273941]  cpu_startup_entry+0x3c/0x50
[   35.273944]  rest_init+0xc4/0xd0
[   35.273947]  start_kernel+0x46c/0x478
[   35.273951]  __primary_switched+0x88/0x98
[   35.273954] handlers:
[   35.447366] [<00000000a2d2d29e>] serial8250_interrupt
[   35.453090] Disabling IRQ #24
```

## kdumpctl 两个命令的关系
<!-- f4229415-0e76-4dcf-8329-5369210c979c -->

sudo kdumpctl restart
sudo kdumpctl rebuild

manual 的说法:
```txt
       start  Start the service.
       rebuild the crash kernel initramfs.
```
就是这样的，restart 来加载内核到内存中，而 rebuild 是处理 initramfs 的

sudo kdumpctl restart 的动作类似:

1. 停掉已加载的 kdump 内核

* 卸载之前已经通过 `kexec -p` 加载的 crash kernel
* 确保旧的 dump capture kernel 不再驻留内存

2. 重新解析 `/etc/kdump.conf`

读取配置项：

* dump 路径（path）
* core_collector
* ssh/nfs/local dump target
* extra kernel args
* systemd unit overrides

3. 加载 crash kernel（remount crashkernel=xx）

执行:

```
kexec -p <path-to-capture-kernel> --append="<args>"
```

- rebuild 会做的事情：
1. 解析 `/etc/kdump.conf`
2. 找到当前的 crash kernel 版本
3. 使用 dracut 创建新的 initramfs
4. 把相关配置（网络、挂载点、过滤规则）全部打进 initramfs

- rebuild 不会做的事：
* 不会加载 crash kernel
* 不会启动/重启 kdump
* 不会修改 crashkernel= 内核参数

什么时候使用哪个？
1. 用 `restart` 的场景：
	* 修改 `/etc/kdump.conf` 的配置（大部分情况下）
	* crashkernel 大小已设置好（不用 rebuild）
	* 想确认 kdump 服务是否正常运行
2. 用 `rebuild` 的场景：
	* 增加新文件系统/驱动（ext4 → xfs）
	* 需要将新的网络配置加入 initramfs
	* dracut 模块变动
	* 升级 kernel 后 crash kernel 的 initramfs 损坏
	* 修改了需要写入 initramfs 的关键项（如 sshkey）

通常操作顺序：
```sh
sudo kdumpctl rebuild
sudo kdumpctl restart
```

## kexecboot
<!-- 33343c85-9478-4201-8773-027056818904 -->

- https://github.com/kexecboot/kexecboot
  - Kexecboot is a nice Linux-As-a-Bootloader implementation based on kexec.

使用 rasdaemon.nix 的环境
```txt
sh ./autogen.sh
./configure <options>
make
make install
```
但是这个程序依赖 /dev/fb0 ，不想调试了，就这样了。

我大致可以猜到这个项目想要实现的效果，估计是 kexec 来跳转
到他的这个 kernel ，然后他作为 bootloader 来加载系统中存在的 kernel 。

## /etc/sysconfig/kdump 和 /etc/kdump.conf 的作用

## kexec -l 和 kexec -p 的区别
<!-- f1a37096-5aa4-45b0-80b0-05175722f450 -->

```txt
 -l, --load           Load the new kernel into the current kernel.
 -p, --load-panic     Load the new kernel for use on panic.
```
-l 是把 kernel 加载到普通内存的区域，而 -p 是必须加载到 panic 的位置

应该是对应内核中的这两个定义:
```c
struct kimage *kexec_image;
struct kimage *kexec_crash_image;
```

## 一些有趣的探索
在 -kernel 模式启动的虚拟机可以正常的产生 vmcore 吗?

可以，而且很容易，就是 kt 安装需要的内容，然后 kdump.service 会自动处理

记住 kdump.service 是 kdumpctl 这个包提供的
```txt
 sudo systemctl status kdump.service
○ kdump.service - Crash recovery kernel arming
     Loaded: loaded (/usr/lib/systemd/system/kdump.service; disabled; preset: disabled)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: inactive (dead)
vn on  master 🗔
🤒  sudo systemctl status kdump.service
```

工具的依赖关系:
```txt
Installing:
 kdump-utils    x86_64 1.0.59-1.fc42  upda 290.3 KiB
Installing dependencies:
 dracut-network x86_64 107-4.fc42     upda 112.8 KiB
 dracut-squash  x86_64 107-4.fc42     upda   7.2 KiB
 makedumpfile   x86_64 1.7.8-1.fc42   upda 815.0 KiB
 squashfs-tools x86_64 4.6.1-6.fc42   fedo 653.6 KiB
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
