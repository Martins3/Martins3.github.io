# 使用 bcc 来学习内核

# https://github.com/iovisor/bcc/tree/master/tools

工具使用分析。

## 具体工具
### cachestat
- [ ] page cache 和 buffer 的命中率分析，但是感觉这个工具有 bug

### 通用工具

## funccount

## stackcount
P101 中展示了如何使用 stackcount 也是可以制作火焰图的，展示其调用来源。

## trace

## argdist

argdist cachestat dcstat fileslower javagc mountsnoop opensnoop pythoncalls rubystat sslsniff tcpaccept tplist
bashreadline cachetop deadlock filetop javaobjnew mysqld_qslower perlcalls pythonflow runqlat stackcount tcpconnect trace
biolatency capable deadlock.c funccount javastat nfsdist perlflow pythongc runqlen statsnoop tcpconnlat ttysnoop
biosnoop cobjnew doc funclatency javathreads nfsslower perlstat pythonstat runqslower syncsnoop tcpdrop vfscount
biotop cpudist drsnoop funcslower killsnoop nodegc phpcalls reset-trace shmsnoop syscount tcplife vfsstat
bitesize cpuunclaimed execsnoop gethostlatency lib nodestat phpflow rubycalls slabratetop tclcalls tcpretrans wakeuptime
bpflist dbslower ext4dist hardirqs llcstat offcputime phpstat rubyflow sofdsnoop tclflow tcpsubnet xfsdist
btrfsdist dbstat ext4slower javacalls mdflush offwaketime pidpersec rubygc softirqs tclobjnew tcptop xfsslower
btrfsslower dcsnoop filelife javaflow memleak oomkill profile rubyobjnew solisten tclstat tcptracer

## 目录 usr/share/bcc/tools 

## deadlock 这个组件如何来分析内核的


## 这个程序让我有点蒙
bpftool prog help  
o


## https://github.com/iovisor/bcc/tree/master/libbpf-tools


 nix-shell '<nixpkgs>' -A bcc --command " make -j"

应该还是可以解决的
```txt
biostacks.bpf.c:83:14: error: A call to built-in function '__stack_chk_fail' is not supported.
int BPF_PROG(blk_account_io_done, struct request *rq)
```

# 洞悉 Linux 系统和发应性能

nixos 下是 execsnoop 而 debian execsnoop-bpfcc

## 随书代码
- https://github.com/brendangregg/bpf-perf-tools-book/blob/master/originals/Ch14_Kernel/kmem.bt

## 代码查询
https://github.com/iovisor/bcc

## 什么，还可以使用这个生成火焰图
/usr/share/bcc/tools/profile -p `pidof fio` -f
