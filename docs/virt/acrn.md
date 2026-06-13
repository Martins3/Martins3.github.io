```txt
config ACRN_GUEST
	bool "ACRN Guest support"
	depends on X86_64
	select X86_HV_CALLBACK_VECTOR
	help
	  This option allows to run Linux as guest in the ACRN hypervisor. ACRN is
	  a flexible, lightweight reference open-source hypervisor, built with
	  real-time and safety-criticality in mind. It is built for embedded
	  IOT with small footprint and real-time features. More details can be
	  found in https://projectacrn.org/.
```

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
