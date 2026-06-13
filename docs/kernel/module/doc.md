https://docs.kernel.org/kbuild/index.html

## Kconfig Language
- https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html


```rst
- reverse dependencies: "select" <symbol> ["if" <expr>]

  While normal dependencies reduce the upper limit of a symbol (see
  below), reverse dependencies can be used to force a lower limit of
  another symbol. The value of the current menu symbol is used as the
  minimal value <symbol> can be set to. If <symbol> is selected multiple
  times, the limit is set to the largest selection.
  Reverse dependencies can only be used with boolean or tristate
  symbols.

  Note:
	select should be used with care. select will force
	a symbol to a value without visiting the dependencies.
	By abusing select you are able to select a symbol FOO even
	if FOO depends on BAR that is not set.
	In general use select only for non-visible symbols
	(no prompts anywhere) and for symbols with no dependencies.
	That will limit the usefulness but on the other hand avoid
	the illegal configurations all over.

	If "select" <symbol> is followed by "if" <expr>, <symbol> will be
	selected by the logical AND of the value of the current menu symbol
	and <expr>. This means, the lower limit can be downgraded due to the
	presence of "if" <expr>. This behavior may seem weird, but we rely on
	it. (The future of this behavior is undecided.)
```

这两个警告按道理应该直接警告的。

## Kbuild
https://docs.kernel.org/kbuild/kbuild.html

编译时候的各种选项以及文件产出，例如 modules.order

## Configuration targets and editors
https://docs.kernel.org/kbuild/kconfig.html

介绍各种 make menuconfig 之类的使用

## 有办法用脚本总结那些代码是和硬件相关，那些和硬件无关的吗?
1. 操作系统到底需要提供什么功能 ?
2. 其中那些功能是依赖于硬件实现的 ?
2. 那些是需要操作系统封装的 ?
2. 那些不需要硬件支持 ?



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
