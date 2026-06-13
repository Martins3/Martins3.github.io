# vmx
## L0 的模拟 nested 和 L1 以及以上的版本模拟 nested 有区别吗?

并不是一样的，在 L1 中可以观测到非常频繁的 handle_vmread
```txt
sudo bpftrace -e "kprobe:handle_vmread { @[kstack] = count(); }"
```
但是在 L1 中完全观测不到:

此外，L3 虚拟机的出现，让 handle_vmread 的数量差别非常大:

```txt
🧀  sudo bpftrace -e "kprobe:handle_vmread { @[kstack] = count(); }"

Attaching 1 probe...
^C

@[
    handle_vmread+5
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 37375
linux on  master [+?] via 🐍 v3.11.8 via ❄️  impure (yyds-env) 13900k took 5s
🧀  sudo bpftrace -e "kprobe:handle_vmread { @[kstack] = count(); }"

Attaching 1 probe...
^C

@[
    handle_vmread+5
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 1333
```

## 如何处理更加高层次的 ept
```sh
sudo bpftrace -e "kprobe:kvm_tdp_page_fault { @[kstack] = count(); }"
```


```sh
sudo perf ftrace -G kvm_tdp_page_fault -g 'smp_*' -g irq_enter_rcu -g __sysvec_irq_work -g irq_exit_rcu | tee a
```

L1 的流程图:
```txt
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 3)               |  kvm_tdp_page_fault [kvm]() {
 3)   0.145 us    |    kvm_arch_has_noncoherent_dma [kvm]();
 3)   0.112 us    |    fast_page_fault [kvm]();
 3)               |    mmu_topup_memory_caches [kvm]() {
 3)               |      kvm_mmu_topup_memory_cache [kvm]() {
 3)   0.116 us    |        __kvm_mmu_topup_memory_cache [kvm]();
 3)   0.305 us    |      }
 3)               |      kvm_mmu_topup_memory_cache [kvm]() {
 3)   0.085 us    |        __kvm_mmu_topup_memory_cache [kvm]();
 3)   0.256 us    |      }
 3)               |      kvm_mmu_topup_memory_cache [kvm]() {
 3)   0.088 us    |        __kvm_mmu_topup_memory_cache [kvm]();
 3)   0.252 us    |      }
 3)   1.208 us    |    }
 3)               |    kvm_faultin_pfn [kvm]() {
 3)               |      __gfn_to_pfn_memslot [kvm]() {
 3)               |        hva_to_pfn [kvm]() {
 3)               |          get_user_pages_fast_only() {
 3)   0.101 us    |            is_valid_gup_args();
 3)               |            internal_get_user_pages_fast() {
 3)   0.116 us    |              pud_huge();
 3)   0.091 us    |              pmd_huge();
 3)               |              __pte_offset_map() {
 3)   0.090 us    |                __rcu_read_lock();
 3)   0.265 us    |              }
 3)   0.251 us    |              try_grab_folio();
 3)   0.091 us    |              folio_fast_pin_allowed();
 3)   0.096 us    |              __rcu_read_unlock();
 3)   2.649 us    |            }
 3)   3.061 us    |          }
 3)   3.313 us    |        }
 3)   3.528 us    |      }
 3)   3.795 us    |    }
 3)   0.115 us    |    _raw_read_lock();
 3)   0.335 us    |    is_page_fault_stale [kvm]();
 3)               |    kvm_tdp_mmu_map [kvm]() {
 3)               |      kvm_mmu_hugepage_adjust [kvm]() {
 3)   0.596 us    |        kvm_mmu_max_mapping_level [kvm]();
 3)   1.284 us    |      }
 3)   0.090 us    |      __rcu_read_lock();
 3)               |      tdp_iter_start [kvm]() {
 3)   0.306 us    |        tdp_iter_restart [kvm]();
 3)   0.533 us    |      }
 3)   0.143 us    |      tdp_iter_next [kvm]();
 3)   0.344 us    |      tdp_iter_next [kvm]();
 3)   0.181 us    |      tdp_iter_next [kvm]();
 3)               |      make_spte [kvm]() {
 3)   0.314 us    |        kvm_is_mmio_pfn [kvm]();
 3)               |        vmx_get_mt_mask [kvm_intel]() {
 3)   0.094 us    |          kvm_arch_has_noncoherent_dma [kvm]();
 3)   0.289 us    |        }
 3)               |        mmu_try_to_unsync_pages [kvm]() {
 3)   0.245 us    |          kvm_gfn_is_write_tracked [kvm]();
 3)   0.555 us    |        }
 3)   1.700 us    |      }
 3)   0.243 us    |      handle_changed_spte [kvm]();
 3)   0.109 us    |      __rcu_read_unlock();
 3) + 11.138 us   |    }
 3)   0.106 us    |    _raw_read_unlock();
 3)               |    kvm_release_pfn_clean [kvm]() {
 3)   0.109 us    |      kvm_pfn_to_refcounted_page [kvm]();
 3)               |      kvm_release_page_clean [kvm]() {
 3)               |        mark_page_accessed() {
 3)   0.098 us    |          folio_mark_accessed();
 3)   0.280 us    |        }
 3)   0.522 us    |      }
 3)   0.937 us    |    }
 3) + 24.222 us   |  }
 ------------------------------------------
 3)  qemu-sy-6792  =>  qemu-sy-6789
 ------------------------------------------

 3)               |  kvm_tdp_page_fault [kvm]() {
 3)   0.173 us    |    kvm_arch_has_noncoherent_dma [kvm]();
 3)   0.118 us    |    fast_page_fault [kvm]();
 3)               |    mmu_topup_memory_caches [kvm]() {
 3)               |      kvm_mmu_topup_memory_cache [kvm]() {
 3)   0.116 us    |        __kvm_mmu_topup_memory_cache [kvm]();
 3)   0.355 us    |      }
 3)               |      kvm_mmu_topup_memory_cache [kvm]() {
 3)   0.085 us    |        __kvm_mmu_topup_memory_cache [kvm]();
 3)   0.249 us    |      }
 3)               |      kvm_mmu_topup_memory_cache [kvm]() {
 3)   0.090 us    |        __kvm_mmu_topup_memory_cache [kvm]();
 3)   0.252 us    |      }
 3)   1.254 us    |    }
 3)               |    kvm_faultin_pfn [kvm]() {
 3)               |      __gfn_to_pfn_memslot [kvm]() {
 3)               |        hva_to_pfn [kvm]() {
 3)               |          get_user_pages_fast_only() {
 3)   0.098 us    |            is_valid_gup_args();
 3)               |            internal_get_user_pages_fast() {
 3)   0.145 us    |              pud_huge();
 3)   0.096 us    |              pmd_huge();
 3)               |              __pte_offset_map() {
 3)   0.089 us    |                __rcu_read_lock();
 3)   0.260 us    |              }
 3)   0.368 us    |              try_grab_folio();
 3)   0.091 us    |              folio_fast_pin_allowed();
 3)   0.103 us    |              __rcu_read_unlock();
 3)   3.795 us    |            }
 3)   4.229 us    |          }
 3)   4.485 us    |        }
 3)   4.660 us    |      }
 3)   5.065 us    |    }
 3)   0.148 us    |    _raw_read_lock();
 3)   0.433 us    |    is_page_fault_stale [kvm]();
 3)               |    kvm_tdp_mmu_map [kvm]() {
 3)               |      kvm_mmu_hugepage_adjust [kvm]() {
 3)   0.466 us    |        kvm_mmu_max_mapping_level [kvm]();
 3)   0.716 us    |      }
 3)   0.091 us    |      __rcu_read_lock();
 3)               |      tdp_iter_start [kvm]() {
 3)   0.366 us    |        tdp_iter_restart [kvm]();
 3)   0.588 us    |      }
 3)   0.154 us    |      tdp_iter_next [kvm]();
 3)   0.226 us    |      tdp_iter_next [kvm]();
 3)   0.201 us    |      tdp_iter_next [kvm]();
 3)               |      make_spte [kvm]() {
 3)   0.315 us    |        kvm_is_mmio_pfn [kvm]();
 3)               |        vmx_get_mt_mask [kvm_intel]() {
 3)   0.090 us    |          kvm_arch_has_noncoherent_dma [kvm]();
 3)   0.270 us    |        }
 3)               |        mmu_try_to_unsync_pages [kvm]() {
 3)   0.296 us    |          kvm_gfn_is_write_tracked [kvm]();
 3)   0.693 us    |        }
 3)   1.858 us    |      }
 3)   0.302 us    |      handle_changed_spte [kvm]();
 3)   0.121 us    |      __rcu_read_unlock();
 3)   9.057 us    |    }
 3)   0.110 us    |    _raw_read_unlock();
 3)               |    kvm_release_pfn_clean [kvm]() {
 3)   0.105 us    |      kvm_pfn_to_refcounted_page [kvm]();
 3)               |      kvm_release_page_clean [kvm]() {
 3)               |        mark_page_accessed() {
 3)   0.094 us    |          folio_mark_accessed();
 3)   0.326 us    |        }
 3)   0.564 us    |      }
 3)   0.963 us    |    }
 3) + 25.122 us   |  }
```

```plain
 0)               |    kvm_tdp_mmu_map [kvm]() {
 0)               |      kvm_mmu_hugepage_adjust [kvm]() {
 0)   0.630 us    |        kvm_mmu_max_mapping_level [kvm]();
 0)   0.795 us    |      }
 0)   0.088 us    |      __rcu_read_lock();
 0)               |      tdp_iter_start [kvm]() {
 0)   0.099 us    |        tdp_iter_restart [kvm]();
 0)   0.258 us    |      }
 0)   0.103 us    |      tdp_iter_next [kvm]();
 0)   0.096 us    |      kvm_mmu_memory_cache_alloc [kvm]();
 0)   0.082 us    |      kvm_mmu_memory_cache_alloc [kvm]();
 0)               |      tdp_mmu_init_child_sp [kvm]() {
 0)   0.083 us    |        tdp_mmu_init_sp [kvm]();
 0)   0.233 us    |      }
 0)               |      tdp_mmu_link_sp [kvm]() {
 0)   0.090 us    |        make_nonleaf_spte [kvm]();
 0)   0.113 us    |        handle_changed_spte [kvm]();
 0)               |        __mod_lruvec_page_state() {
 0)   0.089 us    |          __rcu_read_lock();
 0)               |          __mod_lruvec_state() {
 0)   0.086 us    |            __mod_node_page_state();
 0)               |            __mod_memcg_lruvec_state() {
 0)   0.103 us    |              cgroup_rstat_updated();
 0)   0.286 us    |            }
 0)   0.612 us    |          }
 0)   0.097 us    |          __rcu_read_unlock();
 0)   1.182 us    |        }
 0)   3.059 us    |      }
 0)   0.167 us    |      tdp_iter_next [kvm]();
 0)   0.084 us    |      kvm_mmu_memory_cache_alloc [kvm]();
 0)   0.085 us    |      kvm_mmu_memory_cache_alloc [kvm]();
 0)               |      tdp_mmu_init_child_sp [kvm]() {
 0)   0.084 us    |        tdp_mmu_init_sp [kvm]();
 0)   0.233 us    |      }
 0)               |      tdp_mmu_link_sp [kvm]() {
 0)   0.086 us    |        make_nonleaf_spte [kvm]();
 0)   0.109 us    |        handle_changed_spte [kvm]();
 0)               |        __mod_lruvec_page_state() {
 0)   0.083 us    |          __rcu_read_lock();
 0)               |          __mod_lruvec_state() {
 0)   0.084 us    |            __mod_node_page_state();
 0)               |            __mod_memcg_lruvec_state() {
 0)   0.083 us    |              cgroup_rstat_updated();
 0)   0.241 us    |            }
 0)   0.554 us    |          }
 0)   0.086 us    |          __rcu_read_unlock();
 0)   1.025 us    |        }
 0)   2.329 us    |      }
 0)   0.112 us    |      tdp_iter_next [kvm]();
 0)               |      make_spte [kvm]() {
 0)   0.104 us    |        kvm_is_mmio_pfn [kvm]();
 0)               |        vmx_get_mt_mask [kvm_intel]() {
 0)   0.094 us    |          kvm_arch_has_noncoherent_dma [kvm]();
 0)   0.250 us    |        }
 0)   0.609 us    |      }
 0)   0.146 us    |      handle_changed_spte [kvm]();
 0)   0.105 us    |      __rcu_read_unlock();
 0) + 11.053 us   |    }
 0)   0.090 us    |    _raw_read_unlock();
 0)               |    kvm_release_pfn_clean [kvm]() {
 0)   0.088 us    |      kvm_pfn_to_refcounted_page [kvm]();
 0)               |      kvm_release_page_clean [kvm]() {
 0)               |        mark_page_accessed() {
 0)   0.087 us    |          folio_mark_accessed();
 0)   0.249 us    |        }
 0)   0.408 us    |      }
 0)   0.727 us    |    }
 0) + 19.535 us   |  }

```

L0 的流程图
```txt
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
  2)               |  kvm_tdp_page_fault [kvm]() {
  2)   0.122 us    |    kvm_arch_has_noncoherent_dma [kvm]();
  2)   0.225 us    |    kvm_gfn_is_write_tracked [kvm]();
  2)               |    fast_page_fault [kvm]() {
  2)               |      walk_shadow_page_lockless_begin [kvm]() {
  2)   0.062 us    |        __rcu_read_lock();
  2)   0.207 us    |      }
  2)               |      kvm_tdp_mmu_fast_pf_get_last_sptep [kvm]() {
  2)               |        tdp_iter_start [kvm]() {
  2)   0.101 us    |          tdp_iter_restart [kvm]();
  2)   0.353 us    |        }
  2)   0.214 us    |        tdp_iter_next [kvm]();
  2)   0.146 us    |        tdp_iter_next [kvm]();
  2)   0.141 us    |        tdp_iter_next [kvm]();
  2)   0.063 us    |        tdp_iter_next [kvm]();
  2)   1.338 us    |      }
  2)               |      walk_shadow_page_lockless_end [kvm]() {
  2)   0.068 us    |        __rcu_read_unlock();
  2)   0.189 us    |      }
  2)   2.871 us    |    }
  2)               |    mmu_topup_memory_caches [kvm]() {
  2)               |      kvm_mmu_topup_memory_cache [kvm]() {
  2)   0.071 us    |        __kvm_mmu_topup_memory_cache [kvm]();
  2)   0.686 us    |      }
  2)               |      kvm_mmu_topup_memory_cache [kvm]() {
  2)   0.058 us    |        __kvm_mmu_topup_memory_cache [kvm]();
  2)   0.166 us    |      }
  2)               |      kvm_mmu_topup_memory_cache [kvm]() {
  2)   0.057 us    |        __kvm_mmu_topup_memory_cache [kvm]();
  2)   0.165 us    |      }
  2)   1.353 us    |    }
  2)               |    kvm_faultin_pfn [kvm]() {
  2)               |      __gfn_to_pfn_memslot [kvm]() {
  2)               |        hva_to_pfn [kvm]() {
  2)               |          get_user_pages_fast_only() {
  2)   0.072 us    |            is_valid_gup_args();
  2)               |            internal_get_user_pages_fast() {
  2)   0.069 us    |              pud_huge();
  2)   0.233 us    |              try_grab_folio();
  2)   0.634 us    |            }
  2)   0.878 us    |          }
  2)   1.011 us    |        }
  2)   1.145 us    |      }
  2)   1.298 us    |    }
  2)   0.095 us    |    _raw_read_lock();
  2)   0.080 us    |    is_page_fault_stale [kvm]();
  2)               |    kvm_tdp_mmu_map [kvm]() {
  2)               |      kvm_mmu_hugepage_adjust [kvm]() {
  2)   0.204 us    |        __kvm_mmu_max_mapping_level [kvm]();
  2)   0.349 us    |      }
  2)   0.057 us    |      __rcu_read_lock();
  2)               |      tdp_iter_start [kvm]() {
  2)   0.059 us    |        tdp_iter_restart [kvm]();
  2)   0.165 us    |      }
  2)   0.096 us    |      tdp_iter_next [kvm]();
  2)   0.986 us    |      tdp_iter_next [kvm]();
  2)   0.081 us    |      tdp_iter_next [kvm]();
  2)               |      make_spte [kvm]() {
  2)   0.130 us    |        kvm_is_mmio_pfn [kvm]();
  2)               |        vmx_get_mt_mask [kvm_intel]() {
  2)   0.067 us    |          kvm_arch_has_noncoherent_dma [kvm]();
  2)   0.197 us    |        }
  2)               |        mmu_try_to_unsync_pages [kvm]() {
  2)   0.066 us    |          kvm_gfn_is_write_tracked [kvm]();
  2)   0.285 us    |        }
  2)   0.999 us    |      }
  2)   0.756 us    |      handle_changed_spte [kvm]();
  2)   0.067 us    |      __rcu_read_unlock();
  2)   4.573 us    |    }
  2)   0.068 us    |    _raw_read_unlock();
  2)               |    kvm_release_pfn_clean [kvm]() {
  2)   0.110 us    |      kvm_pfn_to_refcounted_page [kvm]();
  2)               |      kvm_release_page_clean [kvm]() {
  2)               |        mark_page_accessed() {
  2)   0.080 us    |          folio_mark_accessed();
  2)   0.190 us    |        }
  2)   0.349 us    |      }
  2)   0.643 us    |    }
  2) + 16.619 us   |  }
```

## intel 在虚拟机也是支持 dirty log 的，有趣哇 nested_vmx_write_pml_buffer


## 观察下会存在这些区别吗?

```c
void kvm_init_mmu(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu_role_regs regs = vcpu_to_role_regs(vcpu);
	union kvm_cpu_role cpu_role = kvm_calc_cpu_role(vcpu, &regs);

	if (mmu_is_nested(vcpu))
		init_kvm_nested_mmu(vcpu, cpu_role);
	else if (tdp_enabled)
		init_kvm_tdp_mmu(vcpu, cpu_role);
	else
		init_kvm_softmmu(vcpu, cpu_role);
}
```
在 L1 中启动 L2 后，并没有观察到 kvm_init_mmu 中走到 init_kvm_nested_mmu ，真刺激

## 有趣的 backtrace
```txt
@[
    vmx_switch_vmcs+1
    nested_vmx_vmexit+317
    nested_ept_inject_page_fault+76
    ept_page_fault+482
    kvm_mmu_page_fault+313
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 266
@[
    vmx_switch_vmcs+1
    nested_vmx_vmexit+317
    nested_vmx_reflect_vmexit+741
    vmx_handle_exit+116
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 6815
@[
    vmx_switch_vmcs+1
    nested_vmx_enter_non_root_mode+375
    nested_vmx_run+291
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 7235
```

## 万万没有想到，难道是硬件支持无限


## sync_vmcs02_to_vmcs12

## 看看这两个注释

```c
	/*
	 * Newly recognized interrupts are injected via either virtual interrupt
	 * delivery (RVI) or KVM_REQ_EVENT.  Virtual interrupt delivery is
	 * disabled in two cases:
	 *
	 * 1) If L2 is running and the vCPU has a new pending interrupt.  If L1
	 * wants to exit on interrupts, KVM_REQ_EVENT is needed to synthesize a
	 * VM-Exit to L1.  If L1 doesn't want to exit, the interrupt is injected
	 * into L2, but KVM doesn't use virtual interrupt delivery to inject
	 * interrupts into L2, and so KVM_REQ_EVENT is again needed.
	 *
	 * 2) If APICv is disabled for this vCPU, assigned devices may still
	 * attempt to post interrupts.  The posted interrupt vector will cause
	 * a VM-Exit and the subsequent entry will call sync_pir_to_irr.
	 */
	if (!is_guest_mode(vcpu) && kvm_vcpu_apicv_active(vcpu))
		vmx_set_rvi(max_irr);
	else if (got_posted_interrupt)
		kvm_make_request(KVM_REQ_EVENT, vcpu);
```

```c
void vmx_hwapic_irr_update(struct kvm_vcpu *vcpu, int max_irr)
{
	/*
	 * When running L2, updating RVI is only relevant when
	 * vmcs12 virtual-interrupt-delivery enabled.
	 * However, it can be enabled only when L1 also
	 * intercepts external-interrupts and in that case
	 * we should not update vmcs02 RVI but instead intercept
	 * interrupt. Therefore, do nothing when running L2.
	 */
	if (!is_guest_mode(vcpu))
		vmx_set_rvi(max_irr);
}
```

## 看看这个函数
vmx_deliver_nested_posted_interrupt
还有
vmx_has_nested_events

嵌套还有 posted interupt，真的太可怕了

## 理解一下这个注释
vmx_update_exception_bitmap
```c
	/* When we are running a nested L2 guest and L1 specified for it a
	 * certain exception bitmap, we must trap the same exceptions and pass
	 * them to L1. When running L2, we will only handle the exceptions
	 * specified above if L1 did not want them.
	 */
	if (is_guest_mode(vcpu))
		eb |= get_vmcs12(vcpu)->exception_bitmap;
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
