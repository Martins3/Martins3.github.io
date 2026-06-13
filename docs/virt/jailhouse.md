
```txt
config JAILHOUSE_GUEST
	bool "Jailhouse non-root cell support"
	depends on X86_64 && PCI
	select X86_PM_TIMER
	help
	  This option allows to run Linux as guest in a Jailhouse non-root
	  cell. You can leave this option disabled if you only want to start
	  Jailhouse and run Linux afterwards in the root cell.
```

不知道这个项目是不是做这个的: https://github.com/siemens/jailhouse

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
