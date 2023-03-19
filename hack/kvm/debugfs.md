## 了解下 dbugfs 的中记录
```txt
blocking
directed_yield_attempted
directed_yield_successful
exits
fpu_reload
guest_mode
halt_attempted_poll
halt_exits
halt_poll_fail_hist
halt_poll_fail_ns
halt_poll_invalid
halt_poll_success_hist
halt_poll_success_ns
halt_successful_poll
halt_wait_hist
halt_wait_ns
halt_wakeup
host_state_reload
hypercalls
insn_emulation
insn_emulation_fail
invlpg
io_exits
irq_exits
irq_injections
irq_window_exits
l1d_flush
max_mmu_page_hash_collisions
max_mmu_rmap_size
mmio_exits
mmu_cache_miss
mmu_flooded
mmu_pde_zapped
mmu_pte_write
mmu_recycled
mmu_shadow_zapped
mmu_unsync
nested_run
nmi_injections
nmi_window_exits
notify_window_exits
nx_lpage_splits
pages_1g
pages_2m
pages_4k
pf_emulate
pf_fast
pf_fixed
pf_guest
pf_mmio_spte_created
pf_spurious
pf_taken
preemption_other
preemption_reported
remote_tlb_flush
remote_tlb_flush_requests
req_event
request_irq_exits
signal_exits
tlb_flush
```
- [ ] 而且还存在一个函数 KVM_GET_STATS_FD

# kvm_stat

## 基本的使用


```txt
--guest=<guest_name>::
```
- guest name 是什么含义？

--log : 展示所有的
--skip-zero-records:: 如果该行中所有的都是 0，那么就输出

--debugfs
--debugfs-include-past :
--tracepoints
这两个是互斥的吗？

## 如何实现指定的 pid 的

- 通过 perf 的话，tracepoint ？

```py
    def _perf_event_open(self, attr, pid, cpu, group_fd, flags):
        """Wrapper for the sys_perf_evt_open() syscall.

        Used to set up performance events, returns a file descriptor or -1
        on error.

        Attributes are:
        - syscall number
        - struct perf_event_attr *
        - pid or -1 to monitor all pids
        - cpu number or -1 to monitor all cpus
        - The file descriptor of the group leader or -1 to create a group.
        - flags

        """
        return self.syscall(ARCH.sc_perf_evt_open, ctypes.pointer(attr),
                            ctypes.c_int(pid), ctypes.c_int(cpu),
                            ctypes.c_int(group_fd), ctypes.c_long(flags))
```
- 似乎是一共三个来源，应该是分别都可以设置 pid

## is child 是什么意思
tracepoint_is_child

*f*::	filter by regular expression
 ::     *Note*: Child events pull in their parents, and parents' stats summarize
                all child events, not just the filtered ones


*x*::	toggle reporting of stats for child trace events
 ::     *Note*: The stats for the parents summarize the respective child trace
                events


## 两个来源是相同的吗？
-f help 展示是哪里的来源:
```txt
sudo /home/martins3/kernel/centos-4.18.0-193.28.1-x86_64/tools/kvm/kvm_stat/kvm_stat -f help
  kvm_ack_irq
  kvm_age_hva
  kvm_apic
  kvm_apic_accept_irq
  kvm_apic_ipi
  kvm_apicv_accept_irq
  kvm_apicv_inhibit_changed
  kvm_async_pf_completed
  kvm_async_pf_not_present
  kvm_async_pf_ready
  kvm_async_pf_repeated_fault
  kvm_avic_doorbell
  kvm_avic_ga_log
  kvm_avic_incomplete_ipi
  kvm_avic_kick_vcpu_slowpath
  kvm_avic_unaccelerated_access
  kvm_cpuid
  kvm_cr
  kvm_dirty_ring_exit
  kvm_dirty_ring_push
  kvm_dirty_ring_reset
  kvm_emulate_insn
  kvm_entry
  kvm_eoi
  kvm_exit
  kvm_fast_mmio
  kvm_fpu
  kvm_halt_poll_ns
  kvm_hv_flush_tlb
  kvm_hv_flush_tlb_ex
  kvm_hv_hypercall
  kvm_hv_hypercall_done
  kvm_hv_notify_acked_sint
  kvm_hv_send_ipi
  kvm_hv_send_ipi_ex
  kvm_hv_stimer_callback
  kvm_hv_stimer_cleanup
  kvm_hv_stimer_expiration
  kvm_hv_stimer_set_config
  kvm_hv_stimer_set_count
  kvm_hv_stimer_start_one_shot
  kvm_hv_stimer_start_periodic
  kvm_hv_syndbg_get_msr
  kvm_hv_syndbg_set_msr
  kvm_hv_synic_send_eoi
  kvm_hv_synic_set_irq
  kvm_hv_synic_set_msr
  kvm_hv_timer_state
  kvm_hypercall
  kvm_inj_exception
  kvm_inj_virq
  kvm_invlpga
  kvm_ioapic_delayed_eoi_inj
  kvm_ioapic_set_irq
  kvm_mmio
  kvm_msi_set_irq
  kvm_msr
  kvm_nested_intercepts
  kvm_nested_intr_vmexit
  kvm_nested_vmenter
  kvm_nested_vmenter_failed
  kvm_nested_vmexit
  kvm_nested_vmexit_inject
  kvm_page_fault
  kvm_pi_irte_update
  kvm_pic_set_irq
  kvm_pio
  kvm_ple_window_update
  kvm_pml_full
  kvm_pv_eoi
  kvm_pv_tlb_flush
  kvm_pvclock_update
  kvm_set_irq
  kvm_set_spte_hva
  kvm_skinit
  kvm_smm_transition
  kvm_test_age_hva
  kvm_track_tsc
  kvm_try_async_get_page
  kvm_unmap_hva_range
  kvm_update_master_clock
  kvm_userspace_exit
  kvm_vcpu_wakeup
  kvm_vmgexit_enter
  kvm_vmgexit_exit
  kvm_vmgexit_msr_protocol_enter
  kvm_vmgexit_msr_protocol_exit
  kvm_wait_lapic_expire
  kvm_write_tsc_offset
  kvm_xen_hypercall
  vcpu_match_mmio
```

这里的一个基本案例上，表示来源可以多个
```txt
ExecStart=/usr/bin/kvm_stat -dtcz -s 10 -L /var/log/kvm_stat.csv
```

### 如果是从 tracepoints 获取，会导致和 kernel 版本关联吗?
