# 为tlpi 的可执行文件添加.out扩展名

1. 添加.out 的规则, 由于所有的文件都添加了 Makefile.inc
```
%.out : %.c
	$(CC) $(CFLAGS) $< -o $@  $(LDLIBS)
```
2. 将所有EXE 的变量添加上 `.out`
```
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
