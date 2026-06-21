# 分析 arm 的 cpuid 的代码

fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm jscvt fcma lrcpc dcpop sha3 asimddp sha512 asimdfhm dit uscat ilrcpc flagm ssbs sb paca pacg dcpodp flagm2 frint i8mm bf16 bti ecv

## ecv
```c
static __always_inline u64 __arch_counter_get_cntvct(void)
{
	u64 cnt;

	asm volatile(ALTERNATIVE("isb\n mrs %0, cntvct_el0",
				 "nop\n" __mrs_s("%0", SYS_CNTVCTSS_EL0),
				 ARM64_HAS_ECV)
		     : "=r" (cnt));
	arch_counter_enforce_ordering(cnt);
	return cnt;
}
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
