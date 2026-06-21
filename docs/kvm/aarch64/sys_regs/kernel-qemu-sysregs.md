## 3. QEMU 侧：TCG 模拟 与 KVM 状态同步

### 3.1 描述符 `ARMCPRegInfo` 和 `cp_regs` 哈希表

QEMU 在 `target/arm/cpregs.h` 定义：

```c
struct ARMCPRegInfo {
    const char *name;
    uint8_t cp, crn, crm, opc0, opc1, opc2;
    CPState state;
    int type;
    CPAccessRights access;
    ...
    ptrdiff_t fieldoffset;
    CPAccessFn *accessfn;
    CPReadFn *readfn;
    CPWriteFn *writefn;
    CPReadFn *raw_readfn;
    CPWriteFn *raw_writefn;
};
```

所有支持的寄存器在 `target/arm/helper.c` 的 `register_cp_regs_for_features()`
里按 CPU feature 分批注册：

```c
void register_cp_regs_for_features(ARMCPU *cpu)
{
    define_arm_cp_regs(cpu, cp_reginfo);
    if (arm_feature(env, ARM_FEATURE_V6)) {
        define_arm_cp_regs(cpu, v6_idregs);
        define_arm_cp_regs(cpu, v6_cp_reginfo);
    }
    if (arm_feature(env, ARM_FEATURE_V7)) { ... }
    if (arm_feature(env, ARM_FEATURE_V8)) { ... }
    ...
}
```

`define_one_arm_cp_reg()` 把描述符插到 `cpu->cp_regs` 哈希表里，key 是
`ENCODE_AA64_CP_REG()` 或 `ENCODE_CP_REG()`。

### 3.2 TCG 模式

在 TCG 下，翻译器遇到 `MRS/MSR` 时直接查 `cp_regs` 表，调用 `readfn/writefn`
或根据 `fieldoffset` 读写 `CPUARMState`。`ARMCPRegInfo` 里的 `accessfn`
做权限检查，`ARM_CP_CONST` 等类型处理特殊语义。

### 3.3 KVM 模式：运行时 offload 给内核

KVM 模式下，guest 真正跑在硬件虚拟化扩展上，sysreg trap 由 **内核** 处理。QEMU
不需要在运行时模拟这些寄存器，但它要负责：

1. 初始化时从内核拿到支持的寄存器列表
2. vCPU 运行前把 QEMU 侧状态写回内核
3. vCPU 退出后把内核状态读回 QEMU（用于迁移、调试等）

相关代码在 `target/arm/kvm.c`。

#### 3.3.1 初始化 `cpreg_indexes/values`

CPU realize 时先走 TCG 路径创建 `cp_regs` 哈希表，然后 `arm_cpu_realizefn()`
调用 `arm_init_cpreg_list()`（非 KVM）或 KVM 路径的
`kvm_arm_init_cpreg_list()`：

```c
static int kvm_arm_init_cpreg_list(ARMCPU *cpu)
{
    rl.n = 0;
    ret = kvm_vcpu_ioctl(cs, KVM_GET_REG_LIST, &rl);
    ...
    ret = kvm_vcpu_ioctl(cs, KVM_GET_REG_LIST, rlp);
    qsort(&rlp->reg, rlp->n, sizeof(rlp->reg[0]), compare_u64);

    for (...) {
        if (!kvm_arm_reg_syncs_via_cpreg_list(rlp->reg[i]))
            continue;
        cpu->cpreg_indexes[arraylen] = regidx;
        arraylen++;
    }
    cpu->cpreg_array_len = arraylen;

    write_kvmstate_to_list(cpu);
}
```

`kvm_arm_reg_syncs_via_cpreg_list()` 排除 `KVM_REG_ARM_CORE` 和
`KVM_REG_ARM64_SVE`，这两种由 QEMU 单独处理。

#### 3.3.2 运行时的同步

- Kernel -> QEMU：`kvm_arch_get_registers()` 调用
  `write_kvmstate_to_list(cpu)`，对每个 `cpreg_indexes[i]` 执行
  `kvm_get_one_reg()`，结果存到 `cpreg_values[i]`。
- QEMU -> Kernel：`kvm_arch_put_registers()` 调用
  `write_list_to_kvmstate(cpu, level)`，对每个索引执行 `kvm_set_one_reg()`。

#### 3.3.3 按 ID 快速查找

`cpreg_indexes` 是有序的，所以 QEMU 可以用二分查找定位某个 KVM ID 的存储位置：

```c
static uint64_t *kvm_arm_get_cpreg_ptr(ARMCPU *cpu, uint64_t regidx)
{
    uint64_t *res = bsearch(&regidx, cpu->cpreg_indexes, cpu->cpreg_array_len,
                            sizeof(uint64_t), compare_u64);
    return &cpu->cpreg_values[res - cpu->cpreg_indexes];
}
```

迁移时的 `kvm_arm_cpu_pre_save()` / `kvm_arm_cpu_post_load()`
就用这个机制读写虚拟时间等寄存器。

### 3.4 与迁移（migration）的关系

`VMStateDescription vmstate_arm_cpu` 保存 `cpreg_indexes` 和
`cpreg_values`。源端 save 时把当前值发出去；目的端 load 时：

1. `cpu_post_load()` 把 incoming 的 `cpreg_vmstate_indexes` 与本地
   `cpreg_indexes` 对齐。
2. 调用 `write_list_to_kvmstate(cpu, KVM_PUT_FULL_STATE)` 把所有 sysreg
   写回内核。
3. 再 `write_list_to_cpustate(cpu)` 让 QEMU 内部状态也同步。

如果两边 CPU 特性不同，导致 `kvm_set_one_reg()` 返回
`-ENOENT`/`-EINVAL`，就会出现经典的热迁移错误：

```text
qemu-system-aarch64: error while loading state for instance 0x0 of device 'cpu'
qemu-system-aarch64: load of migration failed: Operation not permitted
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
