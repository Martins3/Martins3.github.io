# 收集一些真正的 lockless 设计

- https://doc.dpdk.org/guides/prog_guide/ring_lib.html

## 收集一些内核中 lockless 的内容

- for_each_shadow_entry_lockless
  - [ ] lockless 的实现方法
  - [ ] 为什么需要 lock
  - [ ] fast_page_fault 是怎么回事

## lockless
让人想起了 slab 的内容:
https://lwn.net/SubscriberLink/827180/a1c1305686bfea67/

- [An introduction to lockless algorithms](https://lwn.net/Articles/844224/) : 本来以为 lockless 实际上没有什么作用，但是实际上在 Linux 中是存在很多使用的

https://news.ycombinator.com/item?id=35684232
