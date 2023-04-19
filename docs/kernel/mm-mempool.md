## mempool
使用 mempool 的目的:
The purpose of mempools is to help out in situations where a memory allocation must succeed, but sleeping is not an option. To that end, mempools pre-allocate a pool of memory and reserve it until it is needed. [^16]
[^16]: [kernel doc : Driver porting: low-level memory allocation](https://lwn.net/Articles/22909/)

## 结合 raid1_reshape 分析一下
