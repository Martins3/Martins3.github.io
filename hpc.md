# easy perf 阅读笔记
A good reference specifically about statistics for performance engineering is a book by Dror
G. Feitelson, “Workload Modeling for Computer Systems Performance Evaluation”, that has
more information on modal distributions, skewness, and other related topics.

# https://en.algorithmica.org/hpc/ 阅读笔记

# 三句话，实现 BLAS 80% 的性能 https://news.ycombinator.com/item?id=30509893

## links
- https://github.com/google/highway

## SIMD
- https://github.com/DLTcollab/sse2neon : 将 x86 sse 转换为 arm neon 的
  - 这里有一个附录，描述各种使用 simd 的库，而且使用 sss3neon 转换

- [ARM’s Scalable Vector Extensions: A Critical Look at SVE2 For Integer Workloads](https://gist.github.com/zingaburga/805669eb891c820bd220418ee3f0d6bd)
  - 非常有趣
