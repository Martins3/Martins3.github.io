# mm/frontswap.c

1. 一共存在哪几种 backend ?
2. frontend 如何组织它们 ?
3. 涉及到硬件了吗 ?
5. 前端和后端之间的接口是什么 ?

- [ ] 至少存在 swap 下是如何使用文件和磁盘的分流的



- http://www.wowotech.net/memory_management/zram.html : 就是这个吗 ?
  - https://github.com/maximumadmin/zramd

## https://lwn.net/Articles/454795/

Exactly how the kernel talks to tmem will be described in **Part 2**,
but there are certain classes of data maintained by the kernel that are suitable.
Two of these are known to kernel developers as "clean pagecache pages" and "swap pages".

> 两种类型的适合 tmem

Collectively these sources of suitable data for tmem can be referred to as "frontends" for tmem
and we will detail them in **Part 3**.

The two basic operations of tmem are "put" and "get".
If the kernel wishes to save a chunk of data in tmem, it uses the "put" operation, providing a pool id, a handle, and the location of the data;
if the put returns success, tmem has copied the data.

within a pool, the kernel chooses a unique "handle" to represent the equivalent of an address for the chunk of data.
When the kernel requests the creation of a pool, it specifies **certain attributes** to be described below.
If pool creation is successful, tmem provides a "pool id".
Handles are unique within pools, not across pools, and consist of a 192-bit "object id" and a 32-bit "index."
The rough equivalent of an object is a "file" and the index is the rough equivalent of a page offset into the file.
> handle 来描述 equivalent of an address for ...
> handle 等价于 地址

If the kernel wishes to retrieve data, it uses the "get" operation and provides the pool id, the handle, and a location for tmem to place the data;
if the get succeeds, on return, the data will be present at the specified location.
Note that, unlike I/O, the copying performed by tmem is fully synchronous.
As a result, arbitrary locks can (and, to avoid races, often should!) be held by the caller.

There are two basic pool types: ephemeral and persistent.
Pages successfully put to an **ephemeral** pool may or may not be present later when the kernel uses a subsequent get with a matching handle.
Pages successfully put to a **persistent** pool are guaranteed to be present for a subsequent get. (Additionally, a pool may be "shared" or "private".)

The kernel is responsible for maintaining coherency between tmem and the kernel's own data,
and tmem has two types of "flush" operations to assist with this: To disassociate a handle from any tmem data, the kernel uses a "flush" operation.
To disassociate all chunks of data in an object, the kernel uses a "flush object" operation.
After a flush, subsequent gets will fail. A get on an (unshared) ephemeral pool is destructive, i.e. implies a flush; otherwise, the get is non-destructive and an explicit flush is required.
(There are two additional coherency guarantees that are described in Appendix B.)
> 数据是一次性的吗 ? get 一次就没有了

While other frontends are possible, the two existing tmem frontends, frontswap and cleancache, cover two of the primary types of kernel memory that are sensitive to memory pressure.
These two frontends are complementary:
1. **cleancache handles (clean) mapped pages that would otherwise be reclaimed by the kernel;**
2. **frontswap handles (dirty) anonymous pages that would otherwise be swapped out by the kernel.**

> clean 和 dirty 的含义是什么 ?
