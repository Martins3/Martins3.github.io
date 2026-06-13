## SVM_EXIT_TASK_SWITCH 和 EXIT_REASON_TASK_SWITCH

哈哈，居然是为了模拟 x86 架构上的 task switch ，唉，浪费时间。

- handle_task_switch
  - kvm_task_switch
    - emulator_task_switch
      - emulator_do_task_switch

## x86 kvm 代码统计
```txt
=======================================================================================
 Language                    Files        Lines         Code     Comments       Blanks
=======================================================================================
 C                              39        76220        52127        12328        11765
---------------------------------------------------------------------------------------
 ./x86.c                                  13990         9984         1903         2103
 ./vmx/vmx.c                               8701         5885         1498         1318
 ./mmu/mmu.c                               7508         4657         1601         1250
 ./vmx/nested.c                            7173         4604         1577          992
 ./emulate.c                               5504         4279          468          757
 ./svm/svm.c                               5442         3658          845          939
 ./svm/sev.c                               3422         2398          435          589
 ./lapic.c                                 3350         2351          435          564
 ./hyperv.c                                2916         2164          311          441
 ./xen.c                                   2304         1580          377          347
 ./mmu/tdp_mmu.c                           1828          944          585          299
 ./svm/nested.c                            1820         1225          265          330
 ./cpuid.c                                 1630         1100          307          223
 ./svm/avic.c                              1221          757          265          199
 ./pmu.c                                   1043          679          193          171
 ./ioapic.c                                 776          554          112          110
 ./i8254.c                                  751          557           84          110
 ./vmx/pmu_intel.c                          736          524          112          100
 ./mtrr.c                                   720          479          113          128
 ./i8259.c                                  660          503           79           78
 ./smm.c                                    640          481           39          120
 ./vmx/sgx.c                                510          333          104           73
 ./mmu/spte.c                               503          278          142           83
 ./irq_comm.c                               449          361           14           74
 ./mmu/page_track.c                         371          224           77           70
 ./vmx/posted_intr.c                        353          181          119           53
 ./vmx/hyperv_evmcs.c                       315          260           46            9
 ./svm/pmu.c                                243          184           25           34
 ./vmx/hyperv.c                             229          165           26           38
 ./debugfs.c                                196          148           12           36
 ./mmu/tdp_iter.c                           177           89           62           26
 ./irq.c                                    173           98           48           27
 ./vmx/main.c                               167          134            1           32
```


## 手册
- [AMD64 Architecture Programmer's Manual, Volume 1](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf)
- [AMD64 Architecture Programmer's Manual, Volume 2](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24592.pdf)

## 速查手册

### AMD vmcb

arch/x86/include/asm/svm.h
```c
struct __attribute__ ((__packed__)) vmcb_control_area {
  u32 intercepts[MAX_INTERCEPT];
  u32 reserved_1[15 - MAX_INTERCEPT];
  u16 pause_filter_thresh;
  u16 pause_filter_count;
  u64 iopm_base_pa;
  u64 msrpm_base_pa;
  u64 tsc_offset;
  u32 asid;
  u8 tlb_ctl;
  u8 reserved_2[3];
  u32 int_ctl;
  u32 int_vector;
  u32 int_state;
  u8 reserved_3[4];
  u32 exit_code;
  u32 exit_code_hi;
  u64 exit_info_1;
  u64 exit_info_2;
  u32 exit_int_info;
  u32 exit_int_info_err;
  u64 nested_ctl;
  u64 avic_vapic_bar;
  u64 ghcb_gpa;
  u32 event_inj;
  u32 event_inj_err;
  u64 nested_cr3;
  u64 virt_ext;
  u32 clean;
  u32 reserved_5;
  u64 next_rip;
  u8 insn_len;
  u8 insn_bytes[15];
  u64 avic_backing_page;  /* Offset 0xe0 */
  u8 reserved_6[8]; /* Offset 0xe8 */
  u64 avic_logical_id;  /* Offset 0xf0 */
  u64 avic_physical_id; /* Offset 0xf8 */
  u8 reserved_7[8];
  u64 vmsa_pa;    /* Used for an SEV-ES guest */
  u8 reserved_8[720];
  /*
   * Offset 0x3e0, 32 bytes reserved
   * for use by hypervisor/software.
   */
  union {
    struct hv_vmcb_enlightenments hv_enlightenments;
    u8 reserved_sw[32];
  };
};
```

### Voulume 2 : Appendix B VMCB Layout, Control Area

![](./img/vmcb/a.png)
![](./img/vmcb/b.png)
![](./img/vmcb/c.png)
![](./img/vmcb/d.png)
![](./img/vmcb/e.png)
![](./img/vmcb/f.png)
![](./img/vmcb/g.png)

### Voulume 2 : Appendix C SVM Intercept Exit Codes, State Save Area

![](./img/vmcb/1.png)
![](./img/vmcb/2.png)
![](./img/vmcb/3.png)
![](./img/vmcb/4.png)

### SVM Intercept Exit Codes

![](./img/svm_exit/1.png)
![](./img/svm_exit/2.png)
![](./img/svm_exit/3.png)

### AMD feature 相关的代码

arch/x86/include/asm/cpufeatures.h
```c
/* AMD SVM Feature Identification, CPUID level 0x8000000a (EDX), word 15 */
#define X86_FEATURE_NPT     (15*32+ 0) /* Nested Page Table support */
#define X86_FEATURE_LBRV    (15*32+ 1) /* LBR Virtualization support */
#define X86_FEATURE_SVML    (15*32+ 2) /* "svm_lock" SVM locking MSR */
#define X86_FEATURE_NRIPS   (15*32+ 3) /* "nrip_save" SVM next_rip save */
#define X86_FEATURE_TSCRATEMSR    (15*32+ 4) /* "tsc_scale" TSC scaling support */
#define X86_FEATURE_VMCBCLEAN   (15*32+ 5) /* "vmcb_clean" VMCB clean bits support */
#define X86_FEATURE_FLUSHBYASID   (15*32+ 6) /* flush-by-ASID support */
#define X86_FEATURE_DECODEASSISTS (15*32+ 7) /* Decode Assists support */
#define X86_FEATURE_PAUSEFILTER   (15*32+10) /* filtered pause intercept */
#define X86_FEATURE_PFTHRESHOLD   (15*32+12) /* pause filter threshold */
#define X86_FEATURE_AVIC    (15*32+13) /* Virtual Interrupt Controller */
#define X86_FEATURE_V_VMSAVE_VMLOAD (15*32+15) /* Virtual VMSAVE VMLOAD */
#define X86_FEATURE_VGIF    (15*32+16) /* Virtual GIF */
#define X86_FEATURE_X2AVIC    (15*32+18) /* Virtual x2apic */
#define X86_FEATURE_V_SPEC_CTRL   (15*32+20) /* Virtual SPEC_CTRL */
#define X86_FEATURE_VNMI    (15*32+25) /* Virtual NMI */
#define X86_FEATURE_SVME_ADDR_CHK (15*32+28) /* "" SVME addr check */
```

## Intel VMCS

arch/x86/include/asm/vmx.h
```c
/* VMCS Encodings */
enum vmcs_field {
  VIRTUAL_PROCESSOR_ID            = 0x00000000,
  POSTED_INTR_NV                  = 0x00000002,
  LAST_PID_POINTER_INDEX    = 0x00000008,
  GUEST_ES_SELECTOR               = 0x00000800,
  GUEST_CS_SELECTOR               = 0x00000802,
  GUEST_SS_SELECTOR               = 0x00000804,
  GUEST_DS_SELECTOR               = 0x00000806,
  GUEST_FS_SELECTOR               = 0x00000808,
  GUEST_GS_SELECTOR               = 0x0000080a,
  GUEST_LDTR_SELECTOR             = 0x0000080c,
  GUEST_TR_SELECTOR               = 0x0000080e,
  GUEST_INTR_STATUS               = 0x00000810,
  GUEST_PML_INDEX     = 0x00000812,
  HOST_ES_SELECTOR                = 0x00000c00,
  HOST_CS_SELECTOR                = 0x00000c02,
  HOST_SS_SELECTOR                = 0x00000c04,
  HOST_DS_SELECTOR                = 0x00000c06,
  HOST_FS_SELECTOR                = 0x00000c08,
  HOST_GS_SELECTOR                = 0x00000c0a,
  HOST_TR_SELECTOR                = 0x00000c0c,
  IO_BITMAP_A                     = 0x00002000,
  IO_BITMAP_A_HIGH                = 0x00002001,
  IO_BITMAP_B                     = 0x00002002,
  IO_BITMAP_B_HIGH                = 0x00002003,
  MSR_BITMAP                      = 0x00002004,
  MSR_BITMAP_HIGH                 = 0x00002005,
  VM_EXIT_MSR_STORE_ADDR          = 0x00002006,
  VM_EXIT_MSR_STORE_ADDR_HIGH     = 0x00002007,
  VM_EXIT_MSR_LOAD_ADDR           = 0x00002008,
  VM_EXIT_MSR_LOAD_ADDR_HIGH      = 0x00002009,
  VM_ENTRY_MSR_LOAD_ADDR          = 0x0000200a,
  VM_ENTRY_MSR_LOAD_ADDR_HIGH     = 0x0000200b,
  PML_ADDRESS                     = 0x0000200e,
  PML_ADDRESS_HIGH                = 0x0000200f,
  TSC_OFFSET                      = 0x00002010,
  TSC_OFFSET_HIGH                 = 0x00002011,
  VIRTUAL_APIC_PAGE_ADDR          = 0x00002012,
  VIRTUAL_APIC_PAGE_ADDR_HIGH     = 0x00002013,
  APIC_ACCESS_ADDR                = 0x00002014,
  APIC_ACCESS_ADDR_HIGH           = 0x00002015,
  POSTED_INTR_DESC_ADDR           = 0x00002016,
  POSTED_INTR_DESC_ADDR_HIGH      = 0x00002017,
  VM_FUNCTION_CONTROL             = 0x00002018,
  VM_FUNCTION_CONTROL_HIGH        = 0x00002019,
  EPT_POINTER                     = 0x0000201a,
  EPT_POINTER_HIGH                = 0x0000201b,
  EOI_EXIT_BITMAP0                = 0x0000201c,
  EOI_EXIT_BITMAP0_HIGH           = 0x0000201d,
  EOI_EXIT_BITMAP1                = 0x0000201e,
  EOI_EXIT_BITMAP1_HIGH           = 0x0000201f,
  EOI_EXIT_BITMAP2                = 0x00002020,
  EOI_EXIT_BITMAP2_HIGH           = 0x00002021,
  EOI_EXIT_BITMAP3                = 0x00002022,
  EOI_EXIT_BITMAP3_HIGH           = 0x00002023,
  EPTP_LIST_ADDRESS               = 0x00002024,
  EPTP_LIST_ADDRESS_HIGH          = 0x00002025,
  VMREAD_BITMAP                   = 0x00002026,
  VMREAD_BITMAP_HIGH              = 0x00002027,
  VMWRITE_BITMAP                  = 0x00002028,
  VMWRITE_BITMAP_HIGH             = 0x00002029,
  XSS_EXIT_BITMAP                 = 0x0000202C,
  XSS_EXIT_BITMAP_HIGH            = 0x0000202D,
  ENCLS_EXITING_BITMAP    = 0x0000202E,
  ENCLS_EXITING_BITMAP_HIGH = 0x0000202F,
  TSC_MULTIPLIER                  = 0x00002032,
  TSC_MULTIPLIER_HIGH             = 0x00002033,
  TERTIARY_VM_EXEC_CONTROL  = 0x00002034,
  TERTIARY_VM_EXEC_CONTROL_HIGH = 0x00002035,
  PID_POINTER_TABLE   = 0x00002042,
  PID_POINTER_TABLE_HIGH    = 0x00002043,
  GUEST_PHYSICAL_ADDRESS          = 0x00002400,
  GUEST_PHYSICAL_ADDRESS_HIGH     = 0x00002401,
  VMCS_LINK_POINTER               = 0x00002800,
  VMCS_LINK_POINTER_HIGH          = 0x00002801,
  GUEST_IA32_DEBUGCTL             = 0x00002802,
  GUEST_IA32_DEBUGCTL_HIGH        = 0x00002803,
  GUEST_IA32_PAT      = 0x00002804,
  GUEST_IA32_PAT_HIGH   = 0x00002805,
  GUEST_IA32_EFER     = 0x00002806,
  GUEST_IA32_EFER_HIGH    = 0x00002807,
  GUEST_IA32_PERF_GLOBAL_CTRL = 0x00002808,
  GUEST_IA32_PERF_GLOBAL_CTRL_HIGH= 0x00002809,
  GUEST_PDPTR0                    = 0x0000280a,
  GUEST_PDPTR0_HIGH               = 0x0000280b,
  GUEST_PDPTR1                    = 0x0000280c,
  GUEST_PDPTR1_HIGH               = 0x0000280d,
  GUEST_PDPTR2                    = 0x0000280e,
  GUEST_PDPTR2_HIGH               = 0x0000280f,
  GUEST_PDPTR3                    = 0x00002810,
  GUEST_PDPTR3_HIGH               = 0x00002811,
  GUEST_BNDCFGS                   = 0x00002812,
  GUEST_BNDCFGS_HIGH              = 0x00002813,
  GUEST_IA32_RTIT_CTL   = 0x00002814,
  GUEST_IA32_RTIT_CTL_HIGH  = 0x00002815,
  HOST_IA32_PAT     = 0x00002c00,
  HOST_IA32_PAT_HIGH    = 0x00002c01,
  HOST_IA32_EFER      = 0x00002c02,
  HOST_IA32_EFER_HIGH   = 0x00002c03,
  HOST_IA32_PERF_GLOBAL_CTRL  = 0x00002c04,
  HOST_IA32_PERF_GLOBAL_CTRL_HIGH = 0x00002c05,
  PIN_BASED_VM_EXEC_CONTROL       = 0x00004000,
  CPU_BASED_VM_EXEC_CONTROL       = 0x00004002,
  EXCEPTION_BITMAP                = 0x00004004,
  PAGE_FAULT_ERROR_CODE_MASK      = 0x00004006,
  PAGE_FAULT_ERROR_CODE_MATCH     = 0x00004008,
  CR3_TARGET_COUNT                = 0x0000400a,
  VM_EXIT_CONTROLS                = 0x0000400c,
  VM_EXIT_MSR_STORE_COUNT         = 0x0000400e,
  VM_EXIT_MSR_LOAD_COUNT          = 0x00004010,
  VM_ENTRY_CONTROLS               = 0x00004012,
  VM_ENTRY_MSR_LOAD_COUNT         = 0x00004014,
  VM_ENTRY_INTR_INFO_FIELD        = 0x00004016,
  VM_ENTRY_EXCEPTION_ERROR_CODE   = 0x00004018,
  VM_ENTRY_INSTRUCTION_LEN        = 0x0000401a,
  TPR_THRESHOLD                   = 0x0000401c,
  SECONDARY_VM_EXEC_CONTROL       = 0x0000401e,
  PLE_GAP                         = 0x00004020,
  PLE_WINDOW                      = 0x00004022,
  NOTIFY_WINDOW                   = 0x00004024,
  VM_INSTRUCTION_ERROR            = 0x00004400,
  VM_EXIT_REASON                  = 0x00004402,
  VM_EXIT_INTR_INFO               = 0x00004404,
  VM_EXIT_INTR_ERROR_CODE         = 0x00004406,
  IDT_VECTORING_INFO_FIELD        = 0x00004408,
  IDT_VECTORING_ERROR_CODE        = 0x0000440a,
  VM_EXIT_INSTRUCTION_LEN         = 0x0000440c,
  VMX_INSTRUCTION_INFO            = 0x0000440e,
  GUEST_ES_LIMIT                  = 0x00004800,
  GUEST_CS_LIMIT                  = 0x00004802,
  GUEST_SS_LIMIT                  = 0x00004804,
  GUEST_DS_LIMIT                  = 0x00004806,
  GUEST_FS_LIMIT                  = 0x00004808,
  GUEST_GS_LIMIT                  = 0x0000480a,
  GUEST_LDTR_LIMIT                = 0x0000480c,
  GUEST_TR_LIMIT                  = 0x0000480e,
  GUEST_GDTR_LIMIT                = 0x00004810,
  GUEST_IDTR_LIMIT                = 0x00004812,
  GUEST_ES_AR_BYTES               = 0x00004814,
  GUEST_CS_AR_BYTES               = 0x00004816,
  GUEST_SS_AR_BYTES               = 0x00004818,
  GUEST_DS_AR_BYTES               = 0x0000481a,
  GUEST_FS_AR_BYTES               = 0x0000481c,
  GUEST_GS_AR_BYTES               = 0x0000481e,
  GUEST_LDTR_AR_BYTES             = 0x00004820,
  GUEST_TR_AR_BYTES               = 0x00004822,
  GUEST_INTERRUPTIBILITY_INFO     = 0x00004824,
  GUEST_ACTIVITY_STATE            = 0x00004826,
  GUEST_SYSENTER_CS               = 0x0000482A,
  VMX_PREEMPTION_TIMER_VALUE      = 0x0000482E,
  HOST_IA32_SYSENTER_CS           = 0x00004c00,
  CR0_GUEST_HOST_MASK             = 0x00006000,
  CR4_GUEST_HOST_MASK             = 0x00006002,
  CR0_READ_SHADOW                 = 0x00006004,
  CR4_READ_SHADOW                 = 0x00006006,
  CR3_TARGET_VALUE0               = 0x00006008,
  CR3_TARGET_VALUE1               = 0x0000600a,
  CR3_TARGET_VALUE2               = 0x0000600c,
  CR3_TARGET_VALUE3               = 0x0000600e,
  EXIT_QUALIFICATION              = 0x00006400,
  GUEST_LINEAR_ADDRESS            = 0x0000640a,
  GUEST_CR0                       = 0x00006800,
  GUEST_CR3                       = 0x00006802,
  GUEST_CR4                       = 0x00006804,
  GUEST_ES_BASE                   = 0x00006806,
  GUEST_CS_BASE                   = 0x00006808,
  GUEST_SS_BASE                   = 0x0000680a,
  GUEST_DS_BASE                   = 0x0000680c,
  GUEST_FS_BASE                   = 0x0000680e,
  GUEST_GS_BASE                   = 0x00006810,
  GUEST_LDTR_BASE                 = 0x00006812,
  GUEST_TR_BASE                   = 0x00006814,
  GUEST_GDTR_BASE                 = 0x00006816,
  GUEST_IDTR_BASE                 = 0x00006818,
  GUEST_DR7                       = 0x0000681a,
  GUEST_RSP                       = 0x0000681c,
  GUEST_RIP                       = 0x0000681e,
  GUEST_RFLAGS                    = 0x00006820,
  GUEST_PENDING_DBG_EXCEPTIONS    = 0x00006822,
  GUEST_SYSENTER_ESP              = 0x00006824,
  GUEST_SYSENTER_EIP              = 0x00006826,
  HOST_CR0                        = 0x00006c00,
  HOST_CR3                        = 0x00006c02,
  HOST_CR4                        = 0x00006c04,
  HOST_FS_BASE                    = 0x00006c06,
  HOST_GS_BASE                    = 0x00006c08,
  HOST_TR_BASE                    = 0x00006c0a,
  HOST_GDTR_BASE                  = 0x00006c0c,
  HOST_IDTR_BASE                  = 0x00006c0e,
  HOST_IA32_SYSENTER_ESP          = 0x00006c10,
  HOST_IA32_SYSENTER_EIP          = 0x00006c12,
  HOST_RSP                        = 0x00006c14,
  HOST_RIP                        = 0x00006c16,
};
```
文档在 : **25 VIRTUAL MACHINE CONTROL STRUCTURES**

表格在 : **APPENDIX B FIELD ENCODING IN VMCS**

### GUEST_PDPTR1

### TPR_THRESHOLD

参考 27.7.7 VM Exits Induced by the TPR Threshold

## struct vmcs12 的作用是什么
<!-- c6498df0-6879-41b2-85c5-ef3a2936254a -->

正如之前理解的那样，vmcs12 就是 L1 提供给 L2 的 vmcs 。
注意，这个东西可以由 kvm 定义的，因为 vmcs12 不能被 L1 直接访问，必须通过
VMWRITE 之类的指令才可以访问，而 VMWRITE 之类的指令一定会导致 vmexit ，然后存放
在 struct vmcs12 中就可以了。

所以也很容易理解为什么热迁移需要迁移 vmcs12 ，这个也算是 L1 虚拟机的状态了。

```c
/*
 * struct vmcs12 describes the state that our guest hypervisor (L1) keeps for a
 * single nested guest (L2), hence the name vmcs12. Any VMX implementation has
 * a VMCS structure, and vmcs12 is our emulated VMX's VMCS. This structure is
 * stored in guest memory specified by VMPTRLD, but is opaque to the guest,
 * which must access it using VMREAD/VMWRITE/VMCLEAR instructions.
 * More than one of these structures may exist, if L1 runs multiple L2 guests.
 * nested_vmx_run() will use the data here to build the vmcs02: a VMCS for the
 * underlying hardware which will be used to run L2.
 * This structure is packed to ensure that its layout is identical across
 * machines (necessary for live migration).
 *
 * IMPORTANT: Changing the layout of existing fields in this structure
 * will break save/restore compatibility with older kvm releases. When
 * adding new fields, either use space in the reserved padding* arrays
 * or add the new fields to the end of the structure.
 */
typedef u64 natural_width;
struct __packed vmcs12 {
  /* According to the Intel spec, a VMCS region must start with the
   * following two fields. Then follow implementation-specific data.
   */
  struct vmcs_hdr hdr;
  u32 abort;

  u32 launch_state; /* set to 0 by VMCLEAR, to 1 by VMLAUNCH */
  u32 padding[7]; /* room for future expansion */

  u64 io_bitmap_a;
  u64 io_bitmap_b;
  u64 msr_bitmap;
  u64 vm_exit_msr_store_addr;
  u64 vm_exit_msr_load_addr;
  u64 vm_entry_msr_load_addr;
  u64 tsc_offset;
  u64 virtual_apic_page_addr;
  u64 apic_access_addr;
  u64 posted_intr_desc_addr;
  u64 ept_pointer;
  u64 eoi_exit_bitmap0;
  u64 eoi_exit_bitmap1;
  u64 eoi_exit_bitmap2;
  u64 eoi_exit_bitmap3;
  u64 xss_exit_bitmap;
  u64 guest_physical_address;
  u64 vmcs_link_pointer;
  u64 guest_ia32_debugctl;
  u64 guest_ia32_pat;
  u64 guest_ia32_efer;
  u64 guest_ia32_perf_global_ctrl;
  u64 guest_pdptr0;
  u64 guest_pdptr1;
  u64 guest_pdptr2;
  u64 guest_pdptr3;
  u64 guest_bndcfgs;
  u64 host_ia32_pat;
  u64 host_ia32_efer;
  u64 host_ia32_perf_global_ctrl;
  u64 vmread_bitmap;
  u64 vmwrite_bitmap;
  u64 vm_function_control;
  u64 eptp_list_address;
  u64 pml_address;
  u64 encls_exiting_bitmap;
  u64 tsc_multiplier;
  u64 padding64[1]; /* room for future expansion */
  /*
   * To allow migration of L1 (complete with its L2 guests) between
   * machines of different natural widths (32 or 64 bit), we cannot have
   * unsigned long fields with no explicit size. We use u64 (aliased
   * natural_width) instead. Luckily, x86 is little-endian.
   */
  natural_width cr0_guest_host_mask;
  natural_width cr4_guest_host_mask;
  natural_width cr0_read_shadow;
  natural_width cr4_read_shadow;
  natural_width dead_space[4]; /* Last remnants of cr3_target_value[0-3]. */
  natural_width exit_qualification;
  natural_width guest_linear_address;
  natural_width guest_cr0;
  natural_width guest_cr3;
  natural_width guest_cr4;
  natural_width guest_es_base;
  natural_width guest_cs_base;
  natural_width guest_ss_base;
  natural_width guest_ds_base;
  natural_width guest_fs_base;
  natural_width guest_gs_base;
  natural_width guest_ldtr_base;
  natural_width guest_tr_base;
  natural_width guest_gdtr_base;
  natural_width guest_idtr_base;
  natural_width guest_dr7;
  natural_width guest_rsp;
  natural_width guest_rip;
  natural_width guest_rflags;
  natural_width guest_pending_dbg_exceptions;
  natural_width guest_sysenter_esp;
  natural_width guest_sysenter_eip;
  natural_width host_cr0;
  natural_width host_cr3;
  natural_width host_cr4;
  natural_width host_fs_base;
  natural_width host_gs_base;
  natural_width host_tr_base;
  natural_width host_gdtr_base;
  natural_width host_idtr_base;
  natural_width host_ia32_sysenter_esp;
  natural_width host_ia32_sysenter_eip;
  natural_width host_rsp;
  natural_width host_rip;
  natural_width paddingl[8]; /* room for future expansion */
  u32 pin_based_vm_exec_control;
  u32 cpu_based_vm_exec_control;
  u32 exception_bitmap;
  u32 page_fault_error_code_mask;
  u32 page_fault_error_code_match;
  u32 cr3_target_count;
  u32 vm_exit_controls;
  u32 vm_exit_msr_store_count;
  u32 vm_exit_msr_load_count;
  u32 vm_entry_controls;
  u32 vm_entry_msr_load_count;
  u32 vm_entry_intr_info_field;
  u32 vm_entry_exception_error_code;
  u32 vm_entry_instruction_len;
  u32 tpr_threshold;
  u32 secondary_vm_exec_control;
  u32 vm_instruction_error;
  u32 vm_exit_reason;
  u32 vm_exit_intr_info;
  u32 vm_exit_intr_error_code;
  u32 idt_vectoring_info_field;
  u32 idt_vectoring_error_code;
  u32 vm_exit_instruction_len;
  u32 vmx_instruction_info;
  u32 guest_es_limit;
  u32 guest_cs_limit;
  u32 guest_ss_limit;
  u32 guest_ds_limit;
  u32 guest_fs_limit;
  u32 guest_gs_limit;
  u32 guest_ldtr_limit;
  u32 guest_tr_limit;
  u32 guest_gdtr_limit;
  u32 guest_idtr_limit;
  u32 guest_es_ar_bytes;
  u32 guest_cs_ar_bytes;
  u32 guest_ss_ar_bytes;
  u32 guest_ds_ar_bytes;
  u32 guest_fs_ar_bytes;
  u32 guest_gs_ar_bytes;
  u32 guest_ldtr_ar_bytes;
  u32 guest_tr_ar_bytes;
  u32 guest_interruptibility_info;
  u32 guest_activity_state;
  u32 guest_sysenter_cs;
  u32 host_ia32_sysenter_cs;
  u32 vmx_preemption_timer_value;
  u32 padding32[7]; /* room for future expansion */
  u16 virtual_processor_id;
  u16 posted_intr_nv;
  u16 guest_es_selector;
  u16 guest_cs_selector;
  u16 guest_ss_selector;
  u16 guest_ds_selector;
  u16 guest_fs_selector;
  u16 guest_gs_selector;
  u16 guest_ldtr_selector;
  u16 guest_tr_selector;
  u16 guest_intr_status;
  u16 host_es_selector;
  u16 host_cs_selector;
  u16 host_ss_selector;
  u16 host_ds_selector;
  u16 host_fs_selector;
  u16 host_gs_selector;
  u16 host_tr_selector;
  u16 guest_pml_index;
};
```

KVM 并不是让 L1 直接按偏移访问 vmcs12，而是通过 field encoding 查 offset，再读写结构体字段，见：

所以 vmcs12 本质上是“用软件 struct 存 Intel VMCS field”。

```c
static inline short get_vmcs12_field_offset(unsigned long field)
{
	unsigned short offset;
	unsigned int index;

	if (field >> 15)
		return -ENOENT;

	index = ROL16(field, 6);
	if (index >= nr_vmcs12_fields)
		return -ENOENT;

	index = array_index_nospec(index, nr_vmcs12_fields);
	offset = vmcs12_field_offsets[index];
	if (offset == 0)
		return -ENOENT;
	return offset;
}
```

```c
const unsigned short vmcs12_field_offsets[] = {
	FIELD(VIRTUAL_PROCESSOR_ID, virtual_processor_id),
```


## Intel Exit Reason
<!-- a0a44a42-63da-43b7-b0e0-f195a435e9b6 -->

表格在 : **APPENDIX C FIELD ENCODING IN VMCS**

```c
#define VMX_EXIT_REASONS_FAILED_VMENTRY         0x80000000
#define VMX_EXIT_REASONS_SGX_ENCLAVE_MODE 0x08000000

#define EXIT_REASON_EXCEPTION_NMI       0
#define EXIT_REASON_EXTERNAL_INTERRUPT  1
#define EXIT_REASON_TRIPLE_FAULT        2
#define EXIT_REASON_INIT_SIGNAL     3
#define EXIT_REASON_SIPI_SIGNAL         4

#define EXIT_REASON_INTERRUPT_WINDOW    7
#define EXIT_REASON_NMI_WINDOW          8
#define EXIT_REASON_TASK_SWITCH         9
#define EXIT_REASON_CPUID               10
#define EXIT_REASON_HLT                 12
#define EXIT_REASON_INVD                13
#define EXIT_REASON_INVLPG              14
#define EXIT_REASON_RDPMC               15
#define EXIT_REASON_RDTSC               16
#define EXIT_REASON_VMCALL              18
#define EXIT_REASON_VMCLEAR             19
#define EXIT_REASON_VMLAUNCH            20
#define EXIT_REASON_VMPTRLD             21
#define EXIT_REASON_VMPTRST             22
#define EXIT_REASON_VMREAD              23
#define EXIT_REASON_VMRESUME            24
#define EXIT_REASON_VMWRITE             25
#define EXIT_REASON_VMOFF               26
#define EXIT_REASON_VMON                27
#define EXIT_REASON_CR_ACCESS           28
#define EXIT_REASON_DR_ACCESS           29
#define EXIT_REASON_IO_INSTRUCTION      30
#define EXIT_REASON_MSR_READ            31
#define EXIT_REASON_MSR_WRITE           32
#define EXIT_REASON_INVALID_STATE       33
#define EXIT_REASON_MSR_LOAD_FAIL       34
#define EXIT_REASON_MWAIT_INSTRUCTION   36
#define EXIT_REASON_MONITOR_TRAP_FLAG   37
#define EXIT_REASON_MONITOR_INSTRUCTION 39
#define EXIT_REASON_PAUSE_INSTRUCTION   40
#define EXIT_REASON_MCE_DURING_VMENTRY  41
#define EXIT_REASON_TPR_BELOW_THRESHOLD 43
#define EXIT_REASON_APIC_ACCESS         44
#define EXIT_REASON_EOI_INDUCED         45
#define EXIT_REASON_GDTR_IDTR           46
#define EXIT_REASON_LDTR_TR             47
#define EXIT_REASON_EPT_VIOLATION       48
#define EXIT_REASON_EPT_MISCONFIG       49
#define EXIT_REASON_INVEPT              50
#define EXIT_REASON_RDTSCP              51
#define EXIT_REASON_PREEMPTION_TIMER    52
#define EXIT_REASON_INVVPID             53
#define EXIT_REASON_WBINVD              54
#define EXIT_REASON_XSETBV              55
#define EXIT_REASON_APIC_WRITE          56
#define EXIT_REASON_RDRAND              57
#define EXIT_REASON_INVPCID             58
#define EXIT_REASON_VMFUNC              59
#define EXIT_REASON_ENCLS               60
#define EXIT_REASON_RDSEED              61
#define EXIT_REASON_PML_FULL            62
#define EXIT_REASON_XSAVES              63
#define EXIT_REASON_XRSTORS             64
#define EXIT_REASON_UMWAIT              67
#define EXIT_REASON_TPAUSE              68
#define EXIT_REASON_BUS_LOCK            74
#define EXIT_REASON_NOTIFY              75
```

### Intel VMSCS sdm 对照
|-----------------------------|--------|
| GUEST_INTERRUPTIBILITY_INFO | 27.7.1 |


## [ ] 在  setup_vmcs_config 中来初始化 vmcs 控制那些动作会退出

## /sys/module/kvm_amd/parameters
<!-- c8d8e7f1-1305-4fac-885c-d91cc0156706 -->

```txt
 avic
 ciphertext_hiding_asids
 debug_swap
 dump_invalid_vmcb
 enable_device_posted_irqs
 enable_ipiv
 force_avic
 intercept_smi
 lbrv
 nested
 npt
 nrips
 pause_filter_count
 pause_filter_count_grow
 pause_filter_count_max
 pause_filter_count_shrink
 pause_filter_thresh
 sev
 sev_es
 sev_snp
 tsc_scaling
 vgif
 vls
 vnmi
```

我猜测这些 pause 就是 intel 中 ple 等价物吧:
- pause_filter_count
- pause_filter_count_grow
- pause_filter_count_max
- pause_filter_count_shrink
- pause_filter_thresh

## /sys/module/kvm_intel/parameters
<!-- bab0783e-5826-4138-9d9a-f9104c31134f -->

```txt
 allow_smaller_maxphyaddr
 dump_invalid_vmcs
 emulate_invalid_guest_state
 enable_apicv
 enable_device_posted_irqs
 enable_ipiv
 enable_shadow_vmcs
 enlightened_vmcs
 ept
 eptad
 error_on_inconsistent_vmcs_config
 fasteoi
 flexpriority
 nested
 nested_early_check
 ple_gap
 ple_window
 ple_window_grow
 ple_window_max
 ple_window_shrink
 pml
 preemption_timer
 pt_mode
 sgx
 unrestricted_guest
 vmentry_l1d_flush
 vnmi
 vpid
```
- [x] fasteoi 这个参数其实是只读的，用于加速写 EOI 的模拟，具体在 handle_apic_access
- vpid 是什么，virtual pid ，用来控制 tlb 的
```txt
@[
        vmx_flush_tlb_all+5
        vcpu_enter_guest.constprop.0+1949
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+714
        kvm_vcpu_ioctl+276
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 6
```
- [x] eptad : enable_ept_ad_bits
- [x] pt_mode
```c
/* Default is SYSTEM mode, 1 for host-guest mode (which is BROKEN) */
int __read_mostly pt_mode = PT_MODE_SYSTEM;
#ifdef CONFIG_BROKEN
module_param(pt_mode, int, S_IRUGO);
#endif
```
PT_MODE_SYSTEM 是“整机不区分 host/guest 的 PT 追踪”，
PT_MODE_HOST_GUEST 是“虚拟化感知的 PT 追踪，明确区分 host 与 guest”。
所以，现在 intel 就是不区分 host/guets 的追踪的

- [x] flexpriority 做了什么
FlexPriority 是 Intel VMX 提供的 APIC 虚拟化特性：
把 Guest 的 TPR： 映射到 VMCS 中的虚拟 TPR
CPU 在硬件中完成：
- 中断优先级判断
- 屏蔽逻辑
- 不再 VM-exit
在 VMCS 中体现为： TPR_THRESHOLD VIRTUAL_APIC_PAGE 相关 VM-execution controls

## /sys/module/kvm/parameters
<!-- 790ba84c-989b-494a-9972-9f6baa3fa482 -->

对于 x86 架构下:
```txt
 allow_unsafe_mappings
 eager_page_split
 enable_pmu
 enable_virt_at_load
 enable_vmware_backdoor
 flush_on_reuse
 force_emulation_prefix
 halt_poll_ns
 halt_poll_ns_grow
 halt_poll_ns_grow_start
 halt_poll_ns_shrink
 ignore_msrs
 kvmclock_periodic_sync
 lapic_timer_advance
 min_timer_period_us
 mitigate_smt_rsb
 mmio_caching
 nx_huge_pages
 nx_huge_pages_recovery_period_ms
 nx_huge_pages_recovery_ratio
 pi_inject_timer
 report_ignored_msrs
 tdp_mmu
 tsc_tolerance_ppm
 vector_hashing
```
- [ ] eager_page_split : kvm_mmu_slot_apply_flags 和 kvm_arch_mmu_enable_log_dirty_pt_masked 中使用，似乎和 GPU 有关
- [ ] vector_hashing

对于 arm 架构下:
```txt
allow_unsafe_mappings
enable_virt_at_load
halt_poll_ns
halt_poll_ns_grow
halt_poll_ns_grow_start
halt_poll_ns_shrink
```

## arch/x86/kvm/vmx/capabilities.h

提供 vmcs_config 的封装，描述了

例如 sdm 中的 `30.4.3.1 Determining Whether a Write Access is Virtualized`

> If none of the above are true, whether a write access is virtualized depends on the settings of the “APIC-register virtualization”,
“virtual-interrupt delivery”, and “IPI virtualization” VM-execution controls:

都可以轻松的找到入口。

## 核心执行路径

- vcpu_enter_guest
  - exit_fastpath = static_call(kvm_x86_vcpu_run)(vcpu, req_immediate_exit);
    - vmx_vcpu_run
      - vmx_exit_handlers_fastpath
        - handle_fastpath_set_msr_irqoff

## 一些函数的解释
- mark_page_dirty_in_slot : 当 host 修改了 guest os 的 memory 之后，用于 dirty ring 的跟踪，

## __vmx_complete_interrupts 和 kvm_check_and_inject_events 的关系

- __vmx_complete_interrupts : 通过 idt_vectoring_info 获取 exit 的时候，还有什么 exception 需要重新注入的
- kvm_check_and_inject_events : 执行虚拟机之前，将需要注入的中断放到写到 vmcs 中

- vcpu_run
  - vcpu_enter_guest
    - 这里进行大量的 kvm_check_request 操作
    - kvm_check_and_inject_events
      - 调用 `kvm_x86_call(inject_irq)(vcpu, false);` 来真的将中断注入到 kernel 中
    - vmx_vcpu_run
      - vmx_vcpu_run 的最后调用 __vmx_complete_interrupts 后，然后调用 vmx_exit_handlers_fastpath
      所以，__vmx_complete_interrupts 的退出之后首先

## 如何处理 EXTERNAL_INTERRUPT

实现方法，提前确定好中断会集中到哪一个 CPU 中，
然后将 qemu 所有的 CPU 都绑定到一个 CPU 上。

sudo perf top -e kvm:kvm_exit
```txt
  78.98%  reason EXTERNAL_INTERRUPT rip 0x9c40d4 info 0 0
   4.23%  reason EXTERNAL_INTERRUPT rip 0x9c40da info 0 0
   3.77%  reason MSR_WRITE rip 0xffffffff81075938 info 0 0
   1.78%  reason PREEMPTION_TIMER rip 0x9c40d4 info 0 0
   1.63%  reason EXTERNAL_INTERRUPT rip 0xffffffff82606db9 info 0 0
   1.03%  reason EXTERNAL_INTERRUPT rip 0xffffffff82608d2a info 0 0
   0.73%  reason HLT rip 0xffffffff825fbaf2 info 0 0
   0.62%  reason EXTERNAL_INTERRUPT rip 0xffffffff826087b0 info 0 0
   0.55%  reason EXTERNAL_INTERRUPT rip 0xffffffff82607da5 info 0 0
   0.38%  reason MSR_WRITE rip 0xffffffff8107ca90 info 0 0
   0.28%  reason PAUSE_INSTRUCTION rip 0xffffffff82608d28 info 0 0
   0.27%  reason EXTERNAL_INTERRUPT rip 0x9c40d6 info 0 0
   0.26%  reason EXTERNAL_INTERRUPT rip 0xffffffff811b7860 info 0 0
   0.21%  reason EXTERNAL_INTERRUPT rip 0x518a90 info 0 0
   0.17%  reason EXTERNAL_INTERRUPT rip 0xffffffff81157d00 info 0 0
   0.16%  reason EXTERNAL_INTERRUPT rip 0xffffffff828014b0 info 0 0
   0.14%  reason EXTERNAL_INTERRUPT rip 0x517e96 info 0 0
   0.14%  reason EXTERNAL_INTERRUPT rip 0x9a9c50 info 0 0
   0.13%  reason EXTERNAL_INTERRUPT rip 0x9a9c53 info 0 0
   0.12%  reason PAUSE_INSTRUCTION rip 0xffffffff82606db7 info 0 0
   0.11%  reason EXTERNAL_INTERRUPT rip 0xffffffff8107593a info 0 0
   0.10%  reason EXTERNAL_INTERRUPT rip 0x518f22 info 0 0
   0.10%  reason EXTERNAL_INTERRUPT rip 0x83e00c info 0 0
```

然后用 perf 观测 qemu ，结果如下:
```txt
     clone3
     start_thread
     qemu_thread_start
     kvm_vcpu_thread_fn
     kvm_cpu_exec
     kvm_vcpu_ioctl
   - __GI___ioctl
      - 96.48% entry_SYSCALL_64_after_hwframe
           do_syscall_64
           __x64_sys_ioctl
           kvm_vcpu_ioctl
         - kvm_arch_vcpu_ioctl_run
            - 79.80% vmx_handle_exit_irqoff
               - vmx_do_interrupt_irqoff
                  - 76.58% asm_sysvec_apic_timer_interrupt
                     - 76.16% sysvec_apic_timer_interrupt
                        - 68.89% irq_exit_rcu
                           - __irq_exit_rcu
                              - 68.35% handle_softirqs
                                 - 67.08% net_rx_action
                                    - 63.56% __napi_poll.constprop.0
                                       - 54.91% rtl8169_poll
                                            26.83% __memcpy
                                          - 12.48% napi_gro_receive
                                             - 11.90% dev_gro_receive
                                                - 9.03% inet_gro_receive
                                                   - 4.76% tcp4_gro_receive
                                                      - 3.90% __skb_gro_checksum_complete
                                                         - skb_checksum
                                                            - 3.47% __skb_checksum
                                                                 csum_partial
                                                   - 3.33% tcp_gro_receive
                                                        skb_gro_receive
                                                - 1.04% napi_gro_complete.constprop.0
                                                   - 0.98% netif_receive_skb_list_internal
                                                      - 0.95% __netif_receive_skb_list_core
                                                         - 0.94% __netif_receive_skb_core.constprop.0
                                                            - 0.77% netdev_frame_hook
                                                               - 0.73% ovs_vport_receive
                                                                    0.62% ovs_dp_process_packet
```
执行起来也是简单的，退出，检查发现时 external interrupt

通过 vcpu_vmx::idt_vectoring_info 可以找到当时遇到的中断号是什么，然后直接跳到 idt 中记录的中断入口。


## svm 中断注入函数
<!-- 4dbcdb8d-cabf-46a9-83ca-e0538024665d -->

```txt
@[
        kvm_irq_delivery_to_apic_fast+5
        kvm_arch_set_irq_inatomic+178
        irqfd_wakeup+246
        __wake_up_common+120
        eventfd_write+189
        vfs_write+207
        ksys_write+99
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 21158
```
```txt
   6)               |  kvm_irq_delivery_to_apic_fast [kvm]() {
   6)   0.190 us    |    __rcu_read_lock();
   6)               |    __apic_accept_irq [kvm]() {
   6)               |      svm_deliver_interrupt [kvm_amd]() {
   6)               |        svm_complete_interrupt_delivery [kvm_amd]() {
   6)               |          __kvm_vcpu_kick [kvm]() {
   6)               |            rcuwait_wake_up() {
   6)   0.200 us    |              __rcu_read_lock();
   6)   0.190 us    |              __rcu_read_unlock();
   6)   1.110 us    |            }
   6)   0.450 us    |            kvm_arch_vcpu_should_kick [kvm]();
   6)   2.310 us    |          }
   6)   3.020 us    |        }
   6)   4.020 us    |      }
   6)   4.450 us    |    }
   6)   0.150 us    |    __rcu_read_unlock();
   6)   6.420 us    |  }
```
Hygon C86 7380 32-core Processor 是默认没打开什么优化的，也就是说，这个就是需要 kick 来加速的。

## vmx 中断注入函数
<!-- aa202e62-7e65-4b27-9a2c-96f08e8fddad -->

1. intel 下 vmx_deliver_interrupt
```c
void vmx_deliver_interrupt(struct kvm_lapic *apic, int delivery_mode,
			   int trig_mode, int vector)
{
	struct kvm_vcpu *vcpu = apic->vcpu;

	if (vmx_deliver_posted_interrupt(vcpu, vector)) {
		kvm_lapic_set_irr(vector, apic);
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		kvm_vcpu_kick(vcpu);
	} else {
		trace_kvm_apicv_accept_irq(vcpu->vcpu_id, delivery_mode,
					   trig_mode, vector);
	}
}
```

物理是否打开 vapic ，都是会走到这里的，只是正好选择两个路线。

在 vmx_deliver_posted_interrupt 中来操作，完全符合预期，可以将
中断直接注入到 vCPU 中，使用 ipi 的方法。
注意，这就是 Posted interrupt ，无需让 CPU 切换，直接注入中断，
开销都是在 QEMU 调用 eventfd 来注入的。

例如，这个是经典的中断注入，vhost net 在内核中直接注入:
```txt
@[
    vmx_deliver_interrupt+5
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic_fast+320
    kvm_arch_set_irq_inatomic+184
    irqfd_wakeup+277
    __wake_up_common+117
    eventfd_signal_mask+112
    handle_rx+275
    vhost_run_work_list+69
    vhost_task_fn+75
    ret_from_fork+49
    ret_from_fork_asm+26
]: 214680
```

qemu 将中断注入的方法:
```txt
@[
    vmx_deliver_interrupt+5
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic_fast+320
    kvm_arch_set_irq_inatomic+184
    irqfd_wakeup+277
    __wake_up_common+117
    eventfd_write+204
    vfs_write+247
    ksys_write+111
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 45307
```

```txt
- 7.08%     0.09%  vring_worker  [kernel.kallsyms]  [k] ksys_write
   - 6.99% ksys_write
      - 6.51% vfs_write
         - 5.85% eventfd_write
            - 5.26% __wake_up_common
               - irqfd_wakeup
                  - 4.66% kvm_arch_set_irq_inatomic
                     - 4.49% kvm_irq_delivery_to_apic_fast
                        - 3.78% __apic_accept_irq
                           - vmx_deliver_interrupt
                              - 1.97% __x2apic_send_IPI_mask
                                 - 0.89% __x2apic_send_IPI_dest
```

最后，在 vmx_deliver_interrupt 中，如果是传统方法，添加 KVM_REQ_EVENT 然后 kill out :
```c
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		kvm_vcpu_kick(vcpu);
```


## 在 x86 kvm 中获取 rip 的方法
<!-- ccc6e870-e340-4385-96fc-238b0cf41eae -->

kvm_rip_read(vcpu)

## EXIT_QUALIFICATION 对应的文档

SDM 中的 table 整理的很好:
```txt
Table 28-1. Exit Qualification for Debug Exceptions . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28-4
Table 28-2. Exit Qualification for Task Switches . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28-5
Table 28-3. Exit Qualification for Control-Register Accesses . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28-7
Table 28-4. Exit Qualification for MOV DR . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28-8
Table 28-5. Exit Qualification for I/O Instructions . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28-8
Table 28-6. Exit Qualification for APIC-Access VM Exits from Linear Accesses and Guest-Physical Accesses. . . . . . . . . . . . . . . . . 28-9
Table 28-7. Exit Qualification for EPT Violations . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28-10
```

可以有的:
```c
/*
 * Exit Qualifications for EPT Violations
 */
#define EPT_VIOLATION_ACC_READ_BIT	0
#define EPT_VIOLATION_ACC_WRITE_BIT	1
#define EPT_VIOLATION_ACC_INSTR_BIT	2
#define EPT_VIOLATION_RWX_SHIFT		3
#define EPT_VIOLATION_GVA_IS_VALID_BIT	7
#define EPT_VIOLATION_GVA_TRANSLATED_BIT 8
#define EPT_VIOLATION_ACC_READ		(1 << EPT_VIOLATION_ACC_READ_BIT)
#define EPT_VIOLATION_ACC_WRITE		(1 << EPT_VIOLATION_ACC_WRITE_BIT)
#define EPT_VIOLATION_ACC_INSTR		(1 << EPT_VIOLATION_ACC_INSTR_BIT)
#define EPT_VIOLATION_RWX_MASK		(VMX_EPT_RWX_MASK << EPT_VIOLATION_RWX_SHIFT)
#define EPT_VIOLATION_GVA_IS_VALID	(1 << EPT_VIOLATION_GVA_IS_VALID_BIT)
#define EPT_VIOLATION_GVA_TRANSLATED	(1 << EPT_VIOLATION_GVA_TRANSLATED_BIT)
```

## kvm.ko 和 kvm-intel.ko 分别包含的文件

kvm.ko 包含的文件为: virt/kvm 以及
```txt
kvm-y			+= x86.o emulate.o i8259.o irq.o lapic.o \
			   i8254.o ioapic.o irq_comm.o cpuid.o pmu.o mtrr.o \
			   debugfs.o mmu/mmu.o mmu/page_track.o \
			   mmu/spte.o
```

kvm-intel 和 kvm-amd 只有
arch/x86/kvm/vmx/ 和
arch/x86/kvm/svm/ 下的文件

## vmcs 的示意图
<!-- dfaea5fe-befe-41db-8d7a-87102a74a07e -->
https://docs.hyperdbg.org/tips-and-tricks/considerations/basic-concepts-in-intel-vt-x

配合 /home/martins3/data/vn/docs/kvm/vmcs-fields.md 继续看看吧，这里有
更好的东西。

The VMCS consists of six logical groups:
- Guest-state area: Processor state saved into the guest state area on VM exits and loaded on VM entries.
- Host-state area: Processor state loaded from the host state area on VM exits.
- VM-execution control fields: Fields controlling processor operation in VMX non-root operation.
- VM-exit control fields: Fields that control VM exits.
- VM-entry control fields: Fields that control VM entries.
- VM-exit information fields: Read-only fields to receive information on VM exits describing the cause and the nature of the VM exit.


我们将这些指令按功能逻辑分为四类：**环境管理、执行控制、VMCS 管理和缓存管理**。

| 指令类别           | 指令名称   | 功能描述                               | 备注                                 |
| ---                | ---        | ---                                    | ---                                  |
| **环境进入/退出**  | `VMXON`    | 进入 VMX 操作模式                      | 需先在 CR4 寄存器开启 VMXE 位        |
|                    | `VMXOFF`   | 退出 VMX 操作模式                      | 关闭硬件虚拟化支持                   |
| **VMCS 管理**      | `VMPTRLD`  | 加载并激活指定的 **VMCS** 指针         | 将特定的 VM 设为当前操作目标         |
|                    | `VMPTRST`  | 存储当前活动的 VMCS 指针               | 将当前 VMCS 地址保存到内存           |
|                    | `VMCLEAR`  | 将 VMCS 状态切换为 "Clear"             | 确保数据写回内存，以便在不同核间迁移 |
|                    | `VMREAD`   | 从 VMCS 中读取特定字段的值             | 获取虚拟机的配置或退出原因           |
|                    | `VMWRITE`  | 向 VMCS 中写入特定字段的值             | 配置虚拟机的 CPU 状态或运行策略      |
| **虚拟机执行**     | `VMLAUNCH` | 启动并进入一个新的虚拟机实例           | 第一次启动虚拟机时使用               |
|                    | `VMRESUME` | 恢复并进入一个已挂起的虚拟机           | 虚拟机退出（VM Exit）处理完后的返回  |
| **缓存与内存管理** | `INVEPT`   | 使 **EPT**（扩展页表）缓存失效         | 内存映射变更后同步缓存               |
|                    | `INVVPID`  | 使基于 **VPID** 的 TLB 缓存失效        | 切换地址空间时清除特定的 TLB 项      |
| **其他**           | `VMFUNC`   | 在不触发 VM Exit 的情况下调用 VMM 功能 | 例如：快速切换 EPT 视图              |

1. **初始化**：执行 `VMXON` 开启虚拟化引擎。
2. **配置**：通过 `VMPTRLD` 选定一个 VMCS 结构，然后利用大量的 `VMWRITE` 指令填入虚拟机的寄存器状态、控制位等。
3. **运行**：
	* 如果是首次运行，执行 `VMLAUNCH`。
	* 如果是处理完异常后返回，执行 `VMRESUME`。
4. **退出（VM Exit）**：当虚拟机触发特定条件（如执行了敏感指令）时，硬件自动退回到 VMM，此时 VMM 会用 `VMREAD` 读取退出原因。
5. **清理**：运行结束后，用 `VMCLEAR` 刷新数据并用 `VMXOFF` 关闭模式。

这次的复习相当不错了，下一次来看的时候可以继续的问题为:
1. VMPTRST 是做什么的
3. 最好是可以找到 intel 手册的图片，上面链接的图是2017 年的，不知道有没有更新了)
2. vmcs 表格中的内容需要逐个检查一下

表格的内容的解释:
```txt
› | Host GS selector | `000000101B` | `00000C0AH` | 如何理解来着？

• 这行可以拆成两层理解。

  Host GS selector 是“VMCS host-state area 里的一个 16-bit 字段”，表示 VM exit 回到 host 时，CPU 要
  装载到 GS 段寄存器可见部分的 selector 值。

  00000C0AH 不是字段内容本身，而是这个字段的 VMCS encoding，也就是你执行 VMREAD/VMWRITE 时用来指
  定“我要读写哪个 VMCS 字段”的编号。把它按 27.11.2 的规则拆开：

  - bit 0 = 0
    表示 full access
  - bits 9:1 = 000000101B
    表示这个类别里的第 5 号字段
  - bits 11:10 = 11B
    表示 host-state
  - bits 14:13 = 00B
    表示 16-bit

  所以 0x00000C0A 的意思就是：

  - 这是一个 16-bit
  - 属于 host-state
  - 索引是 5
  - 对应字段名 Host GS selector

  再强调一次：

  - Host GS selector 是“字段名”
  - 000000101B 是它在这组里的 index
  - 00000C0AH 是给 VMREAD/VMWRITE 用的 encoding

  如果你想，我可以顺手把 0xC0A 的二进制位图也画出来，或者把 Host GS selector 和 Host GS base
  (0x6C08) 的区别一起讲清楚。
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
