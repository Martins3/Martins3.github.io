## x86 的 kvm 构建可以不用整个 tree 吗?

似乎现在可以做到的，先需要构建 tree ，之后每次修改不需要。

使用这个命令的时候发现
```sh
		run "make arch/x86/kvm/ -j$core"
		run "make M=./arch/x86/kvm/ -j$core"
```

```txt
WARNING: /home/martins3/data/kernel/linux-nixos/Module.symvers is missing.
         Modules may not have dependencies or modversions.
         You may get many unresolved symbol errors.
         You can set KBUILD_MODPOST_WARN=1 to turn errors into warning
         if you want to proceed at your own risk.
ERROR: modpost: "_raw_spin_unlock_irq" [kvm.ko] undefined!
ERROR: modpost: "_raw_read_trylock" [kvm.ko] undefined!
ERROR: modpost: "preempt_notifier_register" [kvm.ko] undefined!
ERROR: modpost: "finish_rcuwait" [kvm.ko] undefined!
ERROR: modpost: "__SCK__tp_func_ipi_send_cpu" [kvm.ko] undefined!
ERROR: modpost: "trace_event_reg" [kvm.ko] undefined!
ERROR: modpost: "mutex_is_locked" [kvm.ko] undefined!
ERROR: modpost: "rb_erase" [kvm.ko] undefined!
ERROR: modpost: "fpu_free_guest_fpstate" [kvm.ko] undefined!
ERROR: modpost: "clear_page_erms" [kvm.ko] undefined!
WARNING: modpost: suppressed 716 unresolved symbol warnings because there were too many)
make[3]: *** [/home/martins3/data/kernel/linux-nixos/scripts/Makefile.modpost:147: Module.symvers] Error 1
make[2]: *** [/home/martins3/data/kernel/linux-nixos/Makefile:1944: modpost] Error 2
make[1]: *** [/home/martins3/data/kernel/linux-nixos/Makefile:251: __sub-make] Error 2
make[1]: Leaving directory '/home/martins3/data/kernel/linux-nixos/arch/x86/kvm'
make: *** [Makefile:251: __sub-make] Error 2
```
似乎都是 modpost 的时候问题，但是可以跳过吗，或者自动生成一下这个东西。


## 记录

在基础上打了几个 patch ，然后发现:
```txt
[  995.134750] kvm: Unknown symbol irq_bypass_unregister_consumer (err -2)
[  995.142583] kvm: Unknown symbol irq_bypass_register_consumer (err -2)


[ 1073.515178] BPF:      type_id=9110 bits_offset=896
[ 1073.520693] BPF:
[ 1073.523644] BPF: Invalid name
[ 1073.527527] BPF:
[ 1073.530362] failed to validate module [kvm] BTF: -22
[ 1080.669184] usb 1-14.4-port2: attempt power cycle
[ 1090.229312] usb 1-14.4-port2: unable to enumerate USB device
[ 1094.334874] BPF:      type_id=12 bits_offset=64
[ 1094.340200] BPF:
[ 1094.343208] BPF: Invalid name
[ 1094.347106] BPF:
[ 1094.349961] failed to validate module [kvm] BTF: -22
```

## 很多模块可以
可以用类似 kvm 的模式，现构建整个 kernel ，也可以利用 kernel-devel
进入到 docker 环境中:
```sh
rpm -e --nodeps kernel-devel
# 安装 kernel-devel 4.19
cd drivers/md
make -C /lib/modules/4.19/build/ M=$(pwd) -j32
```

为什么 kvm 不可以用这种方法，实测下来，会有错误:

```sh
cd /root/arch/x86/kvm
make -C /lib/modules/4.19/build/ M=$(pwd) -j32
```

```txt
bash-5.0# make -C /lib/modules/martins3-4.19.x86_64/build/ M=./arch/x86/kvm/
make: Entering directory '/usr/src/kernels/martins3-4.19.x86_64'
make[1]: *** No rule to make target 'arch/x86/kvm//../../../virt/kvm/kvm_main.o', needed by 'arch/x86/kvm//kvm.o'.  Stop.
make: *** [Makefile:1524: _module_./arch/x86/kvm/] Error 2
make: Leaving directory '/usr/src/kernels/martins3-4.19.x86_64'
 ```

有时候遇到这个错误:
```txt
[304862.113284] irqbypass: version magic '5.10.00421551c84d0 SMP mod_unload ' should be '5.10.0 SMP mod_unload modversions '
[304862.166664] kvm: version magic '5.10.00421551c84d0 SMP mod_unload ' should be '5.10.0 SMP mod_unload modversions '
[304862.216838] kvm_intel: version magic '5.10.00421551c84d0 SMP mod_unload ' should be '5.10.0 SMP mod_unload modversions '
```


## 问题
make arch/x86/kvm/ 和 make M=./arch/x86/kvm/ 区别是什么?
说是 Documentation/kbuild/modules.rst 中可以解释，但是可能还是需要看看源码了

## TODO
- https://askubuntu.com/questions/168279/how-do-i-build-a-single-in-tree-kernel-module

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
