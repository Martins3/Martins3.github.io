---
title: 'Shared Memory Consistency Models: A Tutorial'
date: 2017-05-04 15:59:00
tags: papre
---

> Warning: Published in 1995. Older than me !

# Abstraction
parallel systems that support the shared memory abstraction are becoming widely accepted in many areas of computing
> shared memory abstraction : different from OS ? by hardware ?

We focus on consistency models proposed for hardware-based sharedmemory systems

The shared memory or single address space abstraction provides several advantages over the message passing (or
private memory) abstraction by presenting a more natural transition from uniprocessors and by simplifying difficult
programming tasks such as **data partitioning** and **dynamic load distribution**

> why should use unimemory ? 
> what are the two terminology

# 1 Introduction


We begin with a short note on who should be concerned with the memory consistency model of a system.
We next describe the programming model offered by **sequential consistency**, and the **implications of sequential consistency on hardware and compiler implementations**.
We then describe several relaxed memory consistency models using a simple and uniform terminology.
The last part of the article describes the programmer-centric view of relaxed memory consistency models

# 2 Memory Consistency Models - Who Should Care?

# 3 Memory Semantics in Uniprocessor Systems

# 4 Understanding Sequential Consistency

# 5 Implementing Sequential Consistency

> Is Sequential and Consistency are controversial words ?

---

# Notes
