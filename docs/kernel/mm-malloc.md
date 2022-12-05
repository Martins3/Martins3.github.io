## malloc
似乎存在一堆自定义的 malloc
1. https://github.com/microsoft/mimalloc
2. https://github.com/jemalloc/jemalloc
    1. https://stackoverflow.com/questions/1624726/how-does-jemalloc-work-what-are-the-benefits
3. https://github.com/plasma-umass/Mesh
4. https://github.com/mjansson/rpmalloc

1. 可以将 glibc 的直接替换为这些自定义的吗 ?
2. 这些东西主要考虑的设计因素是什么

- [ ] how memalign implemented ?

#### jemalloc
tls_cache 层次，是 per thread 的:
| small               | larget              |
|---------------------|---------------------|
| tcache_alloc_small  | tcache_alloc_large  |
| tcache_dalloc_small | tcache_dalloc_large |

arena 是 concurrent 的:

| small : bin           | larget       |
|-----------------------|--------------|
| arena_bin_malloc_hard | large_malloc |
| nonfull_slab_get      | large_dalloc |
| arena_slab_reg_dalloc |              |
| arena_slab_dalloc|              |

| extent                      |
|-----------------------------|
| extent_alloc_default        |
| extent_pruge_lazy_default   |
| extent_purge_forced_default |

https://www.facebook.com/notes/facebook-engineering/scalable-memory-allocation-using-jemalloc/480222803919
https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf
http://applicative.acm.org/2015/applicative.acm.org/speaker-JasonEvans.html

使用的数据结构:
1. pairing head
2. radix tree

jemalloc 是 metadata 和 userdata 相互分开的

tls : (使用什么优化，但是看不懂，JEMALLOC_TLS_MODEL)
1. https://software.intel.com/content/www/us/en/develop/blogs/the-hidden-performance-cost-of-accessing-thread-local-variables.html
2. --disable-initial-exec-tls

extend 主要完成从 kernel 获取的内存地址页的管理，由 per arena 的三个数据结构管理

分配使用是 mmap ，但是释放使用的是 madvise(MADV_FREE 和 MADV_DONTNEED)

- [jemalloc](https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf)
- https://stackoverflow.com/questions/1624726/how-does-jemalloc-work-what-are-the-benefits
可以看一下文章中间的内容，然后读一下源代码
