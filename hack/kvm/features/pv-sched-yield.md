# PV_SCHED_YIELD

guset: 想要发送 ipi 给

- kvm_smp_send_call_func_ipi: 

host:

- kvm_emulate_hypercall
  - kvm_sched_yield
    - kvm_vcpu_yield_to : 如果 target 没有运行，让 target 开始执行。
