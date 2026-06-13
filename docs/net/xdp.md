# xdp

- https://stackoverflow.com/questions/61666624/testing-xdp-vs-dpdk

- https://netdevconf.info/0x13/session.html?xdp-offload-with-virtio-net
  - https://www.youtube.com/watch?v=OwdU7OeO9b0&ab_channel=netdevconf
- https://events19.linuxfoundation.cn/wp-content/uploads/2017/11/Accelerating-VM-Networking-through-XDP_Jason-Wang.pdf

## [xdp-tutorial](https://github.com/xdp-project/xdp-tutorial)

这个教程是是最好的

## xdp 只能关联到到具体的网卡上面 ?
```txt
🧀  sudo ./xdp_loader --dev enp7s0 --progname xdp_pass_func
[sudo] password for martins3:
BPF object (xdp_prog_kern) listing available XDP functions
 xdp_pass_func
 xdp_drop_func
 xdp_abort_func
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: Kernel error message: Underlying driver does not support XDP in native mode
libxdp: Error attaching XDP program to ifindex 2: Operation not supported
libxdp: XDP mode not supported; try using SKB mode
xdp_program__attach: Operation not supported
```

从 dev_xdp_attach 的实现看，的确如此:

```c
	/* don't call drivers if the effective program didn't change */
	if (new_prog != cur_prog) {
		bpf_op = dev_xdp_bpf_op(dev, mode);
		if (!bpf_op) {
			NL_SET_ERR_MSG(extack, "Underlying driver does not support XDP in native mode");
			return -EOPNOTSUPP;
		}

		err = dev_xdp_install(dev, mode, bpf_op, extack, flags, new_prog);
		if (err)
			return err;
	}
```
因为 xdp 是运行接受到中断的早期，所以必须存在网卡驱动的支持。

## AF_XDP 如何理解 ?
- https://www.kernel.org/doc/html/next/networking/af_xdp.html
- https://blog.cloudflare.com/a-story-about-af-xdp-network-namespaces-and-a-cookie

这个写的更加详细点，感觉 xdp 和 AF_XDP 只是都使用了 bpf 而已
- https://rexrock.github.io/post/af_xdp1/

参考这个回答，看来 AF_XDP 可以让用户程序选择性的 bypass kernel stack，AF_XDP 的主要使用者还是 ovs, dpdk cilium 之类的
- https://stackoverflow.com/questions/71348815/do-users-need-to-change-the-socket-type-to-use-xdp


- 更多阅读材料
  - https://mp.weixin.qq.com/s?__biz=Mzg2OTc0ODAzMw==&mid=2247502385&idx=1&sn=c555d4fa2c5be9a113a8ad2acaaf3af0&source=41#wechat_redirect
  - https://mp.weixin.qq.com/s?__biz=Mzg2OTc0ODAzMw==&mid=2247502410&idx=1&sn=515f51354071a04f3b33baa2a6a6faf0&source=41#wechat_redirect



## xdp
- https://www.spinics.net/lists/xdp-newbies/msg00185.html
- https://www.tigera.io/learn/guides/ebpf/ebpf-xdp/
- https://docs.cilium.io/en/latest/bpf/

## 这些基础技术搞一搞
https://mp.weixin.qq.com/s?__biz=Mzg2OTc0ODAzMw==&mid=2247502385&idx=1&sn=c555d4fa2c5be9a113a8ad2acaaf3af0&source=41#wechat_redirect
https://mp.weixin.qq.com/s?__biz=Mzg2OTc0ODAzMw==&mid=2247502410&idx=1&sn=515f51354071a04f3b33baa2a6a6faf0&source=41#wechat_redirect
https://arthurchiao.art/blog/socket-acceleration-with-ebpf-zh/

这哥们其他的 blog 也可以看看:
https://kiosk007.top/post/ebpf%E8%B6%85%E4%B9%8E%E4%BD%A0%E6%83%B3%E8%B1%A1/


## bpf 可以直接实现网络包的转发

- https://mp.weixin.qq.com/s/slJFVkPK4GEMNQYmtCmw_g
> 与网络转发相关的 hook 点主要有 XDP（eXpress Data Path）、TC（Traffic Control）、LWT（Light Weight Tunnel）等。

```txt
# tc qdisc add dev eth1 clsact
# tc filter add dev eth1 ingress bpf da obj ingress_redirect.o sec classifier-redirect
```
用 bpftool 来观测下，而且需要测试下其效果

## 各种虚拟网卡也是支持 xdp 的

例如: drivers/net/hyperv/netvsc_bpf.c


## 收集
https://news.ycombinator.com/item?id=45812756

## 这个东西?
https://github.com/microsoft/xdp-for-windows

## xdp 真的有那么大作用吗，不就是....？

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
