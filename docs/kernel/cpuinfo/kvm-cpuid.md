[cpuid 必然导致 vmexit 的](https://stackoverflow.com/questions/63214415/does-hypervisor-like-kvm-need-to-vm-exit-on-cpuid)

## QEMU 给 kvm 设置 cpuid 的流程
KVM_SET_CPUID2 : 就是会修改 Guest 使用 cpuid 获取到能力。

- kvm_vcpu_ioctl_set_cpuid2
  - kvm_set_cpuid
    - `__kvm_update_cpuid_runtime`
    - kvm_vcpu_after_set_cpuid
      - vmx_vcpu_after_set_cpuid
        - vmx_update_exception_bitmap : 其实这个也不对啊

## 既然没有任何检查，是不是说明 kvm 的 filter 内容可以重新加回去?

## arch/x86/include/asm/cpufeatures.h

## QEMU 是如何调用的
关键描述:
- kvm_arch_get_supported_cpuid

其调用者为:
- x86_cpu_filter_features
- x86_cpu_get_supported_feature_word
- x86_cpu_get_supported_cpuid
- cpu_x86_cpuid

- kvm_request_xsave_components


## 为什么 Host 中没有看到，但是 Guest 中有看到
分析下，为什么 host 上不显示这个
1. arch/x86/kernel/cpu/capflags.c 直接就没有生成这个；
2. arch/x86/kernel/cpu/Makefile 中通过 arch/x86/kernel/cpu/mkcapflags.sh 来生成。

## 是不是 kvm 上也存在特殊的操作
- 应该是和那个东西没有什么关系吧

很奇怪，为什么

## cpuflags 对于性能的影响是什么？

## 如果 Host 中不提供，应该是硬件来控制的吧

## 分析下组织结构到底是怎么样子的

这是 Linux 定义的:
```c
enum cpuid_leafs
{
	CPUID_1_EDX		= 0,
	CPUID_8000_0001_EDX,
	CPUID_8086_0001_EDX,
	CPUID_LNX_1,
	CPUID_1_ECX,
	CPUID_C000_0001_EDX,
	CPUID_8000_0001_ECX,
	CPUID_LNX_2,
	CPUID_LNX_3,
	CPUID_7_0_EBX,
	CPUID_D_1_EAX,
	CPUID_LNX_4,
	CPUID_7_1_EAX,
	CPUID_8000_0008_EBX,
	CPUID_6_EAX,
	CPUID_8000_000A_EDX,
	CPUID_7_ECX,
	CPUID_8000_0007_EBX,
	CPUID_7_EDX,
	CPUID_8000_001F_EAX,
	CPUID_8000_0021_EAX,
};
```
- [ ]  CPUID_1_EDX 之类的这种是什么意思?

分析函数 `get_cpu_cap` 可以看的比较清楚:

或者分析
```c
static const struct cpuid_reg reverse_cpuid[] = {
	[CPUID_1_EDX]         = {         1, 0, CPUID_EDX},
	[CPUID_8000_0001_EDX] = {0x80000001, 0, CPUID_EDX},
	[CPUID_8086_0001_EDX] = {0x80860001, 0, CPUID_EDX},
	[CPUID_1_ECX]         = {         1, 0, CPUID_ECX},
	[CPUID_C000_0001_EDX] = {0xc0000001, 0, CPUID_EDX},
	[CPUID_8000_0001_ECX] = {0x80000001, 0, CPUID_ECX},
	[CPUID_7_0_EBX]       = {         7, 0, CPUID_EBX},
	[CPUID_D_1_EAX]       = {       0xd, 1, CPUID_EAX},
	[CPUID_8000_0008_EBX] = {0x80000008, 0, CPUID_EBX},
	[CPUID_6_EAX]         = {         6, 0, CPUID_EAX},
	[CPUID_8000_000A_EDX] = {0x8000000a, 0, CPUID_EDX},
	[CPUID_7_ECX]         = {         7, 0, CPUID_ECX},
	[CPUID_8000_0007_EBX] = {0x80000007, 0, CPUID_EBX},
	[CPUID_7_EDX]         = {         7, 0, CPUID_EDX},
	[CPUID_7_1_EAX]       = {         7, 1, CPUID_EAX},
	[CPUID_12_EAX]        = {0x00000012, 0, CPUID_EAX},
	[CPUID_8000_001F_EAX] = {0x8000001f, 0, CPUID_EAX},
	[CPUID_7_1_EDX]       = {         7, 1, CPUID_EDX},
	[CPUID_8000_0007_EDX] = {0x80000007, 0, CPUID_EDX},
	[CPUID_8000_0021_EAX] = {0x80000021, 0, CPUID_EAX},
};
```
所以，


这是 QEMU 定义的，两侧基本对应的
```c
/* CPUID feature words */
typedef enum FeatureWord {
    FEAT_1_EDX,         /* CPUID[1].EDX */
    FEAT_1_ECX,         /* CPUID[1].ECX */
    FEAT_7_0_EBX,       /* CPUID[EAX=7,ECX=0].EBX */
    FEAT_7_0_ECX,       /* CPUID[EAX=7,ECX=0].ECX */
    FEAT_7_0_EDX,       /* CPUID[EAX=7,ECX=0].EDX */
    FEAT_7_1_EAX,       /* CPUID[EAX=7,ECX=1].EAX */
    FEAT_8000_0001_EDX, /* CPUID[8000_0001].EDX */
    FEAT_8000_0001_ECX, /* CPUID[8000_0001].ECX */
    FEAT_8000_0007_EDX, /* CPUID[8000_0007].EDX */
    FEAT_8000_0008_EBX, /* CPUID[8000_0008].EBX */
    FEAT_C000_0001_EDX, /* CPUID[C000_0001].EDX */
    FEAT_KVM,           /* CPUID[4000_0001].EAX (KVM_CPUID_FEATURES) */
    FEAT_KVM_HINTS,     /* CPUID[4000_0001].EDX */
    FEAT_SVM,           /* CPUID[8000_000A].EDX */
    FEAT_XSAVE,         /* CPUID[EAX=0xd,ECX=1].EAX */
    FEAT_6_EAX,         /* CPUID[6].EAX */
    FEAT_XSAVE_XCR0_LO, /* CPUID[EAX=0xd,ECX=0].EAX */
    FEAT_XSAVE_XCR0_HI, /* CPUID[EAX=0xd,ECX=0].EDX */
    FEAT_ARCH_CAPABILITIES,
    FEAT_CORE_CAPABILITY,
    FEAT_PERF_CAPABILITIES,
    FEAT_VMX_PROCBASED_CTLS,
    FEAT_VMX_SECONDARY_CTLS,
    FEAT_VMX_PINBASED_CTLS,
    FEAT_VMX_EXIT_CTLS,
    FEAT_VMX_ENTRY_CTLS,
    FEAT_VMX_MISC,
    FEAT_VMX_EPT_VPID_CAPS,
    FEAT_VMX_BASIC,
    FEAT_VMX_VMFUNC,
    FEAT_14_0_ECX,
    FEAT_SGX_12_0_EAX,  /* CPUID[EAX=0x12,ECX=0].EAX (SGX) */
    FEAT_SGX_12_0_EBX,  /* CPUID[EAX=0x12,ECX=0].EBX (SGX MISCSELECT[31:0]) */
    FEAT_SGX_12_1_EAX,  /* CPUID[EAX=0x12,ECX=1].EAX (SGX ATTRIBUTES[31:0]) */
    FEAT_XSAVE_XSS_LO,     /* CPUID[EAX=0xd,ECX=1].ECX */
    FEAT_XSAVE_XSS_HI,     /* CPUID[EAX=0xd,ECX=1].EDX */
    FEATURE_WORDS,
} FeatureWord;
```
对比下 arch/x86/include/asm/cpufeatures.h 中的含义:
```c
#define X86_FEATURE_FPU			( 0*32+ 0) /* Onboard FPU */
```
这个具体的数值并没有任何硬件意义，在使用数组记录硬件数组而已:
```c
cpu_feature_enabled(X86_FEATURE_VME)
```
他们都是和 `cpuid_leafs` 中的定义是对应的:


### 整理思路
- 首先，存在一个 ./cpuid -d -c 1 的
```txt
CPUID 00000007:00 = 00000001 219c07ab 1840073c ac004410 | .......!<.@..D..
```
- cpu_x86_cpuid : index count
  - index 为 7 ，count 为 0 其中 ebx 为 219c07ab
- `kvm_arch_get_supported_cpuid` 为 function index


## [x] 为什么会存在 CPUID_LNX_4 ，是为了告诉用户态什么东西吗？

例如 X86_FEATURE_SPLIT_LOCK_DETECT
- 似乎很多都隐藏了，这是一个特殊点

- split_lock_setup
  - `__split_lock_setup`

有时候，其实就是像是全局变量一样，用于通知剩下的各个模块。

## [x] 简单分析一下 arch/x86/kvm/cpuid.c 到底说了什么?

1. 对于 QEMU 提供查询服务
QEMU 侧
- x86_cpu_expand_features
  - kvm_arch_get_supported_cpuid
    - get_supported_cpuid
      - try_get_cpuid
        - kvm_ioctl(s, KVM_GET_SUPPORTED_CPUID, cpuid);
    - 后面还存在一系列的 fixup

kernel 侧:
- kvm_dev_ioctl_get_cpuid : 使用命令 KVM_GET_SUPPORTED_CPUID 和 KVM_GET_EMULATED_CPUID
  - sanity_check_entries : 一些无关痛痒的检查
  - 初始化 kvm_cpuid_array
  - get_cpuid_func
    - do_cpuid_func
      - `__do_cpuid_func_emulated`
      - `__do_cpuid_func`
        - do_host_cpuid : 全部都是被 `__do_cpuid_func` 调用，
          - `get_next_cpuid`
        - kvm_cpu_cap_has : 访问数组 `kvm_cpu_caps`，检查一些比较特殊的 flags
        - cpuid_entry_override : 访问数组 `kvm_cpu_caps`，保证只有 kvm 支持的内容才会传递出去

- kvm_arch_vcpu_ioctl:
  - kvm_vcpu_ioctl_get_cpuid2 : 一个简单的拷贝，将 vcpu->arch.cpuid_entries 拷贝出来，QEMU 没有使用这个

2. QEMU 设置 cpuid

- kvm_arch_init_vcpu
  - 在 stack 中声明 cpuid_data
  - 通过调用 cpu_x86_cpuid 来获取，而 cpu_x86_cpuid 通过访问 CPUX86State::features


3. kvm 内查询
- kvm_find_cpuid_entry_index
  - cpuid_entry2_find

3. 初始化 : 直接从 QEMU 中拷贝过来的
- kvm_vcpu_ioctl_set_cpuid2
  - kvm_set_cpuid

4. kvm 初始化获取参数:
- vmx_set_cpu_caps
  - kvm_set_cpu_caps : 这里规定了 kvm 中总共可以使用那些 flags
    - `__kvm_cpu_cap_mask`
    - cpuid_count : 当然需要 host 中也提供才可以，但是只能访问两个

## 微调的位置
所以，现在一共存在三次微调
1. kvm 中 kvm_set_cpu_caps 的过滤的，表示是 kvm 支持的 cpu flags

QEMU:
1. kvm_arch_init_vcpu : 如果确定是 kvm，那么需要进行对应的调整
2. cpu_x86_cpuid : 因为是 QEMU 模拟的，很多特性需要进行修改

## 有的 cpuflags 是看不到的，例如 spec_ctrl
cpuid -1 可以检测到
```txt
      IBRS/IBPB: indirect branch restrictions  = true
```
但是 /proc/cpuinfo 不能
```c
#define X86_FEATURE_SPEC_CTRL		(18*32+26) /* "" Speculation Control (IBRS + IBPB) */
```
这个并不会导致，这个是一个判断失误。

## [ ] 如何理解 KVM_CPUID_FLAG_SIGNIFCANT_INDEX


## 当向 kvm 的 cpuid 的时候，kvm 是不做检查需要的 flags 的
- kvm_arch_init_vcpu 中
```c
        case 0x7:
        case 0x12:
            for (j = 0; ; j++) {
                c->function = i;
                c->flags = KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                c->index = j;
                cpu_x86_cpuid(env, i, j, &c->eax, &c->ebx, &c->ecx, &c->edx);
                // 去掉 avx2 的时候
                /* [huxueshi:kvm_arch_init_vcpu:2006] 1 219c078b 1840072c ac004410 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 810 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                printf("[huxueshi:%s:%d] %x %x %x %x\n", __FUNCTION__, __LINE__, c->eax, c->ebx, c->ecx, c->edx);

                // 增加 rtm 的 flag
                if (c->ebx == 0x219c07ab){
                    c->ebx = c->ebx | (1 << 11);
                }
                // 增加 avx2
                /* [huxueshi:kvm_arch_init_vcpu:2006] 1 219c07ab 1840072c ac004410 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 810 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [huxueshi:kvm_arch_init_vcpu:2006] 0 0 0 0 */

                if (j > 1 && (c->eax & 0xf) != 1) {
                    break;
                }

                if (cpuid_i == KVM_MAX_CPUID_ENTRIES) {
                    fprintf(stderr, "cpuid_data is full, no space for "
                                "cpuid(eax:0x12,ecx:0x%x)\n", j);
                    abort();
                }
                c = &cpuid_data.entries[cpuid_i++];
            }
            break;
```

```txt
[    0.743907] Run /init as init process
[    0.745631] traps: init[1] trap invalid opcode ip:7f480c569f49 sp:7ffc5c6e76b8 error:0 in libc.so.6[7f480c40a000+16f000]
[    0.746049] Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000004
[    0.746323] CPU: 0 PID: 1 Comm: init Not tainted 6.3.0-rc6-dirty #214
[    0.746554] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[    0.746794] Call Trace:
[    0.746900]  <TASK>
[    0.746976]  dump_stack_lvl+0x4b/0x80
[    0.747115]  panic+0x32b/0x350
[    0.747235]  do_exit+0x988/0xb30
[    0.747355]  do_group_exit+0x31/0x80
[    0.747486]  get_signal+0xa17/0xa40
[    0.747617]  ? __schedule+0x312/0x11b0
[    0.747756]  arch_do_signal_or_restart+0x2e/0x270
[    0.747930]  exit_to_user_mode_prepare+0xea/0x130
[    0.748105]  irqentry_exit_to_user_mode+0x9/0x20
[    0.748276]  asm_exc_invalid_op+0x1a/0x20
[    0.748426] RIP: 0033:0x7f480c569f49
[    0.748558] Code: 20 c5 dd 74 d9 c5 fd d7 ca c5 fd d7 c3 09 c1 74 90 85 c0 75 2c 85 d2 0f 84 84 00 00 00 89 d0 48 89 f7 0f bd c0 48 8d 44 07 e0 <0f
> 01 d6 74 04 c5 fc 77 c3 c5 f8 77 c3 66 2e 0f 1f 84 00 00 00 00
[    0.749213] RSP: 002b:00007ffc5c6e76b8 EFLAGS: 00010206
[    0.749393] RAX: 00007ffc5c6e7fc0 RBX: 00007ffc5c6e7758 RCX: 0000000080102020
[    0.749647] RDX: 00007ffc5c6e7770 RSI: 000000000000002f RDI: 00007ffc5c6e7fe0
[    0.749904] RBP: 00007ffc5c6e7fc0 R08: 000000000000003f R09: 00007f480c3e9c50
[    0.750159] R10: 00007ffc5c6e7300 R11: 0000000000000202 R12: 00007ffc5c6e7758
[    0.750414] R13: 00007ffc5c6e7770 R14: 00007f480c5ca5a0 R15: 0000000000000000
[    0.750668]  </TASK>
[    0.750778] Kernel Offset: disabled
[    0.750907] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000004 ]---
```
得到的结果如上。

可见，kvm 是对于 QEMU 设置何种 flags 是不做检查的。

## 那些 cpuid 是需要从 kvm 退出到 QEMU 来处理的
应该是不需要的

## 如何模拟 cache

在 `cpu_x86_cpuid` 中的:
```c
        /* cache info: needed for Pentium Pro compatibility */
        if (cpu->cache_info_passthrough) {
            x86_cpu_get_cache_cpuid(index, 0, eax, ebx, ecx, edx);
            break;
        } else if (cpu->vendor_cpuid_only && IS_AMD_CPU(env)) {
            *eax = *ebx = *ecx = *edx = 0;
            break;
        }
```

## [ ] kvm 会如何过滤 cpuid ?


## kvm 如何向 QEMU 返回系统中支持的 cpuid 的

其实，系统中，比较的应该是

```c
static __always_inline u32 kvm_cpu_cap_get(unsigned int x86_feature)
{
	unsigned int x86_leaf = __feature_leaf(x86_feature);

	reverse_cpuid_check(x86_leaf);
	return kvm_cpu_caps[x86_leaf] & __feature_bit(x86_feature);
}
```

## kvm_vcpu::arch::cpuid_entries 的组装和使用是怎么样的?
- 直接拷贝过来，然后让


## [ ] 如果是 -cpu host 的时候，那么还存在设置 KVM_SET_CPUID2 吗?
这个时候，应该是 QEMU 从 kvm 获取到什么就设置回去

## 到底是如何提供给 Guest 侧内容的

这是启动之后的结果，在系统安静的时候，都会产生多
```txt
@[
    kvm_emulate_cpuid+5
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 46766

@[
    emulator_get_cpuid+5
    vendor_intel+77
    x86_emulate_insn+1495
    x86_emulate_instruction+824
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 3
```

- kvm_emulate_cpuid
  - kvm_cpuid
    - kvm_find_cpuid_entry_index : 直接在数组中找，不去调用 cpuid 指令

### [ ] 为什么会存在这两个 backtrace ？

### [ ] 存在 `em_cpuid` 无人调用

## windows 的 CPUID 还是在这个体系下面的吗?

## [ ] 是如何修改 Guest 运行的状态时候的 CPUID 的，在硬件中?

## [ ] 是如何被过滤的?

## 思考一个问题
既然 model 如何难以定义，凭什么说，model 支持就可以传递过来。

是如何 check 一个 model 的?
