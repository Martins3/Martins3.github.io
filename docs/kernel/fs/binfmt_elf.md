# binfmt_elf.c

关键参考: https://lwn.net/Articles/631631/

通用路径在 `fs/exec.c`：

```text
execve / execveat
  -> do_execveat_common()
    -> bprm_execve()
      -> exec_binprm()
        -> search_binary_handler()
          -> fmt->load_binary(bprm)
             对 ELF 来说就是 load_elf_binary()
```

`search_binary_handler()` 会遍历全局 `formats` 链表。每个元素是一个 `struct linux_binfmt`，其中 `load_binary` 是真正的格式识别和装载函数。`binfmt_elf.c` 中注册的对象是：

```c
static struct linux_binfmt elf_format = {
	.module		= THIS_MODULE,
	.load_binary	= load_elf_binary,
#ifdef CONFIG_COREDUMP
	.core_dump	= elf_core_dump,
	.min_coredump	= ELF_EXEC_PAGESIZE,
#endif
};
```

```c
static struct linux_binfmt elf_format = {
	.module		= THIS_MODULE,
	.load_binary	= load_elf_binary,
	.load_shlib	= load_elf_library,
	.core_dump	= elf_core_dump,
	.min_coredump	= ELF_EXEC_PAGESIZE,
};
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
