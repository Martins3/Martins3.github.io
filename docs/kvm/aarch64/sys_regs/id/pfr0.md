## aarch64 ID_AA64PFR0_EL1
<!-- 7a7e18cd-e55a-4b23-8911-da944db56dcf -->

PFR0 也是一个经典 prefix 了，就是用来描述一个 CPU 有那些寄存器的。


- ID_AA64PFR0_EL1 回答“这个 CPU 支持哪些 Exception Level、有没有 SVE/NEON/FP、RAS/AMU/MPAM 等”。
- ID_PFR0_EL1 回答“从 AArch32 视角看，这个 CPU 支持哪些指令集状态和特性”。

在 sys_reg_desc 中，这是定义了两个字段:
```c
static const struct sys_reg_desc sys_reg_descs[] = {
	AA32_ID_SANITISED(ID_PFR0_EL1),

	/* CRm=4 */
	ID_FILTERED(ID_AA64PFR0_EL1, id_aa64pfr0_el1,
		    ~(ID_AA64PFR0_EL1_AMU |
		      ID_AA64PFR0_EL1_MPAM |
		      ID_AA64PFR0_EL1_SVE |
		      ID_AA64PFR0_EL1_RAS |
		      ID_AA64PFR0_EL1_AdvSIMD |
		      ID_AA64PFR0_EL1_FP)),

```


AArch64 和 AArch32 之间的切换只能通过异常入口/异常返回完成：
- AArch64 -> AArch32：在 AArch64 里设置目标 EL 的 SPSR.M[4] = 1，然后 ERET。CPU 就会切到 AArch32 状态运行。
- AArch32 -> AArch64：触发异常进入更高 EL，如果那个 EL 是 AArch64，就自然切回来了。

同一个 EL 内部不能随机切换，因为每个 EL 有自己的执行状态。




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
