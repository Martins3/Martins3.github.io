---
title: 'Computer Architecture: A Quantitative Approach Appendix B'
date: 2018-08-29 10:22:59
tags: 量化
---

# Review of Memory Hierarchy
|     |     |
|:---:|:---:|
|cache                                |fully associative             |write allocate
|virtual memory                       |dirty bit                     |**unified cache**
|memory stall cycles                  |block offset                  |misses per instruction
|direct mapped                        |write back                    |**block**
|valid bit                            |data cache                    |locality
|block address                        |hit time                      |address trace
|write through                        |cache miss                    |**set**
|instruction cache                    |page fault                    |random replacement
|average memory access time           |miss rate                     |**index field**
|cache hit                            |n-way set associative         |**no-write allocate**
|page                                 |least recently used           |**write buffer**
|miss penalty                         |tag field                     |**write stall**

A fixed-size collection of data containing the requested word, called a **block** or line run, is retrieved from the main memory and placed into the cache

Latency determines the time to retrieve the first word of the block, and bandwidth determines the time to retrieve the rest of this block

processor is stalled waiting for a memory access, which we call the **memory stall cycles**

every instruction requires an **instruction access**, and it is easy to decide if it also requires a **data access**
> *就算访问instruction aceesss 访问全部从cache中间获取的，也是比CPU的速度要慢，两者之间时候含有特殊的机构来处理时间差*

using a single number for miss penalty is a simplification

Many microprocessors
today provide hardware to count the number of misses and memory references,
which is a much easier and faster way to measure miss rate.
> 似乎是可以收集的数据用于分析， 不知道如何利用对应的接口

miss rates and miss penalties are often different for reads and writes

## B.1 Introduction

### Cache Performance Review
Assume we have a computer where the cycles per instruction (CPI) is 1.0 when all
memory accesses hit in the cache. The only data accesses are loads and stores, and
these total 50% of the instructions. If the miss penalty is 50 clock cycles and the
miss rate is 1%, how much faster would the computer be if all instructions were
cache hits?
**1.75**
> 每一条指令都是含有指令的访问和%50的数据访问， 所以不是1.25。 注意， 一条指令中间可以含有多个访存， 而且instruction 获取指令必定导致访问
> !!! 无法理解miss rate 0.02的来源　B-5 最后一行



### Four Memory Hierarchy Questions
Q1: Where can a block be placed in the upper level? (block placement)
Q2: How is a block found if it is in the upper level? (block identification)
Q3: Which block should be replaced on a miss? (block replacement)
Q4: What happens on a write? (write strategy)

> !!!! 虽然这一部分看过，但是重要的名词依旧有必要梳理清楚

### An Example: The Opteron Data Cache

## B.2 Cache Performance

## B.3 Six Basic Cache Optimizations

## B.4 Virtual Memory
> virtual memory 含有这么多好处，那些好处是主要的，那些好处是附带的好处， 为了实现virtual memory 付出的代价是什么

 Page or segment is used for block, and page fault or address fault is used for miss.

### Four Memory Hierarchy Questions Revisited
> 同样的四个问题，策略有什么不同，策略不同的原因是什么


## Protection and Examples of Virtual Memory
