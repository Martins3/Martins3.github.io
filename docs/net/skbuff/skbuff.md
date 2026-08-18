# skbuff

## 克隆与复制

三个层次，开销递增

### skb_clone：只复制元数据

`net/core/skbuff.c` 中 `skb_clone()`：

```c
struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t gfp_mask)
{
	struct sk_buff_fclones *fclones = container_of(skb, struct sk_buff_fclones, skb1);
	struct sk_buff *n;

	if (skb_orphan_frags(skb, gfp_mask))	/* zerocopy 的页不能直接共享 */
		return NULL;

	if (skb->fclone == SKB_FCLONE_ORIG &&
	    refcount_read(&fclones->fclone_ref) == 1) {
		/* fclone 快路径：直接用成对分配的 skb2 */
		n = &fclones->skb2;
		refcount_set(&fclones->fclone_ref, 2);
		n->fclone = SKB_FCLONE_CLONE;
	} else {
		n = kmem_cache_alloc(net_hotdata.skbuff_cache, gfp_mask);
		if (!n)
			return NULL;
		n->fclone = SKB_FCLONE_UNAVAILABLE;
	}
	return __skb_clone(n, skb);
}
```

`__skb_clone()` 拷贝元数据（`__copy_skb_header()` 一把 memcpy headers group），共享 head/data/tail/end，然后：

```c
	atomic_inc(&(skb_shinfo(skb)->dataref));
	skb->cloned = 1;   /* 原件标记 cloned */
	n->cloned = 1;     /* 克隆同样标记 */
	refcount_set(&n->users, 1);
```

克隆出的 skb 与原件共享同一份数据缓冲区和 shinfo，由 `dataref` 计数；任何一方想改数据都得先 COW。克隆的 `sk` 和 `destructor` 置 NULL（不属于 socket，不做内存记账）。典型场景：tcpdump 抓包、报文同时转发和本地投递、TCP 重传队列。

### pskb_copy：复制线性区头部

`__pskb_copy_fclone()`（`pskb_copy()` 是它的包装）：分配新 skb，只拷贝 `skb_headlen()` 这段线性数据（加上指定 headroom），frags 逐个 `skb_frag_ref()` 加页引用继续共享，frag_list 用 `skb_clone_fraglist()` 给每个子 skb 加引用。适合"只想改头部"的场景（比如 NAT 改地址）。

### skb_copy / skb_copy_expand：完整深拷贝

`net/core/skbuff.c` 中 `skb_copy()`：

```c
	headerlen = skb_headroom(skb);
	size = skb_end_offset(skb) + skb->data_len;
	n = __alloc_skb(size, gfp_mask, skb_alloc_rx_flag(skb), NUMA_NO_NODE);
	...
	skb_reserve(n, headerlen);
	skb_put(n, skb->len);
	BUG_ON(skb_copy_bits(skb, -headerlen, n->head, headerlen + skb->len));
	skb_copy_header(n, skb);
```

注意它的副产品：通过 `skb_copy_bits()` 把非线性数据也拷进新缓冲区，返回的 skb 是完全线性的、完全私有的。另外它会拒绝 `SKB_GSO_FRAGLIST` 类型的 skb（`WARN_ON_ONCE` 后返回 NULL）。`skb_copy_expand()` 类似，额外允许指定新的 headroom/tailroom。

### COW：skb_cow / skb_cow_head / pskb_expand_head / skb_unshare

判断函数（`include/linux/skbuff.h`）：

- `skb_shared(skb)`：`users != 1`，即 skb 对象本身被别人引用（比如在别的队列里）。
- `skb_cloned(skb)`：`cloned` 置位且 `dataref & SKB_DATAREF_MASK != 1`，即数据缓冲区被共享。
- `skb_header_cloned(skb)`：扣除 payload-only 引用后仍不止一个引用，即头部不可写。

修改报文内容前的标准动作是 COW：

```c
static inline int __skb_cow(struct sk_buff *skb, unsigned int headroom, int cloned)
{
	int delta = 0;

	if (headroom > skb_headroom(skb))
		delta = headroom - skb_headroom(skb);

	if (delta || cloned)
		return pskb_expand_head(skb, ALIGN(delta, NET_SKB_PAD), 0, GFP_ATOMIC);
	return 0;
}
```

`skb_cow()` 传 `skb_cloned()` 的结果；`skb_cow_head()` 传 `skb_header_cloned()` 的结果——后者用于"只 push 头部、不改数据"的场景，对 TCP 那种 payload-only 克隆更宽松。

核心在 `net/core/skbuff.c` 的 `pskb_expand_head()`：要求 `users == 1`（`BUG_ON(skb_shared(skb))`），重新 `kmalloc_reserve()` 一块数据区，把旧数据 memcpy 过去，连 shinfo 一起拷（只拷到 `frags[nr_frags]`，这就是 `frags` 必须是 shinfo 最后一个字段的原因）。若原先是克隆，则给所有 frags 加引用、克隆 frag_list，再 `skb_release_data()` 优雅地放下旧头；否则直接 `skb_free_head()`。最后修正所有指针和头部偏移（`skb_headers_offset_update()`），`cloned` 清零、`dataref` 归 1。

`skb_unshare()` 是更重的版本：`skb_cloned()` 为真时直接 `skb_copy()` 出一个全新的私有副本（含全部非线性数据），返回新 skb；常用于协议栈要长期持有并修改报文的场景。`skb_share_check()` 类似但只做 `skb_clone()`（仍共享数据）。

## TCP 在发送的时候会 clone 一下


从这个 union 看，skb 明显是存在于两个数据结构中的:
```c
struct sk_buff {
	union {
		struct {
			/* These two members must be first to match sk_buff_head. */
			struct sk_buff		*next;
			struct sk_buff		*prev;

			union {
				struct net_device	*dev;
				/* Some protocols might use this space to store information,
				 * while device pointer would be NULL.
				 * UDP receive path is one user.
				 */
				unsigned long		dev_scratch;
			};
		};
		struct rb_node		rbnode; /* used in netem, ip4 defrag, and tcp stack */
		struct list_head	list;
		struct llist_node	ll_node;
	};
```

TCP 不会在第一次发送后立即释放原始 skb，而是把它作为 master skb 保存在重传队列中：

因为 TCP 发送成功不等于对端已经收到。在收到 ACK 之前，TCP 必须保留原始数据，以便检测丢包和重新发送。

TCP 原始 skb
    │
    ├── clone → 加 TCP/IP 头 → 发给网络层
    │
    └── master → 保留在 tcp_rtx_queue 红黑树中

发生重传时，__tcp_transmit_skb() 再从 master skb 克隆一份用于发送。

问题在于 skb_clone() 的行为大致是：

```txt
n->next = n->prev = NULL;
n->dev = old->dev;
```

### skb 的生命周期

```txt
  应用写入数据
      │
      ▼
  sk_write_queue
  （尚未发送）
      │
      ├── clone → 交给 IP/qdisc/驱动
      │            之后可能随时被消费、释放
      │
      ▼
  tcp_rtx_queue
  （master：已发送但尚未 ACK）
      │
      ├── 收到 ACK ──────→ 从树中删除并释放
      │
      └── 判断丢包 ──────→ 再 clone 一份重传

  不能依赖发送给网络层的 clone，因为 skb 一旦交给下层，所有权也交出去了；下层可能排队、分片、修改或释放它。因此 TCP 必须保留自己控制的 master。
```

### 为什么使用红黑树

这个 rbtree 本质上是 TCP 的“已发送但尚未确认”队列。

tcp_rtx_queue 按 TCP 序列号排列。例如：

```txt
seq 1000～1999
seq 2000～2999
seq 3000～3999
seq 4000～4999
```

TCP 收到 ACK/SACK 后，需要快速完成这些操作：

- 找出 ACK 覆盖了哪些 skb；
- 根据 SACK 区间定位对应 skb；
- 找出未被确认的缺口；
- 标记丢包；
- 选择需要重传的 skb；
- 按序列号顺序遍历未确认数据。

例如收到：

累计 ACK = 3000
SACK      = [8000, 10000)

含义是：

1000～2999  已累计确认，可以释放
3000～7999  可能存在丢包
8000～9999  已经乱序到达

红黑树可以根据序列号在 O(log N) 时间内定位 SACK 区间起点，然后顺序遍历该区间。

## skb 在队列中的关系

这个 union 的设计逻辑其实很简单：一个 skb 同一时刻只会以一种方式被组织，所以几种链接方式共享同一块内存（include/linux/skbuff.h 中
struct sk_buff 开头的 union，注释写明 "rbnode: RB tree node, alternative to next/prev"）。

什么时候用 next/prev（在链表里）

默认形态。skb 挂进任何 struct sk_buff_head 双向链表时用这对指针：

- socket 的 sk_receive_queue、sk_write_queue、sk_error_queue
- qdisc 的普通 FIFO 队列、软中断的 backlog 队列
- 各种临时链表（skb_queue_tail() / skb_dequeue() 一族）

什么时候用 rbnode（在红黑树里）

需要按 key 有序查找而不是 FIFO 的场合，目前内核里就这几类：

- TCP / MPTCP 乱序队列：tcp_rbtree_insert()（net/ipv4/tcp_input.c）和 mptcp 的 out_of_order_queue，按序列号排序
- IP 分片重组：net/ipv4/ip_fragment.c、net/ipv6/reassembly.c，按分片偏移排序
- 定时发送的 qdisc：sch_netem、sch_etf、sch_fq 的 t_root，按发送时间戳排序

struct rb_node 占 3 个指针（parent+color、left、right），恰好覆盖 next、prev、dev 三个字段。

dev 什么时候有效

- 收发包穿越协议栈期间需要它：RX 路径记录入接口，TX 路径记录出接口（__dev_queue_xmit() 用它找到要发的设备）。
- dev 和 next/prev 在内层 struct 里是并列关系，不冲突——所以 skb 在链表里排队时 dev 依然有效。
- 但 skb 一旦进红黑树，dev 就被覆盖掉了。 上面那些 rbtree 场景全是"设备已无关"的阶段
（报文已经到 socket 层、或在重组、或在定时队列）。出队时必须恢复，sch_netem.c 里就有现成的注释和动作：

```c
static struct sk_buff *netem_dequeue(struct Qdisc *sch)
{
	// ...
  /* skb->dev shares skb->rbnode area,
   * we need to restore its value.
   */
  skb->dev = qdisc_dev(sch);
```

ip_fragment.c 里也有同样的注释 "skb->rbnode and skb->dev share the same location"。

另外 dev 自己还是个小 union：dev_scratch——当协议层确定 dev 为 NULL 时（比如 UDP 收包路径），复用这 8 字节存私有数据。

链表里排队用 next/prev，树里排序用 rbnode，dev 只在穿越协议栈时有效，进树就失效、出树要恢复。

## 实验
### net: 路由角度理解设备切换
<!-- 1b404733-553b-48de-8de7-c93e33b3b091 -->

```c
int ip_output(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	struct net_device *dev, *indev = skb->dev; // skb 原来所属的设备
	int ret_val;

	rcu_read_lock();
	dev = skb_dst_dev_rcu(skb); // 由路由规则找到的发送设备
	skb->dev = dev; // 切换原来的设备
	skb->protocol = htons(ETH_P_IP);

	ret_val = NF_HOOK_COND(NFPROTO_IPV4, NF_INET_POST_ROUTING,
				net, sk, skb, indev, dev,
				ip_finish_output,
				!(IPCB(skb)->flags & IPSKB_REROUTED));
	rcu_read_unlock();
	return ret_val;
}
```
配合 docs/net/skbuff/skbuff.bt 使用，可以观察到 skb 在不同的网卡中移动中移动的效果，


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
