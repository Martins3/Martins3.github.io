# 2022
https://dblp.uni-trier.de/db/conf/asplos/asplos2022.html

TMP : 只是看到了 sessions 1B 的位置

## [ ] Software-defined address mapping: a case on 3D memory

看上去是将硬件的内存控制器的功能暴露出来，从而让访问更加的科学。

3D-stacking memory such as High-Bandwidth Memory (HBM) and Hybrid Memory Cube (HMC) provides orders of magnitude more bandwidth and significantly increased channel-level parallelism (CLP) due to its new parallel memory architecture.
- [ ] 什么是 3D memory

## [ ] Parallel virtualized memory translation with nested elastic cuckoo page tables
可以重点关注一下，应该是需要修改硬件的

## [ ] CARAT CAKE: replacing paging via compiler/kernel cooperation
论文内容： https://drive.google.com/file/d/1zYgEhjtiVTiHbGzm8PCrpcZ_EL5mgjFT/view

- [ ] 之前不是又一个基于 Rust ，重新写的一个 Linux kernel 的项目吧，对比的看看?

## NVAlloc: rethinking heap metadata management in persistent memory allocators
一个针对 nvram 的 memory allocator 的，感觉 nvram 养活了好多人啊

## Every walk's a hit: making page walks single-access cache hits.
使用两种方法帮助提升访存性能:
- 让 page table 的层数减少，每一层的 page table 的大小增大
- 让 page table cache 命中率更高 （如何更高，并不知道)

# 2020
- [Occlum: Secure and Efficient Multitasking Inside a Single Enclave of Intel SGX](https://github.com/occlum/occlum) : 基于 SGX 开发操作系统运行在 untrusted 的 os 上.
