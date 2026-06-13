## ubsan

Documentation/dev-tools/ubsan.rst
  - https://www.kernel.org/doc/html/latest/dev-tools/ubsan.html


在没有打开 CONFIG_UBSAN=y 的情况下， 可以给一个模块打开吗?

跑的测试一下再说吧，感觉不是很靠谱的样子啊 这就是全部的东西 lib/ubsan.c ?

而且 ub 不是静态检查的时候就可以发现的吗?

## 效果
```txt
[ 6968.674846] ------------[ cut here ]------------
[ 6968.675334] UBSAN: array-index-out-of-bounds in arch/aarch64/sysreg.c:63:3
[ 6968.676013] index 2 is out of range for type 'int [2]'
[ 6968.676042] CPU: 1 UID: 0 PID: 2633 Comm: tee Tainted: G           O        6.16.0 #40 PREEMPT(full)
[ 6968.676047] Tainted: [O]=OOT_MODULE
[ 6968.676049] Hardware name: QEMU KVM Virtual Machine, BIOS 0.0.0 02/06/2015
[ 6968.676056] Call trace:
[ 6968.676059]  show_stack+0x34/0x98 (C)
[ 6968.676071]  dump_stack_lvl+0x7c/0xb0
[ 6968.676076]  dump_stack+0x18/0x24
[ 6968.676079]  ubsan_epilogue+0x10/0x48
[ 6968.676084]  __ubsan_handle_out_of_bounds+0xa0/0xd0
[ 6968.676092]  test_sysreg+0x268/0x280 [martins3]
[ 6968.676104]  sysreg_store+0xd0/0x120 [martins3]
[ 6968.676110]  kobj_attr_store+0x18/0x30
[ 6968.676116]  sysfs_kf_write+0x58/0x90
[ 6968.676121]  kernfs_fop_write_iter+0x134/0x208
[ 6968.676124]  vfs_write+0x224/0x3b0
[ 6968.676130]  ksys_write+0x78/0x120
[ 6968.676133]  __arm64_sys_write+0x24/0x40
[ 6968.676135]  invoke_syscall.constprop.0+0x58/0xf0
[ 6968.676138]  do_el0_svc+0x48/0xf0
[ 6968.676141]  el0_svc+0x5c/0x240
[ 6968.676147]  el0t_64_sync_handler+0x10c/0x138
[ 6968.676150]  el0t_64_sync+0x198/0x1a0
[ 6968.676154] ---[ end trace ]---
[ 6968.676380] 1b0020 13c039
```

## ubsan 的原理是什么?

先需要看
https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html

https://maskray.me/blog/2023-01-29-all-about-undefined-behavior-sanitizer

然后看 lib/ubsan.h 中，估计就是当添加的代码检查出来错误，那么会直接跳转到对应的位置上:

## 类似的 san
https://github.com/junwha/awesome-sanitizer

- https://github.com/realtime-sanitizer/rtsan

## 替换内核启动，有时候可以注意到
```txt
fuse: Unknown symbol __ubsan_handle_out_of_bounds (err -2)
fuse: Unknown symbol __ubsan_handle_load_invalid_value (err -2)
fuse: Unknown symbol __ubsan_handle_out_of_bounds (err -2)
fuse: Unknown symbol __ubsan_handle_load_invalid_value (err -2)
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
