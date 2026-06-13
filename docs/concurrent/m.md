# Shared Memory Consistency Models: A Tutorial

https://www.lri.fr/~cecile/ENSEIGNEMENT/IPAR/Exposes/Consistency.pdf

经典必读

## Abstraction
parallel systems that support the shared memory abstraction are becoming widely accepted in many areas of computing
> shared memory abstraction : different from OS ? by hardware ?

We focus on consistency models proposed for hardware-based sharedmemory systems

The shared memory or single address space abstraction provides several advantages over the message passing (or
private memory) abstraction by presenting a more natural transition from uniprocessors and by simplifying difficult
programming tasks such as **data partitioning** and **dynamic load distribution**

> why should use unimemory ?
> what are the two terminology

## 1 Introduction


We begin with a short note on who should be concerned with the memory consistency model of a system.
We next describe the programming model offered by **sequential consistency**, and the **implications of sequential consistency on hardware and compiler implementations**.
We then describe several relaxed memory consistency models using a simple and uniform terminology.
The last part of the article describes the programmer-centric view of relaxed memory consistency models

## 2 Memory Consistency Models - Who Should Care

## 3 Memory Semantics in Uniprocessor Systems

## 4 Understanding Sequential Consistency

## 5 Implementing Sequential Consistency

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
