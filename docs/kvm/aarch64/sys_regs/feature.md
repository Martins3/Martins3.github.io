## sys_reg_descs 的 filter 功能
<!-- e943dfb8-40ba-4cb4-a1f0-ff2db9f38e66 -->

例如这里我们的老朋友 AA64PFR0 的，
```c
static const struct sys_reg_desc sys_reg_descs[] = {
	// ...
	ID_FILTERED(ID_AA64PFR0_EL1, id_aa64pfr0_el1,
		    ~(ID_AA64PFR0_EL1_AMU |
		      ID_AA64PFR0_EL1_MPAM |
		      ID_AA64PFR0_EL1_SVE |
		      ID_AA64PFR0_EL1_AdvSIMD |
		      ID_AA64PFR0_EL1_FP)),
```
这个定义主要用来限制来自于用户态的写操作的，

> [!NOTE]
> 参考 Deepseeek ，有待验证

也就是说，过滤器将以下字段强制对用户空间 / KVM guest 不可见（置 0）：
- FP（浮点）
- AdvSIMD（NEON）
- SVE
- AMU
- MPAM

(无论如何，如果虚拟机中不能看到 fp 还是有点奇怪的)

## get_arm64_ftr_reg 的体系到底是什么?

- get_arm64_ftr_reg
	- get_arm64_ftr_reg_nowarn 获取到 sys_reg_descs 中定义的数值:

最经典的使用:
```c
u64 read_sanitised_ftr_reg(u32 id)
{
	struct arm64_ftr_reg *regp = get_arm64_ftr_reg(id);

	if (!regp)
		return 0;
	return regp->sys_val;
}
```

经典的使用为:
commit 011e5f5bf529 ("arm64/cpufeature: Add remaining feature bits in ID_AA64PFR0 register")

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
