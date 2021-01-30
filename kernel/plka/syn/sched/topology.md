# topology
- [ ] build_balance_mask

- sched_domain
  - span
  - sched_domain_topology_level
- sched_group
- root_domain
  - init_rootdomain

- cpu_attach_domain

- degenerate

- rq_attach_root

# cpumask
- start_kernel
  - setup_arch
      - smp_init_cpus :
        - of_parse_and_init_cpus
        - acpi_parse_and_init_cpus
        - smp_cpu_setup
          - set_cpu_possible
            - **cpumask_set_cpu(cpu, &__cpu_possible_mask);**
  - arch_call_rest_init
    - rest_init
      - kernel_init
        - kernel_init_freeable
          - smp_prepare_cpus
            - set_cpu_present : 如果这只是将 possible 拷贝到 present，其意义何在 ?
              - **cpumask_set_cpu(cpu, &__cpu_present_mask);**
          - smp_init
            - bringup_nonboot_cpus
              - cpu_up : 实际上，跟丢了

SMT : L1 高速共享
MC  : 共享 LLC
SOC : DIE

```
config SCHED_SMT
	bool "SMT (Hyperthreading) scheduler support"
	depends on SPARC64 && SMP
	default y
	help
	  SMT scheduler support improves the CPU scheduler's decision making
	  when dealing with SPARC cpus at a cost of slightly increased overhead
	  in some places. If unsure say N here.

config SCHED_MC
	bool "Multi-core scheduler support"
	depends on SPARC64 && SMP
	default y
	help
	  Multi-core scheduler support improves the CPU scheduler's decision
	  making when dealing with multi-core CPU chips at a cost of slightly
	  increased overhead in some places. If unsure say N here.
```

```c
/*
 * Topology list, bottom-up.
 */
static struct sched_domain_topology_level default_topology[] = {
#ifdef CONFIG_SCHED_SMT
	{ cpu_smt_mask, cpu_smt_flags, SD_INIT_NAME(SMT) },
#endif
#ifdef CONFIG_SCHED_MC
	{ cpu_coregroup_mask, cpu_core_flags, SD_INIT_NAME(MC) },
#endif
	{ cpu_cpu_mask, SD_INIT_NAME(DIE) },
	{ NULL, },
};

typedef const struct cpumask *(*sched_domain_mask_f)(int cpu);
typedef int (*sched_domain_flags_f)(void);

struct sched_domain_topology_level {
	sched_domain_mask_f mask;      // 返回某个 cpu 在该 topology level 下的 CPU 的兄弟 cpu 的 mask
	sched_domain_flags_f sd_flags; // 用于返回 domain 的属性
	int		    flags;
	int		    numa_level;
	struct sd_data      data;
};

struct sd_data {
	struct sched_domain *__percpu *sd; // 优秀啊，每一个 cpu 都保存一份所有人的 sched_domain
	struct sched_domain_shared *__percpu *sds;
	struct sched_group *__percpu *sg;
	struct sched_group_capacity *__percpu *sgc;
};
```

- 在 sched_domain 被划分为 sched_group, sched_group 是调度最小单位。
  - [ ] 感觉这么定义的话，岂不是下一级的 sched_domain 就上级的 sched_group
- sched_domain_span 表示 cpu 当前的 domain 管辖的 cpu 范围


- sched_init_domains
  - build_sched_domains
  - `__visit_domain_allocation_hell`
    - `__sdt_alloc`
    - alloc_rootdomain
  - build_sched_domain
    - sd_init
  - build_sched_groups

- 构建 domain 的结果是，在每一个 topology level 中间都存在 NR_cpu 个 sched_domain
  - 这个 sched_domain 包含一定数量的 cpu
  - sched_domain 指向一个链表的 sched_group
