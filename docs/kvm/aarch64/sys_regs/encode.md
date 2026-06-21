# aarch64 sysregs 编解码
<!-- 1b77567b-58f8-4cf1-9a3e-f6c6925550c1 -->

配套代码  : code/src/m/arch/aarch64/sysreg.c :

## sys_reg() 编码
首先，这是 sys reg 的标准定义:
```c
#define SYS_PMMIR_EL1			sys_reg(3, 0, 9, 14, 6)

#define sys_reg(op0, op1, crn, crm, op2) \
	(((op0) << Op0_shift) | ((op1) << Op1_shift) | \
	 ((crn) << CRn_shift) | ((crm) << CRm_shift) | \
	 ((op2) << Op2_shift))
```
这个寄存器真实编码，也就是 msr 指令的操作数值:
https://developer.arm.com/documentation/101800/0201/AArch64-registers/AArch64-Performance-Monitors-register-summary/PMMIR-EL1--Performance-Monitors-Machine-Identification-Register
```txt
MRS <Xt>, PMMIR_EL1
```

例如 SYS_ID_AA64MMFR1_EL1 编码就是 180720

## kvm index 编码
```c
struct kvm_one_reg {
	__u64 id;
	__u64 addr;
};
```

```c
static u64 sys_reg_to_index(const struct sys_reg_desc *reg)
{
	return (KVM_REG_ARM64 | KVM_REG_SIZE_U64 |
		KVM_REG_ARM64_SYSREG |
		(reg->Op0 << KVM_REG_ARM64_SYSREG_OP0_SHIFT) |
		(reg->Op1 << KVM_REG_ARM64_SYSREG_OP1_SHIFT) |
		(reg->CRn << KVM_REG_ARM64_SYSREG_CRN_SHIFT) |
		(reg->CRm << KVM_REG_ARM64_SYSREG_CRM_SHIFT) |
		(reg->Op2 << KVM_REG_ARM64_SYSREG_OP2_SHIFT));
}
```

例如 SYS_ID_AA64MMFR1_EL1 编码就是 603000000013c039


## 他们的关系
```txt
   ┌──────────┬─────────────────────────────────────────┬──────────────────────┐
   │          │ KVM_REG_* index                         │ sys_reg() 编码       │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ 用途     │ 用户态 KVM_GET_ONE_REG/KVM_SET_ONE_REG  │ 内核里查表、指令编码 │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ Op0 位置 │ bit 14-15                               │ bit 19-20            │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ Op1 位置 │ bit 11-13                               │ bit 16-18            │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ CRn 位置 │ bit 7-10                                │ bit 12-15            │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ CRm 位置 │ bit 3-6                                 │ bit 8-11             │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ Op2 位置 │ bit 0-2                                 │ bit 5-7              │
   ├──────────┼─────────────────────────────────────────┼──────────────────────┤
   │ 额外位   │ 含 KVM_REG_ARM64/SIZE_U64/SYSREG 等标志 │ 纯寄存器编码         │
   └──────────┴─────────────────────────────────────────┴──────────────────────┘
```

| 函数                 | 说明                                                                                       |
|----------------------|--------------------------------------------------------------------------------------------|
| id_to_sys_reg_desc   | 从 kvm_one_reg::id 到 sys_reg_desc ，通过 index_to_params + reg_to_encoding 来做编码的变换 |
| kvm_one_reg_to_id    | 获取 kvm_one_reg::id                                                                       |
| kvm_sys_reg_get_user | 调用 sys_reg_desc::get_user                                                                |

## 常见问题

### 通过 kvm_one_reg.id 如何快速找到是那个 reg ?
```c
#define SYS_CTR_EL0                                     sys_reg(3, 3, 0, 0, 1)
```


### 常见问题
面对这个日志，如果找到是哪一个寄存器:
```txt
[601122.158666] kvm [1391196]: Unsupported guest sys_reg access at: ffff80001003c2b4 [20400085]
[601122.172133] kvm [1391179]:  { Op0( 3), Op1( 0), CRn( 9), CRm(14), Op2( 6), func_read },
```
现在看，这个就很容易理解了，直接搜
arch/arm64/include/generated/asm/sysreg-defs.h 就可以了



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
