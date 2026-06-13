## code overview

```txt
󰊕  sys_reg_to_index (几个辅助函数)
󰊕  set_id_reg
󰊕  undef_access
󰊕  bad_trap
󰊕  read_from_write_only
󰊕  write_to_read_only
  sr_loc_attr
󰆼  sr_loc
󰊕  locate_direct_register
󰊕  locate_mapped_el2_register
  locate_register
󰊕  read_sr_from_cpu
󰊕  write_sr_to_cpu
  vcpu_read_sys_reg
  vcpu_write_sys_reg
  get_min_cache_line_size (全部都是 sys_reg_desc 中定义的特殊处理的项目了)
  get_ccsidr
  set_ccsidr
󰊕  access_rw
󰊕  access_dcsw
󰊕  access_dcgsw
  get_access_mask
󰊕  access_vm_reg
󰊕  access_actlr
󰊕  access_gic_sgi
󰊕  access_gic_sre
󰊕  trap_raz_wi
  trap_loregion
󰊕  trap_oslar_el1
  trap_oslsr_el1
  set_oslsr_el1
󰊕  trap_dbgauthstatus_el1
󰊕  trap_debug_regs
󰊕  reg_to_dbg
󰊕  dbg_to_reg
󰊕  demux_wb_reg
󰊕  trap_dbg_wb_reg
󰊕  set_dbg_wb_reg
󰊕  get_dbg_wb_reg
󰊕  reset_dbg_wb_reg
󰊕  reset_amair_el1
󰊕  reset_actlr
󰊕  reset_mpidr
󰊕  hidden_visibility
󰊕  pmu_visibility
  reset_pmu_reg
  reset_pmevcntr
  reset_pmevtyper
  reset_pmselr
  reset_pmcr
  check_pmu_access_disabled
󰊕  pmu_access_el0_disabled
󰊕  pmu_write_swinc_el0_disabled
󰊕  pmu_access_cycle_counter_el0_disabled
󰊕  pmu_access_event_counter_el0_disabled
󰊕  access_pmcr
  access_pmselr
󰊕  access_pmceid
  pmu_counter_idx_valid
󰊕  get_pmu_evcntr
󰊕  set_pmu_evcntr
  access_pmu_evcntr
  access_pmu_evtyper
  set_pmreg
  get_pmreg
  access_pmcnten
  access_pminten
  access_pmovs
󰊕  access_pmswinc
  access_pmuserenr
󰊕  get_pmcr
  set_pmcr
󰊕  ptrauth_visibility
󰊕  access_arch_timer
  arch_timer_set_user
  arch_timer_get_user
󰊕  kvm_arm64_ftr_safe_value
󰊕  arm64_check_features
󰊕  pmuver_to_perfmon
󰊕  sanitise_id_aa64pfr0_el1
󰊕  sanitise_id_aa64pfr1_el1
󰊕  sanitise_id_aa64dfr0_el1
  __kvm_read_sanitised_id_reg
󰊕  kvm_read_sanitised_id_reg
󰊕  read_id_reg
󰊕  is_feature_id_reg
󰊕  is_vm_ftr_id_reg
󰊕  is_vcpu_ftr_id_reg
󰊕  is_aa32_id_reg
󰊕  id_visibility
󰊕  aa32_id_visibility
󰊕  raz_visibility
󰊕  access_id_reg
󰊕  sve_visibility
  sme_visibility
  fp8_visibility
  sanitise_id_aa64pfr0_el1
  sanitise_id_aa64pfr1_el1
  sanitise_id_aa64dfr0_el1
  ignore_feat_doublelock
  set_id_aa64dfr0_el1
  read_sanitised_id_dfr0_el1
  set_id_dfr0_el1
  set_id_aa64pfr0_el1
  set_id_aa64pfr1_el1
  set_id_aa64mmfr0_el1
  set_id_aa64mmfr2_el1
  set_ctr_el0
󰊕  get_id_reg
󰊕  set_id_reg
󰊕  kvm_set_vm_id_reg
󰊕  get_raz_reg
󰊕  set_wi_reg
󰊕  access_ctr
  access_clidr
  reset_clidr
  set_clidr
󰊕  access_csselr
  access_ccsidr
󰊕  mte_visibility
󰊕  el2_visibility
󰊕  bad_vncr_trap
󰊕  bad_redir_trap
  access_sp_el1
󰊕  access_elr
  access_spsr
  access_cntkctl_el12
  reset_hcr
󰊕  __el2_visibility
󰊕  sve_el2_visibility
  vncr_el2_visibility
  sctlr2_visibility
󰊕  sctlr2_el2_visibility
  access_zcr_el2
󰊕  access_gic_vtr
󰊕  access_gic_misr
󰊕  access_gic_eisr
󰊕  access_gic_elrsr
  s1poe_visibility
󰊕  s1poe_el2_visibility
  tcr2_visibility
󰊕  tcr2_el2_visibility
  fgt2_visibility
  fgt_visibility
  s1pie_visibility
󰊕  s1pie_el2_visibility
󰊕  cnthv_visibility
  access_mdcr
  access_ras
󰊕  access_imp_id_reg
󰊕  init_imp_id_regs
󰊕  reset_imp_id_reg
󰊕  set_imp_id_reg
  reset_mdcr
󰊕  handle_at_s1e01
  handle_at_s1e2
󰊕  handle_at_s12
  kvm_supported_tlbi_s12_op
󰊕  handle_alle1is
  kvm_supported_tlbi_ipas2_op
  tlbi_info
󰊕  s2_mmu_unmap_range
󰊕  handle_vmalls12e1is
󰊕  handle_ripas2e1is
  s2_mmu_unmap_ipa
󰊕  handle_ipas2e1is
󰊕  s2_mmu_tlbi_s1e1
󰊕  handle_tlbi_el2
  handle_tlbi_el1
  trap_dbgdidr
󰊕  check_sysreg_table
󰊕  kvm_handle_cp14_load_store
󰊕  perform_access
󰊕  emulate_cp
󰊕  unhandled_cp_access
󰊕  kvm_handle_cp_64
󰊕  emulate_sys_reg
󰊕  kvm_esr_cp10_id_to_sys64
󰊕  kvm_handle_cp10_id
󰊕  kvm_emulate_cp15_id_reg
󰊕  kvm_handle_cp_32	( coprocess 支持)
  kvm_handle_cp15_64
  kvm_handle_cp15_32
  kvm_handle_cp14_64
  kvm_handle_cp14_32
  emulate_sys_reg
  idregs_debug_find (debugfs 支持)
󰊕  idregs_debug_start
󰊕  idregs_debug_next
󰊕  idregs_debug_stop
󰊕  idregs_debug_show
󰊕  idregs_debug_open
󰊕  kvm_sys_regs_create_debugfs
󰊕  reset_vm_ftr_id_reg
  reset_vcpu_ftr_id_reg
  kvm_reset_sys_regs
󰊕  kvm_handle_sys_reg
󰊕  index_to_params (和用户态进行交互的部分)
󰊕  get_reg_by_id
󰊕  id_to_sys_reg_desc
  demux_c15_get
  demux_c15_set
󰊕  kvm_one_reg_to_id
  kvm_sys_reg_get_user
  kvm_arm_sys_reg_get_reg
  kvm_sys_reg_set_user
  kvm_arm_sys_reg_set_reg
󰊕  num_demux_regs
  write_demux_regids
󰊕  sys_reg_to_index
  copy_reg_to_user
󰊕  walk_one_sys_reg
  walk_sys_regs
󰊕  kvm_arm_num_sys_reg_descs
󰊕  kvm_arm_copy_sys_reg_indices
  kvm_vm_ioctl_get_reg_writable_masks
  vcpu_set_hcr
󰊕  kvm_calculate_traps
  kvm_finalize_sys_regs
  kvm_sys_reg_table_init
```
## debugfs

```txt
localhost# cat idregs
        SYS_MIDR_EL1:   00000000481fd010
      SYS_REVIDR_EL1:   0000000000000000
     SYS_ID_PFR0_EL1:   0000000000000000
     SYS_ID_PFR1_EL1:   0000000000000000
     SYS_ID_DFR0_EL1:   0000000004000000
     SYS_ID_AFR0_EL1:   0000000000000000
    SYS_ID_MMFR0_EL1:   0000000000000000
    SYS_ID_MMFR1_EL1:   0000000000000000
    SYS_ID_MMFR2_EL1:   0000000000000000
    SYS_ID_MMFR3_EL1:   0000000000000000
    SYS_ID_ISAR0_EL1:   0000000000000000
    SYS_ID_ISAR1_EL1:   0000000000000000
    SYS_ID_ISAR2_EL1:   0000000000000000
    SYS_ID_ISAR3_EL1:   0000000000000000
    SYS_ID_ISAR4_EL1:   0000000000000000
    SYS_ID_ISAR5_EL1:   0000000000000000
    SYS_ID_MMFR4_EL1:   0000000000000000
    SYS_ID_ISAR6_EL1:   0000000000000000
       SYS_MVFR0_EL1:   0000000000000000
       SYS_MVFR1_EL1:   0000000000000000
       SYS_MVFR2_EL1:   0000000000000000
          S3_0_0_3_3:   0000000000000000
     SYS_ID_PFR2_EL1:   0000000000000000
     SYS_ID_DFR1_EL1:   0000000000000000
    SYS_ID_MMFR5_EL1:   0000000000000000
          S3_0_0_3_7:   0000000000000000
 SYS_ID_AA64PFR0_EL1:   1100000011111111
 SYS_ID_AA64PFR1_EL1:   0000000000000000
 SYS_ID_AA64PFR2_EL1:   0000000000000000
          S3_0_0_4_3:   0000000000000000
 SYS_ID_AA64ZFR0_EL1:   0000000000000000
SYS_ID_AA64SMFR0_EL1:   0000000000000000
          S3_0_0_4_6:   0000000000000000
SYS_ID_AA64FPFR0_EL1:   0000000000000000
 SYS_ID_AA64DFR0_EL1:   0000000010305408
 SYS_ID_AA64DFR1_EL1:   0000000000000000
          S3_0_0_5_2:   0000000000000000
          S3_0_0_5_3:   0000000000000000
 SYS_ID_AA64AFR0_EL1:   0000000000000000
 SYS_ID_AA64AFR1_EL1:   0000000000000000
          S3_0_0_5_6:   0000000000000000
          S3_0_0_5_7:   0000000000000000
SYS_ID_AA64ISAR0_EL1:   0001100010211120
SYS_ID_AA64ISAR1_EL1:   0000000000011001
SYS_ID_AA64ISAR2_EL1:   0000000000000000
SYS_ID_AA64ISAR3_EL1:   0000000000000000
          S3_0_0_6_4:   0000000000000000
          S3_0_0_6_5:   0000000000000000
          S3_0_0_6_6:   0000000000000000
          S3_0_0_6_7:   0000000000000000
SYS_ID_AA64MMFR0_EL1:   0000000000101125
SYS_ID_AA64MMFR1_EL1:   0000000010211122
SYS_ID_AA64MMFR2_EL1:   0000000000001011
SYS_ID_AA64MMFR3_EL1:   0000000000000000
SYS_ID_AA64MMFR4_EL1:   0000000000000000
          S3_0_0_7_5:   0000000000000000
          S3_0_0_7_6:   0000000000000000
          S3_0_0_7_7:   0000000000000000
        SYS_AIDR_EL1:   0000000000000551
         SYS_CTR_EL0:   0000000084448004
```

```c
static const struct seq_operations idregs_debug_sops = {
	.start	= idregs_debug_start,
	.next	= idregs_debug_next,
	.stop	= idregs_debug_stop,
	.show	= idregs_debug_show,
};
```

只有部分 id 被挑出来了:
```c
/*
 * Return true if the register's (Op0, Op1, CRn, CRm, Op2) is
 * (3, 0, 0, crm, op2), where 1<=crm<8, 0<=op2<8, which is the range of ID
 * registers KVM maintains on a per-VM basis.
 *
 * Additionally, the implementation ID registers and CTR_EL0 are handled as
 * per-VM registers.
 */
static inline bool is_vm_ftr_id_reg(u32 id)
{
	switch (id) {
	case SYS_CTR_EL0:
	case SYS_MIDR_EL1:
	case SYS_REVIDR_EL1:
	case SYS_AIDR_EL1:
		return true;
	default:
		return (sys_reg_Op0(id) == 3 && sys_reg_Op1(id) == 0 &&
			sys_reg_CRn(id) == 0 && sys_reg_CRm(id) >= 1 &&
			sys_reg_CRm(id) < 8);

	}
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
