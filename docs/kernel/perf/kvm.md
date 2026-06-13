# 分析 x86_emulate_instruction 大致路径

```txt
         - 15.70% __x64_sys_ioctl                                                                                                                                                                                                                            ▒
            - 14.45% kvm_vcpu_ioctl                                                                                                                                                                                                                          ▒
               - 13.98% kvm_arch_vcpu_ioctl_run                                                                                                                                                                                                              ▒
                  - 5.44% vmx_handle_exit                                                                                                                                                                                                                    ▒
                     - 5.06% x86_emulate_instruction                                                                                                                                                                                                         ▒
                        - 3.13% x86_decode_emulated_instruction                                                                                                                                                                                              ▒
                           - 2.86% x86_decode_insn                                                                                                                                                                                                           ▒
                              - 2.28% __do_insn_fetch_bytes                                                                                                                                                                                                  ▒
                                 - 1.94% kvm_fetch_guest_virt                                                                                                                                                                                                ▒
                                    - 1.23% paging64_gva_to_gpa                                                                                                                                                                                              ▒
                                         1.06% paging64_walk_addr_generic                                                                                                                                                                                    ▒
                        - 1.66% x86_emulate_insn                                                                                                                                                                                                             ▒
                           - 0.72% emulator_read_write                                                                                                                                                                                                       ▒
                              - emulator_read_write_onepage                                                                                                                                                                                                  ▒
                                   0.52% write_mmio                                                                                                                                                                                                          ▒
                  - 2.17% vmx_vcpu_run                                                                                                                                                                                                                       ▒
                       0.53% native_write_msr                                                                                                                                                                                                                ▒
                  - 1.50% vmx_prepare_switch_to_guest                                                                                                                                                                                                        ▒
                     - 0.80% kvm_set_user_return_msr                                                                                                                                                                                                         ▒
                          0.55% native_write_msr_safe                                                                                                                                                                                                        ▒
                  - 1.49% fpu_swap_kvm_fpstate                                                                                                                                                                                                               ▒
                       0.76% restore_fpregs_from_fpstate                                                                                                                                                                                                     ▒
                  - 1.02% vcpu_put                                                                                                                                                                                                                           ▒
                     - 0.99% kvm_arch_vcpu_put                                                                                                                                                                                                               ▒
                          vmx_vcpu_put                                                                                                                                                                                                                       ▒
                  - 0.68% vcpu_load                                                                                                                                                                                                                          ▒
                       0.65% kvm_arch_vcpu_load                                                                                                                                                                                                              ▒
            - 0.64% kvm_vm_ioctl                                                                                                                                                                                                                             ▒
               - 0.56% kvm_send_userspace_msi                                                                                                                                                                                                                ▒
                  - 0.53% kvm_set_msi                                                                                                                                                                                                                        ▒
                       0.51% kvm_irq_delivery_to_apic
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
