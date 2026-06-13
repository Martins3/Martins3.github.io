# unix domain 分析

阅读过 https://github.com/liexusong/linux-source-code-analyze/blob/master/unix-domain-sockets.md

- 通过 tlpi 的 `sockets/us_xfr_v2_cl.c` 和 `sockets/us_xfr_v2_sv.c` 掌握基本的使用方法

- [unix domain vs pipe in function](https://stackoverflow.com/questions/9475442/unix-domain-socket-vs-named-pipes)
- [unix domain vs pipe in performance](https://stackoverflow.com/questions/1235958/ipc-performance-named-pipe-vs-socket) : shared memory 直接碾压
