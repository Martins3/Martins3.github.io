---
title: 'Computer Architecture: A Quantitative Approach Appendix A'
date: 2018-08-29 10:22:59
tags: 量化
---

# Instruction Set Principles

## A.1 Introduction
> Read it with Appendix K

> What's question about this chapter ?
> Maybe only one, should be take into accout when designing a new instruction set
> 1. hardware
>   1. energy
>   2. circuit design
> 2. software
>   1. compiler
>   2. low level programmer
>   3. parallarity

The commercial importance of binary compatibility with PC software combined with the abundance of transistors provided by Moore’s Law led
Intel to use a RISC instruction set internally while supporting an 80x86 instruction set externally
> How do you know it, or just a wishful thinking ?

## A.2 Classifying Instruction Set Architectures
The type of internal storage in a processor is the most basic differentiation, so in this
section we will focus on the alternatives for this portion of the architecture
>  `accumulator architecture` can not understand or imagine !

## A.6 Instructions for Control Flow
We can distinguish four different types of control flow change:
1. Conditional branches
2. Jumps
3. Procedure calls
4. Procedure returns

The most common way to specify the destination is to supply a displacement that is added to the program counter (PC)
Control flow instructions of this sort are called PC-relative


## A.9 Putting It All Together: The RISC-V Architecture

## A.10 Fallacies and Pitfalls

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
