
## [LINUX KERNEL MEMORY BARRIERS](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)

## tools/memory-model/

## 分析 kernel 中 lock 的设计模式的实现
- Documentation/virt/kvm/locking.rst

## [KVM Lock Overview](https://docs.kernel.org/virt/kvm/locking.html)

## [A Tour Through RCU’s Requirements](https://docs.kernel.org/RCU/Design/Requirements/Requirements.html)

## [A Tour Through TREE_RCU's Data Structures](https://www.kernel.org/doc/html/latest/RCU/Design/Data-Structures/Data-Structures.html)

The purpose of this combining tree is to allow per-CPU events such as quiescent states, dyntick-idle transitions, and CPU hotplug operations to be processed efficiently and scalably.

Quiescent states are recorded by the per-CPU `rcu_data` structures, and other events are recorded by the leaf-level `rcu_node` structures.

All of these events are combined at each level of the tree until finally grace periods are completed at the tree’s root rcu_node structure. A grace period can be completed at the root once every CPU (or, in the case of CONFIG_PREEMPT_RCU, task) has passed through a quiescent state.

## [A Tour Through TREE_RCU’s Grace-Period Memory Ordering](https://docs.kernel.org/RCU/Design/Memory-Ordering/Tree-RCU-Memory-Ordering.html)
