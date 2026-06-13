## 为什么内核中存在这么多 efi 相关的东西
<!-- 483dc2f8-1a1f-417e-ba38-8ce668a858a9 -->

drivers/firmware/efi/

drivers/firmware/efi/earlycon.c
drivers/firmware/efi/efi-bgrt.c

drivers/firmware/efi/ovmf-debug-log.c : 最近刚刚添加的

drivers/firmware/efi/sysfb_efi.c

也就是说，还有其他的东西?

然后看看这里的东西:
gpu/efifb.md

例如这个选项:
```txt
config EFI
	bool "EFI runtime service support"
	depends on ACPI
	select UCS2_STRING
	select EFI_RUNTIME_WRAPPERS
	select ARCH_USE_MEMREMAP_PROT
	select EFI_RUNTIME_MAP if KEXEC_CORE
	help
	  This enables the kernel to use EFI runtime services that are
	  available (such as the EFI variable services).

	  This option is only useful on systems that have EFI firmware.
	  In addition, you should use the latest ELILO loader available
	  at <http://elilo.sourceforge.net> in order to take advantage
	  of EFI runtime services. However, even with this option, the
	  resultant kernel should continue to boot on existing non-EFI
	  platforms.
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
