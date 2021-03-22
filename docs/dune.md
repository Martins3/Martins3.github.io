# Loongson Box : A Process Level Virtualization framework Base on KVM

> 项目地址 : https://github.com/Martins3/loongson-dune

原本的 x86 dune 的设计需要使用内核模块，难以维护，和 kvm 想必，其并没有良好的考虑一些 corner case, 更重要的是，其内核的实现和 kvm 的实现存在众多的重合部分，
而其简化部分往往会导致一些安全隐患。

通过将系统调用模拟放到用户态并且利用 kvm 提供的基础设施，我们设计了 Loongson Box,
主要的贡献为:
1. 对于 fork / clone 特殊处理: 大多数的系统调用只需要简单的转发，但是 fork / clone 需要创建新的 vm 后者 vcpu 出来。
2. 对于 userlocal 等寄存器的特殊处理

为了验证 Loongson Box 的正确性，使用了 ltp 作为测试项目, 完全符合预期效果。
