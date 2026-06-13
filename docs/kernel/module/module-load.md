# 思考 module load 的两个问题

## 有的模块可以安装 cpu model 来加载
drivers/char/hw_random/via-rng.c

/lib/modules/5.10.0/kernel/drivers/char/hw_random/via-rng.ko

```txt
[root@localhost ~]# modinfo zhaoxin-rng
filename:       /lib/modules/5.10.0/kernel/drivers/char/hw_random/zhaoxin-rng.ko
license:        GPL
description:    H/W RNG driver for Zhaoxin CPU with PadLock
srcversion:     AA94E1AFB2088D27FFE697E
alias:          cpu:type:x86,ven000Afam0007mod*:feature:*00A2*
alias:          cpu:type:x86,ven0005fam0007mod*:feature:*00A2*
alias:          cpu:type:x86,ven*fam*mod*:feature:*00A2*
```

```txt
[root@localhost ~]# modinfo via-rng
filename:       /lib/modules/5.10.0/kernel/drivers/char/hw_random/via-rng.ko
license:        GPL
description:    H/W RNG driver for VIA CPU with PadLock
srcversion:     862BD7102052177A7BEAB7D
alias:          cpu:type:x86,ven0005fam0006mod*:feature:*
alias:          cpu:type:x86,ven*fam*mod*:feature:*00A2*
```


```c
static struct x86_cpu_id zhaoxin_rng_ids[] = {
	{ X86_VENDOR_CENTAUR, 7, X86_MODEL_ANY, X86_STEPPING_ANY, X86_FEATURE_XSTORE },
	{ X86_VENDOR_ZHAOXIN, 7, X86_MODEL_ANY, X86_STEPPING_ANY, X86_FEATURE_XSTORE },
	{}
};
```

```c
static struct x86_cpu_id via_rng_ids[] = {
	{ X86_VENDOR_CENTAUR, 6, X86_MODEL_ANY, X86_FEATURE_XSTORE },
	{}
};
```

## module-load.d 中如何没有特定的模块，那么会导致后续失败吗？

https://www.freedesktop.org/software/systemd/man/latest/systemd-modules-load.service.html

/usr/lib/systemd/systemd-modules-load -h

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
