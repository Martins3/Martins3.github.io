# kvm timer

## steal time
这个文档整理的非常清楚了: https://liujunming.top/2022/08/20/Notes-about-KVM-steal-time/

- kvm_guest_cpu_init
  - kvm_register_steal_time : 创建 vcpu 的时候
- vcpu_enter_guest
  - record_steal_time : 进入到 guest mode 的时候
- update_rq_clock
  - update_rq_clock_task : guest 调度器计算时间的时候要考虑

## kvm clock
