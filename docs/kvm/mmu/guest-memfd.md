## 分析这个
8dd2eee9d526c30fccfe75da7ec5365c6476e510

## [Guest-first memory for KVM](https://mp.weixin.qq.com/s/XqYuS3Btcdf20ipgEtd7Ug)

```txt
config KVM_SW_PROTECTED_VM
	bool "Enable support for KVM software-protected VMs"
	depends on EXPERT
	depends on KVM && X86_64
	select KVM_GENERIC_PRIVATE_MEM
	help
	  Enable support for KVM software-protected VMs.  Currently, software-
	  protected VMs are purely a development and testing vehicle for
	  KVM_CREATE_GUEST_MEMFD.  Attempting to run a "real" VM workload as a
	  software-protected VM will fail miserably.

	  If unsure, say "N".
```

简单看了一下，似乎是需要配置 guest 为可信的才可以打开，
这个容易，反正我们现在都有各种机器，可以找一个环境测试一下:
```txt
-machine q35,confidential-guest-support=tdx0
-machine q35,confidential-guest-support=sev0
```

RAM_GUEST_MEMFD

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
