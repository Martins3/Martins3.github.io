# Intel VMCS 字段表

## 来源
- PDF: `/home/martins3/data/intel-sdm-vol-3abcd.pdf`
- 文档标题: Intel(R) 64 and IA-32 Architectures Software Developer's Manual, Volume 3 (3A, 3B, 3C, & 3D)
- PDF 元数据时间: 2026-02-06
- 提取范围: Appendix B `Field Encoding in VMCS`，并参考 27.11.2 `VMREAD, VMWRITE, and Encodings of VMCS Fields`

## 说明
- 本文按 Intel 附录 B 的原始分组整理 VMCS field encoding。
- 64-bit 字段会同时列出 `(full)` 和 `(high)` 两种 encoding；这对应 27.11.2 中的 access type。
- 表后的 `Notes` 保留 Intel 原脚注，用来说明字段是否依赖特定 VMX capability。

## 编码规则速记
| 位段 | 含义 |
| --- | --- |
| bit 0 | Access type: `0 = full`, `1 = high`；仅 64-bit 字段允许 `high` |
| bits 9:1 | Index |
| bits 11:10 | Type: `0 = control`, `1 = VM-exit information / read-only data`, `2 = guest state`, `3 = host state` |
| bit 12 | 保留，必须为 0 |
| bits 14:13 | Width: `0 = 16-bit`, `1 = 64-bit`, `2 = 32-bit`, `3 = natural-width` |
| bits 31:15 | 保留，必须为 0 |

## 汇总
| Appendix 表 | Width | Type | 条目数 |
| --- | --- | --- | ---: |
| B-1 | 16-bit | Control | 5 |
| B-2 | 16-bit | Guest state | 11 |
| B-3 | 16-bit | Host state | 7 |
| B-4 | 64-bit | Control | 74 |
| B-5 | 64-bit | Read-only data | 6 |
| B-6 | 64-bit | Guest state | 42 |
| B-7 | 64-bit | Host state | 24 |
| B-8 | 32-bit | Control | 20 |
| B-9 | 32-bit | Read-only data | 8 |
| B-10 | 32-bit | Guest state | 23 |
| B-11 | 32-bit | Host state | 1 |
| B-12 | Natural width | Control | 8 |
| B-13 | Natural width | Read-only data | 6 |
| B-14 | Natural width | Guest state | 23 |
| B-15 | Natural width | Host state | 15 |
| Total | - | - | 273 |

## 详细表

### B-1 16-bit / Control

- Intel 标题: `Encoding for 16-Bit Control Fields (0000_00xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Virtual-processor identifier (VPID) | `000000000B` | `00000000H` |
| Posted-interrupt notification vector | `000000001B` | `00000002H` |
| EPTP index | `000000010B` | `00000004H` |
| HLAT prefix size | `000000011B` | `00000006H` |
| Last PID-pointer index | `000000100B` | `00000008H` |

Notes:
- [1] This field exists only on processors that support the 1-setting of the “enable VPID” VM-execution control.
- [2] This field exists only on processors that support the 1-setting of the “process posted interrupts” VM-execution control.
- [3] This field exists only on processors that support the 1-setting of the “EPT-violation #VE” VM-execution control.
- [4] This field exists only on processors that support the 1-setting of the “enable HLAT” VM-execution control.
- [5] This field exists only on processors that support the 1-setting of the “IPI virtualization” VM-execution control.

### B-2 16-bit / Guest state

- Intel 标题: `Encodings for 16-Bit Guest-State Fields (0000_10xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Guest ES selector | `000000000B` | `00000800H` |
| Guest CS selector | `000000001B` | `00000802H` |
| Guest SS selector | `000000010B` | `00000804H` |
| Guest DS selector | `000000011B` | `00000806H` |
| Guest FS selector | `000000100B` | `00000808H` |
| Guest GS selector | `000000101B` | `0000080AH` |
| Guest LDTR selector | `000000110B` | `0000080CH` |
| Guest TR selector | `000000111B` | `0000080EH` |
| Guest interrupt status | `000001000B` | `00000810H` |
| PML index | `000001001B` | `00000812H` |
| Guest UINV | `000001010B` | `00000814H` |

Notes:
- [1] This field exists only on processors that support the 1-setting of the “virtual-interrupt delivery” VM-execution control.
- [2] This field exists only on processors that support the 1-setting of the “enable PML” VM-execution control.
- [3] This field exists only on processors that support the 1-setting of either the “clear UINV” VM-exit control or the “load UINV” VM-entry control.

### B-3 16-bit / Host state

- Intel 标题: `Encodings for 16-Bit Host-State Fields (0000_11xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Host ES selector | `000000000B` | `00000C00H` |
| Host CS selector | `000000001B` | `00000C02H` |
| Host SS selector | `000000010B` | `00000C04H` |
| Host DS selector | `000000011B` | `00000C06H` |
| Host FS selector | `000000100B` | `00000C08H` |
| Host GS selector | `000000101B` | `00000C0AH` |
| Host TR selector | `000000110B` | `00000C0CH` |

### B-4 64-bit / Control

- Intel 标题: `Encodings for 64-Bit Control Fields (0010_00xx_xxxx_xxxAb)`

| Field | Index | Encoding |
| --- | --- | --- |
| Address of I/O bitmap A (full) | `000000000B` | `00002000H` |
| Address of I/O bitmap A (high) | `000000000B` | `00002001H` |
| Address of I/O bitmap B (full) | `000000001B` | `00002002H` |
| Address of I/O bitmap B (high) | `000000001B` | `00002003H` |
| Address of MSR bitmaps (full) | `000000010B` | `00002004H` |
| Address of MSR bitmaps (high) | `000000010B` | `00002005H` |
| VM-exit MSR-store address (full) | `000000011B` | `00002006H` |
| VM-exit MSR-store address (high) | `000000011B` | `00002007H` |
| VM-exit MSR-load address (full) | `000000100B` | `00002008H` |
| VM-exit MSR-load address (high) | `000000100B` | `00002009H` |
| VM-entry MSR-load address (full) | `000000101B` | `0000200AH` |
| VM-entry MSR-load address (high) | `000000101B` | `0000200BH` |
| Executive-VMCS pointer (full) | `000000110B` | `0000200CH` |
| Executive-VMCS pointer (high) | `000000110B` | `0000200DH` |
| PML address (full) | `000000111B` | `0000200EH` |
| PML address (high) | `000000111B` | `0000200FH` |
| TSC offset (full) | `000001000B` | `00002010H` |
| TSC offset (high) | `000001000B` | `00002011H` |
| Virtual-APIC address (full) | `000001001B` | `00002012H` |
| Virtual-APIC address (high) | `000001001B` | `00002013H` |
| APIC-access address (full) | `000001010B` | `00002014H` |
| APIC-access address (high) | `000001010B` | `00002015H` |
| Posted-interrupt descriptor address (full) | `000001011B` | `00002016H` |
| Posted-interrupt descriptor address (high) | `000001011B` | `00002017H` |
| VM-function controls (full) | `000001100B` | `00002018H` |
| VM-function controls (high) | `000001100B` | `00002019H` |
| EPT pointer (EPTP; full) | `000001101B` | `0000201AH` |
| EPT pointer (EPTP; high) | `000001101B` | `0000201BH` |
| EOI-exit bitmap 0 (EOI_EXIT0; full) | `000001110B` | `0000201CH` |
| EOI-exit bitmap 0 (EOI_EXIT0; high) | `000001110B` | `0000201DH` |
| EOI-exit bitmap 1 (EOI_EXIT1; full) | `000001111B` | `0000201EH` |
| EOI-exit bitmap 1 (EOI_EXIT1; high) | `000001111B` | `0000201FH` |
| EOI-exit bitmap 2 (EOI_EXIT2; full) | `000010000B` | `00002020H` |
| EOI-exit bitmap 2 (EOI_EXIT2; high) | `000010000B` | `00002021H` |
| EOI-exit bitmap 3 (EOI_EXIT3; full) | `000010001B` | `00002022H` |
| EOI-exit bitmap 3 (EOI_EXIT3; high) | `000010001B` | `00002023H` |
| EPTP-list address (full) | `000010010B` | `00002024H` |
| EPTP-list address (high) | `000010010B` | `00002025H` |
| VMREAD-bitmap address (full) | `000010011B` | `00002026H` |
| VMREAD-bitmap address (high) | `000010011B` | `00002027H` |
| VMWRITE-bitmap address (full) | `000010100B` | `00002028H` |
| VMWRITE-bitmap address (high) | `000010100B` | `00002029H` |
| Virtualization-exception information address (full) | `000010101B` | `0000202AH` |
| Virtualization-exception information address (high) | `000010101B` | `0000202BH` |
| XSS-exiting bitmap (full) | `000010110B` | `0000202CH` |
| XSS-exiting bitmap (high) | `000010110B` | `0000202DH` |
| ENCLS-exiting bitmap (full) | `000010111B` | `0000202EH` |
| ENCLS-exiting bitmap (high) | `000010111B` | `0000202FH` |
| Sub-page-permission-table pointer (full) | `000011000B` | `00002030H` |
| Sub-page-permission-table pointer (high) | `000011000B` | `00002031H` |
| TSC multiplier (full) | `000011001B` | `00002032H` |
| TSC multiplier (high) | `000011001B` | `00002033H` |
| Tertiary processor-based VM-execution controls (full) | `000011010B` | `00002034H` |
| Tertiary processor-based VM-execution controls (high) | `000011010B` | `00002035H` |
| Low PASID directory address (full) | `000011100B` | `00002038H` |
| Low PASID directory address (high) | `000011100B` | `00002039H` |
| High PASID directory address (full) | `000011101B` | `0000203AH` |
| High PASID directory address (high) | `000011101B` | `0000203BH` |
| SEAM shared EPT pointer (full) | `000011110B` | `0000203CH` |
| SEAM shared EPT pointer (high) | `000011110B` | `0000203DH` |
| PCONFIG-exiting bitmap (full) | `000011111B` | `0000203EH` |
| PCONFIG-exiting bitmap (high) | `000011111B` | `0000203FH` |
| Hypervisor-managed linear-address translation pointer (HLATP; full) | `000100000B` | `00002040H` |
| HLATP (high) | `000100000B` | `00002041H` |
| PID-pointer table address (full) | `000100001B` | `00002042H` |
| PID-pointer table address (high) | `000100001B` | `00002043H` |
| Secondary VM-exit controls (full) | `000100010B` | `00002044H` |
| Secondary VM-exit controls (high) | `000100010B` | `00002045H` |
| IA32_SPEC_CTRL mask (full) | `000100101B` | `0000204AH` |
| IA32_SPEC_CTRL mask (high) | `000100101B` | `0000204BH` |
| IA32_SPEC_CTRL shadow (full) | `000100110B` | `0000204CH` |
| IA32_SPEC_CTRL shadow (high) | `000100110B` | `0000204DH` |
| Injected-event data (full) | `000101001B` | `00002052H` |
| Injected-event data (high) | `000101001B` | `00002053H` |

Notes:
- [1] This field exists only on processors that support the 1-setting of the “use MSR bitmaps” VM-execution control.
- [2] This field exists only on processors that support the 1-setting of the “enable PML” VM-execution control.
- [3] This field exists only on processors that support the 1-setting of the “use TPR shadow” VM-execution control.
- [4] This field exists only on processors that support the 1-setting of the “virtualize APIC accesses” VM-execution control.
- [5] This field exists only on processors that support the 1-setting of the “process posted interrupts” VM-execution control.
- [6] This field exists only on processors that support the 1-setting of the “enable VM functions” VM-execution control.
- [7] This field exists only on processors that support the 1-setting of the “enable EPT” VM-execution control.
- [8] This field exists only on processors that support the 1-setting of the “virtual-interrupt delivery” VM-execution control.
- [9] This field exists only on processors that support the 1-setting of the “EPTP switching” VM-function control.
- [10] This field exists only on processors that support the 1-setting of the “VMCS shadowing” VM-execution control.
- [11] This field exists only on processors that support the 1-setting of the “EPT-violation #VE” VM-execution control.
- [12] This field exists only on processors that support the 1-setting of the “enable XSAVES/XRSTORS” VM-execution control.
- [13] This field exists only on processors that support the 1-setting of the “enable ENCLS exiting” VM-execution control.
- [14] This field exists only on processors that support the 1-setting of the “sub-page write permissions for EPT” VM-execution control.
- [15] This field exists only on processors that support the 1-setting of the “use TSC scaling” VM-execution control.
- [16] This field exists only on processors that support the 1-setting of the “activate tertiary controls” VM-execution control.
- [17] This field exists only on processors that support the 1-setting of the “PASID translation” VM-execution control.
- [18] This field exists only on processors that support the 1-setting of the “SEAM guest-physical address width” VM-execution control.
- [19] This field exists only on processors that support the 1-setting of the “enable PCONFIG” VM-execution control.
- [20] This field exists only on processors that support the 1-setting of the “enable HLAT” VM-execution control.
- [21] This field exists only on processors that support the 1-setting of the “IPI virtualization” VM-execution control.
- [22] This field exists only on processors that support the 1-setting of the “activate secondary controls” VM-exit control.
- [23] This field exists only on processors that support the 1-setting of the “virtualize IA32_SPEC_CTRL” VM-execution control.
- [24] This field exists only on processors that support FRED transitions.

### B-5 64-bit / Read-only data

- Intel 标题: `Encodings for 64-Bit Read-Only Data Fields (0010_01xx_xxxx_xxxAb)`

| Field | Index | Encoding |
| --- | --- | --- |
| Guest-physical address (full) | `000000000B` | `00002400H` |
| Guest-physical address (high) | `000000000B` | `00002401H` |
| MSR data (full) | `000000001B` | `00002402H` |
| MSR data (high) | `000000001B` | `00002403H` |
| Original-event data (full) | `000000010B` | `00002404H` |
| Original-event data (high) | `000000010B` | `00002405H` |

Notes:
- [1] This field exists only on processors that support the 1-setting of the “enable EPT” VM-execution control.
- [2] This field exists only on processors that support the 1-setting of the “enable MSR-list instructions” VM-execution control.
- [3] This field exists only on processors that support FRED transitions.

### B-6 64-bit / Guest state

- Intel 标题: `Encodings for 64-Bit Guest-State Fields (0010_10xx_xxxx_xxxAb)`

| Field | Index | Encoding |
| --- | --- | --- |
| VMCS link pointer (full) | `000000000B` | `00002800H` |
| VMCS link pointer (high) | `000000000B` | `00002801H` |
| Guest IA32_DEBUGCTL (full) | `000000001B` | `00002802H` |
| Guest IA32_DEBUGCTL (high) | `000000001B` | `00002803H` |
| Guest IA32_PAT (full) | `000000010B` | `00002804H` |
| Guest IA32_PAT (high) | `000000010B` | `00002805H` |
| Guest IA32_EFER (full) | `000000011B` | `00002806H` |
| Guest IA32_EFER (high) | `000000011B` | `00002807H` |
| Guest IA32_PERF_GLOBAL_CTRL (full) | `000000100B` | `00002808H` |
| Guest IA32_PERF_GLOBAL_CTRL (high) | `000000100B` | `00002809H` |
| Guest PDPTE0 (full) | `000000101B` | `0000280AH` |
| Guest PDPTE0 (high) | `000000101B` | `0000280BH` |
| Guest PDPTE1 (full) | `000000110B` | `0000280CH` |
| Guest PDPTE1 (high) | `000000110B` | `0000280DH` |
| Guest PDPTE2 (full) | `000000111B` | `0000280EH` |
| Guest PDPTE2 (high) | `000000111B` | `0000280FH` |
| Guest PDPTE3 (full) | `000001000B` | `00002810H` |
| Guest PDPTE3 (high) | `000001000B` | `00002811H` |
| Guest IA32_BNDCFGS (full) | `000001001B` | `00002812H` |
| Guest IA32_BNDCFGS (high) | `000001001B` | `00002813H` |
| Guest IA32_RTIT_CTL (full) | `000001010B` | `00002814H` |
| Guest IA32_RTIT_CTL (high) | `000001010B` | `00002815H` |
| Guest IA32_LBR_CTL (full) | `000001011B` | `00002816H` |
| Guest IA32_LBR_CTL (high) | `000001011B` | `00002817H` |
| Guest IA32_PKRS (full) | `000001100B` | `00002818H` |
| Guest IA32_PKRS (high) | `000001100B` | `00002819H` |
| Guest IA32_FRED_CONFIG (full) | `000001101B` | `0000281AH` |
| Guest IA32_FRED_CONFIG (high) | `000001101B` | `0000281BH` |
| Guest IA32_FRED_RSP1 (full) | `000001110B` | `0000281CH` |
| Guest IA32_FRED_RSP1 (high) | `000001110B` | `0000281DH` |
| Guest IA32_FRED_RSP2 (full) | `000001111B` | `0000281EH` |
| Guest IA32_FRED_RSP2 (high) | `000001111B` | `0000281FH` |
| Guest IA32_FRED_RSP3 (full) | `000010000B` | `00002820H` |
| Guest IA32_FRED_RSP3 (high) | `000010000B` | `00002821H` |
| Guest IA32_FRED_STKLVLS (full) | `000010001B` | `00002822H` |
| Guest IA32_FRED_STKLVLS (high) | `000010001B` | `00002823H` |
| Guest IA32_FRED_SSP1 (full) | `000010010B` | `00002824H` |
| Guest IA32_FRED_SSP1 (high) | `000010010B` | `00002825H` |
| Guest IA32_FRED_SSP2 (full) | `000010011B` | `00002826H` |
| Guest IA32_FRED_SSP2 (high) | `000010011B` | `00002827H` |
| Guest IA32_FRED_SSP3 (full) | `000010100B` | `00002828H` |
| Guest IA32_FRED_SSP3 (high) | `000010100B` | `00002829H` |

Notes:
- [1] This field exists only on processors that support either the 1-setting of the “load IA32_PAT” VM-entry control or that of the “save IA32_PAT” VM-exit control.
- [2] This field exists only on processors that support either the 1-setting of the “load IA32_EFER” VM-entry control or that of the “save IA32_EFER” VM-exit control.
- [3] This field exists only on processors that support the 1-setting of the “load IA32_PERF_GLOBAL_CTRL” VM-entry control.
- [4] This field exists only on processors that support the 1-setting of the “enable EPT” VM-execution control.
- [5] This field exists only on processors that support either the 1-setting of the “load IA32_BNDCFGS” VM-entry control or that of the “clear IA32_BNDCFGS” VM-exit control.
- [6] This field exists only on processors that support either the 1-setting of the “load IA32_RTIT_CTL” VM-entry control or that of the “clear IA32_RTIT_CTL” VM-exit control.
- [7] This field exists only on processors that support either the 1-setting of the “load IA32_LBR_CTL” VM-entry control or that of the “clear IA32_LBR_CTL” VM-exit control.
- [8] This field exists only on processors that support the 1-setting of the “load PKRS” VM-entry control.
- [9] This field exists only on processors that support either the 1-setting of the “load FRED” VM-entry control or that of the “save FRED” VM-exit control.

### B-7 64-bit / Host state

- Intel 标题: `Encodings for 64-Bit Host-State Fields (0010_11xx_xxxx_xxxAb)`

| Field | Index | Encoding |
| --- | --- | --- |
| Host IA32_PAT (full) | `000000000B` | `00002C00H` |
| Host IA32_PAT (high) | `000000000B` | `00002C01H` |
| Host IA32_EFER (full) | `000000001B` | `00002C02H` |
| Host IA32_EFER (high) | `000000001B` | `00002C03H` |
| Host IA32_PERF_GLOBAL_CTRL (full) | `000000010B` | `00002C04H` |
| Host IA32_PERF_GLOBAL_CTRL (high) | `000000010B` | `00002C05H` |
| Host IA32_PKRS (full) | `000000011B` | `00002C06H` |
| Host IA32_PKRS (high) | `000000011B` | `00002C07H` |
| Host IA32_FRED_CONFIG (full) | `000000100B` | `00002C08H` |
| Host IA32_FRED_CONFIG (high) | `000000100B` | `00002C09H` |
| Host IA32_FRED_RSP1 (full) | `000000101B` | `00002C0AH` |
| Host IA32_FRED_RSP1 (high) | `000000101B` | `00002C0BH` |
| Host IA32_FRED_RSP2 (full) | `000000110B` | `00002C0CH` |
| Host IA32_FRED_RSP2 (high) | `000000110B` | `00002C0DH` |
| Host IA32_FRED_RSP3 (full) | `000000111B` | `00002C0EH` |
| Host IA32_FRED_RSP3 (high) | `000000111B` | `00002C0FH` |
| Host IA32_FRED_STKLVLS (full) | `000001000B` | `00002C10H` |
| Host IA32_FRED_STKLVLS (high) | `000001000B` | `00002C11H` |
| Host IA32_FRED_SSP1 (full) | `000001001B` | `00002C12H` |
| Host IA32_FRED_SSP1 (high) | `000001001B` | `00002C13H` |
| Host IA32_FRED_SSP2 (full) | `000001010B` | `00002C14H` |
| Host IA32_FRED_SSP2 (high) | `000001010B` | `00002C15H` |
| Host IA32_FRED_SSP3 (full) | `000001011B` | `00002C16H` |
| Host IA32_FRED_SSP3 (high) | `000001011B` | `00002C17H` |

Notes:
- [1] This field exists only on processors that support the 1-setting of the “load IA32_PAT” VM-exit control.
- [2] This field exists only on processors that support the 1-setting of the “load IA32_EFER” VM-exit control.
- [3] This field exists only on processors that support the 1-setting of the “load IA32_PERF_GLOBAL_CTRL” VM-exit control.
- [4] This field exists only on processors that support the 1-setting of the “load PKRS” VM-exit control.
- [5] This field exists only on processors that support the 1-setting of the “load FRED” VM-exit control.

### B-8 32-bit / Control

- Intel 标题: `Encodings for 32-Bit Control Fields (0100_00xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Pin-based VM-execution controls | `000000000B` | `00004000H` |
| Primary processor-based VM-execution controls | `000000001B` | `00004002H` |
| Exception bitmap | `000000010B` | `00004004H` |
| Page-fault error-code mask | `000000011B` | `00004006H` |
| Page-fault error-code match | `000000100B` | `00004008H` |
| CR3-target count | `000000101B` | `0000400AH` |
| Primary VM-exit controls | `000000110B` | `0000400CH` |
| VM-exit MSR-store count | `000000111B` | `0000400EH` |
| VM-exit MSR-load count | `000001000B` | `00004010H` |
| VM-entry controls | `000001001B` | `00004012H` |
| VM-entry MSR-load count | `000001010B` | `00004014H` |
| Injected-event identification | `000001011B` | `00004016H` |
| Injected-event error code | `000001100B` | `00004018H` |
| VM-entry instruction length | `000001101B` | `0000401AH` |
| TPR threshold | `000001110B` | `0000401CH` |
| Secondary processor-based VM-execution controls | `000001111B` | `0000401EH` |
| PLE_Gap | `000010000B` | `00004020H` |
| PLE_Window | `000010001B` | `00004022H` |
| Instruction-timeout control | `000010010B` | `00004024H` |
| SEAM-guest KeyID | `000010011B` | `00004026H` |

Notes:
- [1] Older versions of this document called this field VM-entry interruption information.
- [2] Older versions of this document called this field VM-entry error code.
- [3] This field exists only on processors that support the 1-setting of the “use TPR shadow” VM-execution control.
- [4] This field exists only on processors that support the 1-setting of the “activate secondary controls” VM-execution control.
- [5] This field exists only on processors that support the 1-setting of the “PAUSE-loop exiting” VM-execution control.
- [6] This field exists only on processors that support the 1-setting of the “instruction timeout” VM-execution control.
- [7] This field exists only on processors that support the 1-setting of the “SEAM guest-physical address width” VM-execution control.

### B-9 32-bit / Read-only data

- Intel 标题: `Encodings for 32-Bit Read-Only Data Fields (0100_01xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| VM-instruction error | `000000000B` | `00004400H` |
| Exit reason | `000000001B` | `00004402H` |
| Exiting-event identification | `000000010B` | `00004404H` |
| Exiting-event error code | `000000011B` | `00004406H` |
| Original-event identification | `000000100B` | `00004408H` |
| Original-event error code | `000000101B` | `0000440AH` |
| VM-exit instruction length | `000000110B` | `0000440CH` |
| VM-exit instruction information | `000000111B` | `0000440EH` |

Notes:
- [1] Older versions of this document called this field VM-exit interruption information.
- [2] Older versions of this document called this field VM-exit interruption error code.
- [3] Older versions of this document called this field IDT-vectoring information.
- [4] Older versions of this document called this field IDT-vectoring error code.

### B-10 32-bit / Guest state

- Intel 标题: `Encodings for 32-Bit Guest-State Fields (0100_10xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Guest ES limit | `000000000B` | `00004800H` |
| Guest CS limit | `000000001B` | `00004802H` |
| Guest SS limit | `000000010B` | `00004804H` |
| Guest DS limit | `000000011B` | `00004806H` |
| Guest FS limit | `000000100B` | `00004808H` |
| Guest GS limit | `000000101B` | `0000480AH` |
| Guest LDTR limit | `000000110B` | `0000480CH` |
| Guest TR limit | `000000111B` | `0000480EH` |
| Guest GDTR limit | `000001000B` | `00004810H` |
| Guest IDTR limit | `000001001B` | `00004812H` |
| Guest ES access rights | `000001010B` | `00004814H` |
| Guest CS access rights | `000001011B` | `00004816H` |
| Guest SS access rights | `000001100B` | `00004818H` |
| Guest DS access rights | `000001101B` | `0000481AH` |
| Guest FS access rights | `000001110B` | `0000481CH` |
| Guest GS access rights | `000001111B` | `0000481EH` |
| Guest LDTR access rights | `000010000B` | `00004820H` |
| Guest TR access rights | `000010001B` | `00004822H` |
| Guest interruptibility state | `000010010B` | `00004824H` |
| Guest activity state | `000010011B` | `00004826H` |
| Guest SMBASE | `000010100B` | `00004828H` |
| Guest IA32_SYSENTER_CS | `000010101B` | `0000482AH` |
| VMX-preemption timer value | `000010111B` | `0000482EH` |

Notes:
- [1] This field exists only on processors that support the 1-setting of the “activate VMX-preemption timer” VM-execution control.

### B-11 32-bit / Host state

- Intel 标题: `Encoding for 32-Bit Host-State Field (0100_11xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Host IA32_SYSENTER_CS | `000000000B` | `00004C00H` |

### B-12 Natural width / Control

- Intel 标题: `Encodings for Natural-Width Control Fields (0110_00xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| CR0 guest/host mask | `000000000B` | `00006000H` |
| CR4 guest/host mask | `000000001B` | `00006002H` |
| CR0 read shadow | `000000010B` | `00006004H` |
| CR4 read shadow | `000000011B` | `00006006H` |
| CR3-target value 0 | `000000100B` | `00006008H` |
| CR3-target value 1 | `000000101B` | `0000600AH` |
| CR3-target value 2 | `000000110B` | `0000600CH` |
| CR3-target value 31 | `000000111B` | `0000600EH` |

Notes:
- [1] If a future implementation supports more than 4 CR3-target values, they will be encoded consecutively following the 4 encodings given here.

### B-13 Natural width / Read-only data

- Intel 标题: `Encodings for Natural-Width Read-Only Data Fields (0110_01xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Exit qualification | `000000000B` | `00006400H` |
| I/O RCX | `000000001B` | `00006402H` |
| I/O RSI | `000000010B` | `00006404H` |
| I/O RDI | `000000011B` | `00006406H` |
| I/O RIP | `000000100B` | `00006408H` |
| Guest-linear address | `000000101B` | `0000640AH` |

### B-14 Natural width / Guest state

- Intel 标题: `Encodings for Natural-Width Guest-State Fields (0110_10xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Guest CR0 | `000000000B` | `00006800H` |
| Guest CR3 | `000000001B` | `00006802H` |
| Guest CR4 | `000000010B` | `00006804H` |
| Guest ES base | `000000011B` | `00006806H` |
| Guest CS base | `000000100B` | `00006808H` |
| Guest SS base | `000000101B` | `0000680AH` |
| Guest DS base | `000000110B` | `0000680CH` |
| Guest FS base | `000000111B` | `0000680EH` |
| Guest GS base | `000001000B` | `00006810H` |
| Guest LDTR base | `000001001B` | `00006812H` |
| Guest TR base | `000001010B` | `00006814H` |
| Guest GDTR base | `000001011B` | `00006816H` |
| Guest IDTR base | `000001100B` | `00006818H` |
| Guest DR7 | `000001101B` | `0000681AH` |
| Guest RSP | `000001110B` | `0000681CH` |
| Guest RIP | `000001111B` | `0000681EH` |
| Guest RFLAGS | `000010000B` | `00006820H` |
| Guest pending debug exceptions | `000010001B` | `00006822H` |
| Guest IA32_SYSENTER_ESP | `000010010B` | `00006824H` |
| Guest IA32_SYSENTER_EIP | `000010011B` | `00006826H` |
| Guest IA32_S_CET | `000010100B` | `00006828H` |
| Guest SSP | `000010101B` | `0000682AH` |
| Guest IA32_INTERRUPT_SSP_TABLE_ADDR | `000010110B` | `0000682CH` |

Notes:
- [1] This field is supported only on processors that support the 1-setting of the “load CET state” VM-entry control.

### B-15 Natural width / Host state

- Intel 标题: `Encodings for Natural-Width Host-State Fields (0110_11xx_xxxx_xxx0B)`

| Field | Index | Encoding |
| --- | --- | --- |
| Host CR0 | `000000000B` | `00006C00H` |
| Host CR3 | `000000001B` | `00006C02H` |
| Host CR4 | `000000010B` | `00006C04H` |
| Host FS base | `000000011B` | `00006C06H` |
| Host GS base | `000000100B` | `00006C08H` |
| Host TR base | `000000101B` | `00006C0AH` |
| Host GDTR base | `000000110B` | `00006C0CH` |
| Host IDTR base | `000000111B` | `00006C0EH` |
| Host IA32_SYSENTER_ESP | `000001000B` | `00006C10H` |
| Host IA32_SYSENTER_EIP | `000001001B` | `00006C12H` |
| Host RSP | `000001010B` | `00006C14H` |
| Host RIP | `000001011B` | `00006C16H` |
| Host IA32_S_CET | `000001100B` | `00006C18H` |
| Host SSP | `000001101B` | `00006C1AH` |
| Host IA32_INTERRUPT_SSP_TABLE_ADDR | `000001110B` | `00006C1CH` |

Notes:
- [1] This field is supported only on processors that support the 1-setting of the “load CET state” VM-exit control.

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
