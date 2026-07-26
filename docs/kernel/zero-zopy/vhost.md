# vhost zero copy
可以把 vhost-net zero copy 理解成：guest 发包时，vhost 不把 guest 内存里的
packet payload 拷贝进新的 skb 数据区，而是让 skb 的 frags 直接引用这些
guest 用户页，等 skb 真正发送完成后再通知 guest 这个 virtqueue buffer 可以
回收。

普通 copy 路径大概是：

```txt
guest virtio-net TX buffer
  -> vhost 解析 virtqueue
  -> copy_from_iter() 拷贝 payload 到 host skb
  -> tap / bridge / host net stack / NIC
  -> vhost 立刻把 used ring 还给 guest
```

zero-copy TX 路径变成：

```txt
guest virtio-net TX buffer
  -> vhost 解析 virtqueue
  -> sendmsg(tap, msg_control = ubuf_info)
  -> tun/tap 创建 skb，skb frags pin/map guest pages
  -> host net stack / NIC 使用这些 frags
  -> skb 释放时回调 vhost_zerocopy_complete()
  -> vhost 再把 used ring 还给 guest
```

关键点在 drivers/vhost/net.c:855：handle_tx_zerocopy() 里给 sendmsg() 塞了
TUN_MSG_UBUF，里面带 ubuf_info 回调对象。TUN 侧在 drivers/net/tun.c:1751
识别到这个控制信息后，只拷贝头部/小段数据，剩下 payload 走
zerocopy_sg_from_iter() 挂到 skb frags 上。

它不是“完全没有 copy”。这里仍然可能 copy：

- skb linear head 通常会保留一小段，TUN 代码里注释写了约 256 字节头部空
  间。

- 小包不走 zero-copy，vhost 里 len >= VHOST_GOODCOPY_LEN 才考虑，见
  drivers/vhost/net.c:900。

- frags 太多、后端不支持、内存 pin 失败、下层设备/协议需要 linearize 时，
  都会退回 copy 或产生额外 copy。

- 这个实现主要是 TX zero-copy，即 guest -> host/backend。guest RX 方向不是
  这个机制。

为什么不能发完就立刻还 descriptor？因为 skb 还引用 guest buffer。如果
guest 立刻复用这块 virtqueue buffer，会把正在发送的数据改掉。所以 vhost 把
descriptor 状态标成 VHOST_DMA_IN_PROGRESS，等 skb 释放回调
vhost_zerocopy_complete() 后再标记 done，并按顺序更新 used ring，见
drivers/vhost/net.c:390 和 drivers/vhost/net.c:363。

所以一句话总结：vhost-net zero copy 的本质是用 skb frags 延长 guest TX
buffer 的生命周期，省掉 host 内核里“guest buffer -> skb payload”的大块数据
拷贝；代价是页 pin、异步完成、descriptor 延迟回收和更复杂的失败/乱序完成处
理。

## 实际情况
简单看了下，的确是比较容易理解哦，本来从虚拟机中的内存，作为索引，
可以提供给 发送给 nic

vhost zero copy 被报告有问题
commit 098eadce3c62 ("vhost_net: disable zerocopy by default")

如果是这样的话，那么 vhost 怎么就没有问题?

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
