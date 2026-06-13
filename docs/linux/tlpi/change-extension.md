# 为 tlpi 的可执行文件添加.out 扩展名

1. 添加.out 的规则, 由于所有的文件都添加了 Makefile.inc
```plain
%.out : %.c
	$(CC) $(CFLAGS) $< -o $@  $(LDLIBS)
```
2. 将所有 EXE 的变量添加上 `.out`
```sh
#!/bin/sh

DIRS=(lib
  acl altio
	cap
	daemons dirs_links
	filebuff fileio filelock files filesys getopt
	inotify
	loginacct
	memalloc
	mmap
	pgsjc pipes pmsg
	proc proccred procexec procpri procres
	progconc
	psem pshm pty
	shlibs
	signals sockets
	svipc svmsg svsem svshm
	sysinfo
	syslim
	threads time timers tty
	users_groups
	vdso
	vmem
  xattr
)

for i in "${DIRS[@]}"; do
  sed -i "s/^EXE = \([^#]*\)/EXE = \$(addsuffix .out, \1)/g" ${i}/Makefile
done
```

3. 手动修改其中部分部分规则，添加`.out`


# Ref
https://stackoverflow.com/questions/15287862/find-and-replace-using-regex-in-sed

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
