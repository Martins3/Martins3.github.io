# tcp_vegas.c , tcp_cubic.c, cong_avoid.c, tcp_bbr.c, tcp_westwood.c 分别定义了几种 traffic control 的方法
```c
struct tcp_congestion_ops tcp_reno = {
	.flags		= TCP_CONG_NON_RESTRICTED,
	.name		= "reno",
	.owner		= THIS_MODULE,

	/* return slow start threshold (required) */
	.ssthresh	= tcp_reno_ssthresh,
	/* do new cwnd calculation (required) */ .cong_avoid	= tcp_reno_cong_avoid,
	/* new value of cwnd after loss (required) */
	.undo_cwnd	= tcp_reno_undo_cwnd,
};
```



# https://wiki.aalto.fi/download/attachments/69901948/TCP-CongestionControlFinal.pdf




# [万字详文：TCP 拥塞控制详解](https://zhuanlan.zhihu.com/p/144273871)

下面介绍几种常用的 RTT 算法。
1. rtt 经典算法 [RFC793]
2. rtt 标准算法（Jacobson / Karels 算法）
  - Linux 的源代码在：tcp_rtt_estimator
3. Karn 算法

> - [ ] 这里，只是找到了其中的一个算法，其他的实现暂时没有看, 但是应该也是很重要的
> - [ ]  文章中间还分析了一些辅助机制

> 
几种快速重传算法:
- SACk
- FACk
- RACK
  - tcp_rack_advance

丢包是网络波动，还是真的出现了拥塞 ?

- [nagle](https://stackoverflow.com/questions/17842406/how-would-one-disable-nagles-algorithm-in-linux) ???

