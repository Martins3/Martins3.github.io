# qemu 如何管理 sys_regs
<!-- 93ba9162-31e7-48cd-9af2-b03d71c05b1f -->

## ArchCPU 和 ARMCPRegInfo
**struct ARMCPRegInfo 和内核中 struct sys_reg_desc 对应的**，
其中定义类似 readfn 和 writefn ，主要用于 kvm 。

就是 qemu 为了模拟，会定义出来所有支持的 cp_regs
也就是 register_cp_regs_for_features ，对于每一个 CPU 会先调用
init_cpreg_list 做初始化，如果支持 kvm ，会去调用 kvm_arm_init_cpreg_list 来重新初始化
这些 cp_regs 的。

相关结构体都在这里，对于 kvm 而言，其实 ArchCPU::cp_regs 是不用的，直接从 kvm 中获取:

```c
struct ArchCPU {
    CPUState parent_obj;

    CPUARMState env;

    /* Coprocessor information */
    GHashTable *cp_regs;
    /* For marshalling (mostly coprocessor) register state between the
     * kernel and QEMU (for KVM) and between two QEMUs (for migration),
     * we use these arrays.
     */
    /* List of register indexes managed via these arrays; (full KVM style
     * 64 bit indexes, not CPRegInfo 32 bit indexes)
     */
    uint64_t *cpreg_indexes;
    /* Values of the registers (cpreg_indexes[i]'s value is cpreg_values[i]) */
    uint64_t *cpreg_values;
    /* Length of the indexes, values, reset_values arrays */
    int32_t cpreg_array_len;
    /* These are used only for migration: incoming data arrives in
     * these fields and is sanity checked in post_load before copying
     * to the working data structures above.
     */

    uint64_t *cpreg_vmstate_indexes;
    uint64_t *cpreg_vmstate_values;
    int32_t cpreg_vmstate_array_len;
```
这里的结果不是太难理解:
1. cpreg_indexes : 存储 reg 的编码
2. cpreg_vmstate_values : 存储 reg 数值
3. cp_regs : 存储 ARMCPRegInfo

ArchCPU::cpreg_array_len 和 ArchCPU::cpreg_vmstate_array_len 的关系
注释中也说明了，热迁移过来的不可以立刻使用。

## 基本流程

### 初始化
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_init_board
          - machine_run_board_init
            - machvirt_init
              - object_property_set_bool
                - object_property_set_qobject
                  - object_property_set
                    - property_set_bool
                      - device_set_realized
                        - arm_cpu_realizefn
                          - register_cp_regs_for_features : 在函数中定义出来各个 cp reg 的信息，然后调用辅助函数，注册进去
                            - define_arm_cp_regs_with_opaque_len
                              - define_one_arm_cp_reg_with_opaque
                                - add_cpreg_to_hashtable : 创建结构体 ARMCPRegInfo ，将结构体记录在 ArchCPU::cp_regs


kvm_arm_init_cpreg_list 和 init_cpreg_list 是什么关系?

首先调用:
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_init_board
          - machine_run_board_init
            - machvirt_init
              - object_property_set_bool
                - object_property_set_qobject
                  - object_property_set
                    - property_set_bool
                      - device_set_realized
                        - arm_cpu_realizefn
                          - init_cpreg_list

然后调用:
- thread_start
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_init_vcpu
          - kvm_arch_init_vcpu
            - kvm_arm_init_cpreg_list
              - kvm_vcpu_ioctl(cs, KVM_GET_REG_LIST, rlp) : 的确是获取所有的 regs，但是进一步调用的是 copy_core_reg_indices ，只是在获取 reg 的种类的而已
              - write_kvmstate_to_list : 在这里，将寄存器真正的数值保存在 ArchCPU::cpreg_values 中


将 arrlen 打印出来:
```txt
[martins3:init_cpreg_list:257] 277
[martins3:kvm_arm_init_cpreg_list:825] 256
```

### 热迁移

```c
const VMStateDescription vmstate_arm_cpu = {
    .name = "cpu",
    .version_id = 22,
    .minimum_version_id = 22,
    .pre_save = cpu_pre_save,
    .post_save = cpu_post_save,
    .pre_load = cpu_pre_load,
    .post_load = cpu_post_load,
    // ...
}
```

kvm_arm_cpu_pre_load 对于每一个 CPU 都会调用，
如果让 kvm_arm_cpu_pre_load 直接返回 false ，那么可以得到如下日志，
这个也是热迁移失败的经典日志了:
```txt
qemu-system-aarch64: error while loading state for instance 0x0 of device 'cpu'
qemu-system-aarch64: load of migration failed: Operation not permitted
```

核心在 kvm_arch_get_registers 和 kvm_arch_put_registers 了
也就是各种寄存器的维护:

```c
ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(regs.regs[i]), &env->xregs[i]);
ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(regs.sp), &env->sp_el[0]);
ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(sp_el1), &env->sp_el[1]);
ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(regs.pstate), &val);
ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(regs.pc), &env->pc);
ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(elr_el1), &env->elr_el[1]);

ret = kvm_get_one_reg(cs, AARCH64_CORE_REG(spsr[i]), &env->banked_spsr[i + 1]);
ret = kvm_arch_get_sve(cs);
ret = kvm_arch_get_fpsimd(cs);
ret = kvm_get_one_reg(cs, AARCH64_SIMD_CTRL_REG(fp_regs.fpsr), &fpr);
ret = kvm_get_one_reg(cs, AARCH64_SIMD_CTRL_REG(fp_regs.fpcr), &fpr);
```

#### 调试技巧

原来开机的时候有一个配套的 write_list_to_kvmstate 和 write_kvmstate_to_list 来配合使用的。

如果发现了有热迁移兼容性问题，那么
write_list_to_kvmstate 中添加注释，然后启动虚拟机就可以了:

- thread_start
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - qemu_wait_io_event
          - process_queued_cpu_work
            - do_kvm_cpu_synchronize_post_init
              - kvm_arch_put_registers
                - write_list_to_kvmstate


## qemu 也有自己的编码

KVM 初始化 / 迁移时，QEMU 用 cpreg_to_kvm_id() 把 ARMCPRegInfo 的 key 转成 KVM_REG_ARM64_* 的 64 位 ID，
通过 KVM_GET/SET_ONE_REG 与内核交换寄存器值。内核里的 sys_reg_desc 表就是根据这个 ID 来解码并处理读写的。

```c
#define ENCODE_AA64_CP_REG(op0, op1, crn, crm, op2) \
    (CP_REG_AA64_MASK | CP_REG_ARM64_SYSREG |           \
     ((op0) << CP_REG_ARM64_SYSREG_OP0_SHIFT) |         \
     ((op1) << CP_REG_ARM64_SYSREG_OP1_SHIFT) |         \
     ((crn) << CP_REG_ARM64_SYSREG_CRN_SHIFT) |         \
     ((crm) << CP_REG_ARM64_SYSREG_CRM_SHIFT) |         \
     ((op2) << CP_REG_ARM64_SYSREG_OP2_SHIFT))
```

```c
static inline uint32_t kvm_to_cpreg_id(uint64_t kvmid)
{
    uint32_t cpregid = kvmid;
    if ((kvmid & CP_REG_ARCH_MASK) == CP_REG_ARM64) {
        cpregid |= CP_REG_AA64_MASK;
    } else {
        ...
        cpregid |= CP_REG_AA32_NS_MASK;
    }
    return cpregid;
}

static inline uint64_t cpreg_to_kvm_id(uint32_t cpregid)
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
