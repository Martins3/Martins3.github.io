# 用户态访问 sys_reg 是如何模拟的

利用这两个文件测试:
-  mrs-sysreg.bt
-  msr-sysreg.c

通过 bpftrace 可以观察到:
```txt
MRS     pid=682948 comm=aa64dfr0.out     pc=0xffff879b2b90     insn=0xd5380001 sys_reg=0x180000 op0=3 op1=0 CRn=0 CRm=0 op2=0 Rt=X1 name=MIDR_EL1
MRS     pid=682948 comm=aa64dfr0.out     pc=0xaaaaca720ac0     insn=0xd5380500 sys_reg=0x180500 op0=3 op1=0 CRn=0 CRm=5 op2=0 Rt=X0 name=ID_AA64DFR0_EL1
```
我想这个问题已经是无比清晰了，aarch64 下，用户态执行 msr 指令的时候，
会触发 illegal instruction ，然后取决于内核的行为，来返回一个 value 给用户


用户态直接读取 ID_AA64DFR0_EL1 ，结果是 0x6

这个结果完美的对应 arch/arm64/kernel/cpufeature.c:ftr_id_aa64dfr0 中的结果
```c
static const struct arm64_ftr_bits ftr_id_aa64dfr0[] = {
	S_ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64DFR0_EL1_DoubleLock_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_NONSTRICT, FTR_LOWER_SAFE, ID_AA64DFR0_EL1_PMSVer_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64DFR0_EL1_CTX_CMPs_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64DFR0_EL1_WRPs_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64DFR0_EL1_BRPs_SHIFT, 4, 0),
	/*
	 * We can instantiate multiple PMU instances with different levels
	 * of support.
	 */
	S_ARM64_FTR_BITS(FTR_HIDDEN, FTR_NONSTRICT, FTR_EXACT, ID_AA64DFR0_EL1_PMUVer_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_EXACT, ID_AA64DFR0_EL1_DebugVer_SHIFT, 4, 0x6),
	ARM64_FTR_END,
};
```

如果想要获取到真正的结果，需要在内核中观察:

利用 m/arch/aarch64/sysreg.c :

```c
pr_info("%x %llx %llx\n", SYS_ID_AA64DFR0_EL1,
        read_sysreg_s(SYS_ID_AA64DFR0_EL1),
        read_sanitised_ftr_reg(SYS_ID_AA64DFR0_EL1));
```
```
[147166.643998] 180500 110305408 110305408
```

read_sysreg_s 就是直接从硬件中读取的原始数值
read_sanitised_ftr_reg 内核保存并“sanitised”后的特征寄存器值。

将 `0x0000000110305408` 按位域拆开：

| 位域    | 字段        | 值    | 意义                                  |
|---------|-------------|-------|---------------------------------------|
| [3:0]   | DebugVer    | `0x8` | ARMv8.4 调试架构                      |
| [7:4]   | TraceVer    | `0x0` | 不支持 Trace 系统寄存器接口           |
| [11:8]  | PMUVer      | `0x4` | PMUv3，带 `FEAT_PMUv3p4` 扩展         |
| [15:12] | BRPs        | `0x5` | 硬件断点数量 = 5 + 1 = **6 个**       |
| [19:16] | WRPs        | `0x0` | 观察点数量 = 0 + 1 = **1 个**         |
| [23:20] | CTX_CMPs    | `0x3` | 上下文感知断点数量 = 3 + 1 = **4 个** |
| [27:24] | TraceBuffer | `0x0` | 无 Trace Buffer                       |
| [31:28] | TraceFilt   | `0x1` | 实现 `FEAT_TRF`（Trace 过滤）         |
| [35:32] | MTPMU       | `0x1` | 实现 `FEAT_MTPMU`（多线程 PMU 扩展）  |

## exception 大致的数据流流程

```txt
@[
        try_emulate_mrs+0
        el0_undef+56
        el0t_64_sync_handler+216
        el0t_64_sync+420
]: 87
```

使用 ftrace 可以观察到如下的结果:
```text
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 18)               |  try_emulate_mrs() {
 18)   0.430 us    |    aarch64_insn_decode_immediate();
 18)   0.230 us    |    aarch64_insn_decode_register();
 18)               |    do_emulate_mrs() {
 18)               |      arm64_skip_faulting_instruction() {
 18)   0.230 us    |        user_fastforward_single_step();
 18)   0.940 us    |      }
 18)   1.510 us    |    }
 18)   5.700 us    |  }
 18)               |  try_emulate_mrs() {
 18)   0.330 us    |    aarch64_insn_decode_immediate();
 18)   0.230 us    |    aarch64_insn_decode_register();
 18)               |    do_emulate_mrs() {
 18)               |      arm64_skip_faulting_instruction() {
 18)   0.230 us    |        user_fastforward_single_step();
 18)   0.750 us    |      }
 18)   1.200 us    |    }
 18)   3.430 us    |  }
 18)               |  try_emulate_mrs() {
 18)   0.230 us    |    aarch64_insn_decode_immediate();
 18)   0.230 us    |    aarch64_insn_decode_register();
 18)               |    do_emulate_mrs() {
 18)               |      arm64_skip_faulting_instruction() {
 18)   0.280 us    |        user_fastforward_single_step();
 18)   0.720 us    |      }
 18)   1.170 us    |    }
 18)   2.670 us    |  }
 19)               |  try_emulate_mrs() {
 19)   0.330 us    |    aarch64_insn_decode_immediate();
 19)   0.240 us    |    aarch64_insn_decode_register();
 19)               |    do_emulate_mrs() {
 19)               |      arm64_skip_faulting_instruction() {
 19)   0.230 us    |        user_fastforward_single_step();
 19)   0.720 us    |      }
 19)   1.200 us    |    }
 19)   3.630 us    |  }
 19)               |  try_emulate_mrs() {
 19)   0.300 us    |    aarch64_insn_decode_immediate();
 19)   0.230 us    |    aarch64_insn_decode_register();
 19)               |    do_emulate_mrs() {
 19)   0.240 us    |      search_cmp_ftr_reg();
 19)   0.330 us    |      search_cmp_ftr_reg();
 19)   0.220 us    |      search_cmp_ftr_reg();
 19)   0.220 us    |      search_cmp_ftr_reg();
 19)   0.230 us    |      search_cmp_ftr_reg();
 19)               |      arm64_skip_faulting_instruction() {
 19)   0.230 us    |        user_fastforward_single_step();
 19)   0.670 us    |      }
 19)   3.750 us    |    }
 19)   6.090 us    |  }
 21)               |  try_emulate_mrs() {
 21)   0.480 us    |    aarch64_insn_decode_immediate();
 21)   0.230 us    |    aarch64_insn_decode_register();
 21)               |    do_emulate_mrs() {
 21)               |      arm64_skip_faulting_instruction() {
 21)   0.230 us    |        user_fastforward_single_step();
 21)   0.880 us    |      }
 21)   1.440 us    |    }
 21)   5.180 us    |  }
 ------------------------------------------
 19) aa64dfr-677583 =>  sudo-677586
 ------------------------------------------

 19)               |  try_emulate_mrs() {
 19)   0.390 us    |    aarch64_insn_decode_immediate();
 19)   0.230 us    |    aarch64_insn_decode_register();
 19)               |    do_emulate_mrs() {
 19)               |      arm64_skip_faulting_instruction() {
 19)   0.230 us    |        user_fastforward_single_step();
 19)   0.820 us    |      }
 19)   1.340 us    |    }
 19)   5.030 us    |  }
 20)               |  try_emulate_mrs() {
 20)   0.420 us    |    aarch64_insn_decode_immediate();
 20)   0.220 us    |    aarch64_insn_decode_register();
 20)               |    do_emulate_mrs() {
 20)               |      arm64_skip_faulting_instruction() {
 20)   0.230 us    |        user_fastforward_single_step();
 20)   0.760 us    |      }
 20)   1.270 us    |    }
 20)   5.340 us    |  }
 ------------------------------------------
 21)  sleep-677584  =>   cat-677588
 ------------------------------------------

 21)               |  try_emulate_mrs() {
 21)   0.470 us    |    aarch64_insn_decode_immediate();
 21)   0.240 us    |    aarch64_insn_decode_register();
 21)               |    do_emulate_mrs() {
 21)               |      arm64_skip_faulting_instruction() {
 21)   0.230 us    |        user_fastforward_single_step();
 21)   0.730 us    |      }
 21)   1.210 us    |    }
 21)   4.070 us    |  }
```

1. 调用链稳定为：
   ```
   try_emulate_mrs
     -> aarch64_insn_decode_immediate
     -> aarch64_insn_decode_register
     -> do_emulate_mrs
          -> [search_cmp_ftr_reg x N]   # ID_AA64DFR0_EL1 等会触发
          -> arm64_skip_faulting_instruction
               -> user_fastforward_single_step
   ```

2. 耗时：单次 `try_emulate_mrs` 总耗时约 2.7 us ~ 6.1 us。
   - 解码指令和寄存器各约 0.2 us ~ 0.5 us。
   - `do_emulate_mrs` 主体约 1.2 us ~ 3.8 us。
   - 包含多次 `search_cmp_ftr_reg` 的路径耗时更长（如 `ID_AA64DFR0_EL1` 这次约 6.09 us）。

3. 进程切换标记：trace 中出现了 `aa64dfr-677583 => sudo-677586` 和 `sleep-677584 => cat-677588`，
   说明在收集 trace 期间也记录了上下文切换事件。


其细节在 emulate_sys_reg 中，这无需多言，就是最后访问一下 user_val 就可以
```c
static int emulate_sys_reg(u32 id, u64 *valp)
{
	struct arm64_ftr_reg *regp;

	if (!is_emulated(id))
		return -EINVAL;

	if (sys_reg_CRm(id) == 0)
		return emulate_id_reg(id, valp);

	regp = get_arm64_ftr_reg_nowarn(id);
	if (regp)
		*valp = arm64_ftr_reg_user_value(regp);
	else
		/*
		 * The untracked registers are either IMPLEMENTATION DEFINED
		 * (e.g, ID_AFR0_EL1) or reserved RAZ.
		 */
		*valp = 0;
	return 0;
}
```

最后数据的构成:
```c
static inline u64 arm64_ftr_reg_user_value(const struct arm64_ftr_reg *reg)
{
	return (reg->user_val | (reg->sys_val & reg->user_mask));
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
