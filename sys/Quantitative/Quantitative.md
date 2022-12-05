---
title: 'Computer Architecture: A Quantitative Approach'
date: 2018-03-29 10:22:59
tags: readings
---

https://blog.fosketts.net/2011/07/06/defining-failure-mttr-mttf-mtbf/


# Chapter 2
1. Traditionally, designers of memory hierarchies focused on optimizing average memory access time, which is determined by the cache access time, miss rate, and miss penalty. More recently, however, power has become a major consideration
2. six basic way to reduce optimazation
    1. Larger block size to reduce miss rate
    2. Bigger caches to reduce miss rate
    3. Higher associativity to reduce miss rate
    4. Multilevel caches to reduce miss penalty
    5. Giving priority to read misses over writes to reduce miss penalty
    6. Avoiding address translation during indexing of the cache to reduce hit time
        1. Caches must cope with the translation of a virtual address from the processor to a physical address to access memory
        2. **Something obscure**

# Chapter 3
1. instruction-Level Parallelism: Concepts and Challenges
    1. A hazard exists whenever there is a name or data dependence between instructions, and they are close enough that the overlap during execution would change the order of access to the operand involved in the dependence
        1. Data Dependences
            1. data dependence conveys three things:
                1. the possibility of a hazard
                2. the order in which results must be calculated
                3. an upper bound on how much parallelism can possibly be exploited
            2.  A dependence can be overcome in two different ways
                1. maintaining the dependence but avoiding a hazard
                2. eliminating a dependence by transforming
        2. Name Dependences
            1. A name dependence occurs when two instructions use the same register or memory location, called a name, but there is no flow of data between the instructions associated with that name
    2.
# Chapter 4
> If you were plowing a field, which would you rather use: two strong oxen or 1024 chickens?

1. introduction
    1. perhaps the biggest advantage of SIMD versus MIMD is that the programmer continues to think sequentially yet achieves parallel speedup by having parallel data operations
    2. This chapter covers three variations of SIMD: vector architectures, multimedia SIMD instruction set extensions, and graphics processing units (GPUs)

2. Vector Architecture
    1.

3.

4. Graphics Processing Units
    1. Programming the GPU
        1. Thus the design of GPUs may make more sense when architects ask, given the hardware invested to do graphics well, how can we supplement it to improve the performance of a wider range of applications?
        2. In addition to the identifier for blocks (blockIdx) and the identifier for each thread in a block (threadIdx), CUDA provides a keyword for the number of threads per block (blockDim), which comes from the dimBlock parameter in the preceding bullet
        ```
            // Invoke DAXPY with 256 threads per Thread Block
            __host__
            int nblocks = (n+ 255) / 256;
            daxpy<<<nblocks, 256>>>(n, 2.0, x, y);
            // DAXPY in CUDA
            __global__
            void daxpy(int n, double a, double *x, double *y) {
                int i = blockIdx.x*blockDim.x + threadIdx.x;
                if (i < n) y[i] = a*x[i] + y[i];
            }
        ```
    2. NVIDIA GPU Computational Structures
        1. A Grid is the code that runs on a GPU that consists of a set of Thread Blocks.
        2.

# Chapter 6
> The datacenter is the computer.

1. introduction
    1.  shared many goals and requirements
        1. Cost-performance
        2. Energy efficiency
        3. Dependability via redundancy
        4. Network I/O
        5. Both interactive and batch processing workloads
    2. not shared with server architecture
        1. Ample parallelism
        2. Operational costs count
        3. Location counts
        4. Computing efficiently at low utilization
        5.

# Chapter 7
1. introduction
> Moore’s Law can’t continue forever … We have another 10 to 20 years before we reach a fundamental limit

2. Guidelines for DSAs
    1. Use dedicated memories to minimize the distance over which data is moved
    2. Invest the resources saved from dropping advanced microarchitectural optimizations into more arithmetic units or bigger memories
    3. Use the easiest form of parallelism that matches the domain
    4. Reduce data size and type to the simplest needed for the domain
    5. Use a domain-specific programming language to port code to the DSA

3. Example Domain: Deep Neural Networks


# Appendix M
> Those who cannot remember the past are condemned to repeat it

# External links
[SIMD wiki](https://www.wikiwand.com/en/SIMD)
[introduction to cuda](https://devblogs.nvidia.com/even-easier-introduction-cuda/)

# Questions
1. what's MIMD, hardware examples needed !

# Words
catapult
coalescing
scratchpad
dissertation
knack
stewardship
jurisdiction
photon
outage
flaky
anomaly
oxen
predate
heterogeneous
genealogy
jargon
attest


1. 需要真正的理解这一本书的内容 ?
讨论才是最好的阅读方法
create a repo and make issues for it !
2. 可以书写代码吗 ?
3. 后面的链接可以继续阅读吗 ?
4. coperate make sense !
5. Reading material
