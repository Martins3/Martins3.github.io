## 代码来自于
https://github.com/pandengyang/peach/blob/master/guest/Makefile

## 编译的时候有这个警告
```txt
peach/peach.o: warning: objtool: .text+0xd5f: 'naked' return found in MITIGATION_RETHUNK build
peach/peach.o: warning: objtool: handle_vmexit() falls through to next function peach_ioctl()
peach/peach.o: warning: objtool: _vmexit_handler() is missing an ELF size annotation
peach/peach.o: warning: objtool: handle_vmexit+0x193: unknown CFA base reg -1
call this in kernel environment
```

简化一下代码吧，全是 asm ，用这个观察一下 vmenter 和 vmexit ，放到放到虚拟机中去测试，
用于测试嵌套虚拟化还是就不错的。

## 这里的代码，也是有 static call

和 APIC 是一样的实现原理吗?
```c
static u64 native_steal_clock(int cpu)
{
	return 0;
}

DEFINE_STATIC_CALL(pv_steal_clock, native_steal_clock);
DEFINE_STATIC_CALL(pv_sched_clock, native_sched_clock);

void paravirt_set_sched_clock(u64 (*func)(void))
{
	static_call_update(pv_sched_clock, func);
}
```

## 也许这个东西会更加好
https://seiya.me/blog/riscv-hypervisor

[转][译] 100 行 C 代码创建一个 KVM 虚拟机（2019） - 李睿的文章 - 知乎
https://zhuanlan.zhihu.com/p/701258802

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
