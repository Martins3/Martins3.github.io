# 2012

## An Empirical Study of Memory Sharing in Virtual Machines

关于 ksm 的一些统计经验，修改了很多我现在的错误认识:

We provide insight into this issue through an exploration and analysis of memory traces captured from real user machines and controlled virtual machines.
- First, we observe that absolute sharing levels (excluding zero pages) generally remain under 15%, contrasting with prior work that has often reported savings of 30% or more.
- Second, we find that sharing within individual machines often accounts for nearly all (>90%) of the sharing potential within a set of machines, with inter-machine sharing contributing only a small amount. Moreover, even small differences between machines significantly reduce what little inter-machine sharing might otherwise be possible.
- Third, we find that OS features like address space layout randomization can further diminish sharing potential. These findings both temper expectations of real-world sharing gains and suggest that sharing efforts may be equally effective if employed within the operating system of a single machine, rather than exclusively targeting groups of virtual machines.

## 2022
- https://mp.weixin.qq.com/s/oykBIa_nr7T-WYMtWRRUpQ : serverless RunD
