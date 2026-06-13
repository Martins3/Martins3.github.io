## kvm selftests

```txt
现在我已经完成了 KVM selftests 的分析和测试运行。以下是完整分析报告：

KVM Selftests 运行分析报告

1. 测试框架架构

┌─────────────────────────────────────────────────────────────────────┐
│                    KVM Selftests 架构                                │
├─────────────────────────────────────────────────────────────────────┤
│  测试层 (Test Layer)                                                │
│  ├── 通用测试: demand_paging_test, dirty_log_test, irqfd_test...    │
│  └── x86 特定: x86/cpuid_test, x86/vmx_*, x86/svm_*...              │
├─────────────────────────────────────────────────────────────────────┤
│  测试框架 (Test Framework)                                           │
│  ├── include/kvm_util.h        - 核心 KVM API 封装                   │
│  ├── include/kvm_test_harness.h - 测试用例宏定义                     │
│  └── lib/kvm_util.c            - VM/VCPU 管理实现                    │
├─────────────────────────────────────────────────────────────────────┤
│  架构抽象层 (Arch Abstraction)                                       │
│  ├── lib/x86/processor.c       - x86 CPU 相关操作                    │
│  ├── lib/x86/vmx.c             - Intel VMX 支持                      │
│  ├── lib/x86/svm.c             - AMD SVM 支持                        │
│  └── lib/arm64/, lib/s390/...  - 其他架构支持                        │
├─────────────────────────────────────────────────────────────────────┤
│  内核接口 (Kernel Interface)                                         │
│  └── /dev/kvm ioctl 接口封装                                         │
└─────────────────────────────────────────────────────────────────────┘

2. 测试分类

 类别                  测试数量   说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 通用测试 (所有架构)   ~20个      dirty_log_test, demand_paging_test, irqfd_test 等
 x86 特定测试          ~70个      VMX, SVM, MSR, CPUID, Hyper-V 等
 性能测试              ~5个       dirty_log_perf_test, memslot_perf_test 等
 压力测试              ~3个       mmu_stress_test, memslot_modification_stress_test

3. 构建系统

# 1. 准备内核头文件
cd /home/martins3/data/kernel/linux-build
make headers_install INSTALL_HDR_PATH=/home/martins3/data/kernel/linux-build/usr

# 2. 构建测试
cd tools/testing/selftests/kvm
make -j$(nproc)

# 3. 运行单个测试
./kvm_create_max_vcpus
./x86/cpuid_test

4. 核心 API 说明

// 创建 VM 和 VCPU
struct kvm_vm *vm_create_with_one_vcpu(struct kvm_vcpu **vcpu, void *guest_code);

// 运行 VCPU
void vcpu_run(struct kvm_vcpu *vcpu);

// 设置 VCPU 参数
void vcpu_args_set(struct kvm_vcpu *vcpu, unsigned int num, ...);

// Guest 代码中的同步/退出
GUEST_SYNC(stage);      // 同步点
GUEST_DONE();           // 正常退出
GUEST_ASSERT(cond);     // Guest 内断言

// 获取 ucall (Guest -> Host 通信)
enum ucall_cmd get_ucall(struct kvm_vcpu *vcpu, struct ucall *uc);

5. 当前环境测试结果

测试环境: x86_64, Intel CPU (vmx 支持), /dev/kvm 可用

==========================================
KVM Selftests 测试结果
==========================================

通用测试 (arch-independent):
  kvm_create_max_vcpus                     PASS
  kvm_binary_stats_test                    PASS
  guest_print_test                         PASS
  hardware_disable_test                    PASS
  system_counter_offset_test               PASS
  irqfd_test                               PASS
  coalesced_io_test                        PASS
  steal_time                               PASS

x86 特定测试:
  x86/cpuid_test                           PASS
  x86/cr4_cpuid_sync_test                  PASS
  x86/feature_msrs_test                    PASS
  x86/kvm_clock_test                       PASS
  x86/debug_regs                           PASS
  x86/xss_msr_test                         PASS
  x86/hwcr_msr_test                        PASS
  x86/set_boot_cpu_id                      PASS

长时间运行测试:
  dirty_log_test                           PASS
  set_memory_region_test                   PASS
  kvm_page_table_test                      PASS

==========================================
Results: PASS=19 FAIL=0
==========================================

6. 测试执行方式

# 方式1: 直接运行单个测试
cd /home/martins3/data/kernel/linux-build/tools/testing/selftests/kvm
./kvm_create_max_vcpus

# 方式2: 查看详细输出
./x86/sync_regs_test  # 输出 TAP 格式结果

# 方式3: 带参数运行
./demand_paging_test -m 9 -v 4  # 指定 guest mode 和 vcpu 数量

7. 关键设计特点

1. 双模执行模型: 测试代码包含 Host 部分 (main) 和 Guest 部分 (guest_main)，通过 vm_create_with_one_vcpu() 连接
2. UCALL 机制: Guest 通过 I/O 退出或特定指令与 Host 通信，传递测试状态
3. 随机种子: 每个测试使用固定随机种子 (0x6b8b4567) 保证可重复性
4. TAP 输出: 部分测试使用 Test Anything Protocol 格式输出结果，便于自动化处理

8. 限制与注意事项

• 需要 root 权限访问 /dev/kvm
• 某些测试需要特定内核配置或 CPU 特性 (如 VMX, SVM, SEV)
• 嵌套虚拟化测试需要 KVM 嵌套支持
• 部分测试 (如 SEV, AMX) 需要特定硬件支持

所有测试在当前环境均可正常运行，验证了 KVM selftests 构建和运行机制的完整性。
```

非常好，不需要任何特殊操作， 直接 make
```txt
cd /home/martins3/data/kernel/linux-build/tools/testing/selftests/kvm
make
./kvm_create_max_vcpus
```

从 scripts/clang-tools/gen_compile_commands.py 看，明确排除 tools ，所以，想看，就用 bear 吧

解决办法就是直接 bear -- make -j128

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
