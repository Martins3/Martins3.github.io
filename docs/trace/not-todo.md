## 作为一个教程
1. ./a.out 输出 printf 的全过程 : 分析 strace 中间的每一个函数

感觉 strace 可以作为很好的分析办法!
➜  hack git:(draft) ✗ strace echo "hello world" > ~/wow.md

使用 code/trace/funcgraph.sh 作为例子。

## 以后再说
- 回答这个问题:
  - 这个人不知道如何使用 ftrace 看函数的参数，实际上利用 kprobe 是可以实现的
  - https://stackoverflow.com/questions/47900809/using-ftrace-to-see-kernel-function-arguments

- 回答这个问题:
  - https://stackoverflow.com/questions/75333191/how-to-use-ftrace-to-print-partial-function-trace

## Better debugging information for inlined kernel functions
但是很难实现:
https://mp.weixin.qq.com/s/-fyFS4BfssPRJl-KfTdskw

## 在kunpeng 中构建 qemu ，发现 90% 都是在 sys 中，
perf 结果为

但是
```txt
 7.56%  cc1              [kernel.kallsyms]                                  [k] queued_spin_lock_slowpath
 6.25%  cc1              [kernel.kallsyms]                                  [k] __d_lookup_rcu
 2.46%  cc1              [kernel.kallsyms]                                  [k] lockref_get_not_dead
 1.95%  cc1              [kernel.kallsyms]                                  [k] link_path_walk.part.9
 1.67%  cc1              [kernel.kallsyms]                                  [k] lookup_fast
 0.98%  cc1              cc1                                                [.] ht_lookup_with_hash(ht*, unsigned char const*, unsigned long, uns
 0.97%  cc1              [kernel.kallsyms]                                  [k] page_counter_try_charge
 0.95%  cc1              [kernel.kallsyms]                                  [k] dput
 0.94%  cc1              cc1                                                [.] _cpp_lex_direct
 0.80%  cc1              cc1                                                [.] ggc_internal_alloc(unsigned long, void (*)(void*), unsigned long,
 0.74%  cc1              libc.so.6                                          [.] _int_malloc
 0.70%  cc1              [kernel.kallsyms]                                  [k] legitimize_path.isra.8
 0.69%  cc1              [kernel.kallsyms]                                  [k] el0_svc_common
 0.66%  cc1              cc1                                                [.] cpp_get_token_1(cpp_reader*, unsigned int*)
 0.65%  cc1              [kernel.kallsyms]                                  [k] page_counter_cancel
 0.60%  cc1              cc1                                                [.] bitmap_set_bit(bitmap_head*, int)
 0.59%  cc1              cc1                                                [.] htab_hash_string
 0.56%  cc1              libc.so.6                                          [.] malloc
```
但是构建内核没有这么大的争抢


## intel pin 类似的都整理到一起吧
https://github.com/dyninst/dyninst

## intel pt 是做什么的

- https://man7.org/linux/man-pages/man1/perf-intel-pt.1.html

看看 intel sdm v3-ch33

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
