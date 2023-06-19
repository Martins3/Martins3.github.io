# bpftrace 的代码
sudo bpftrace -e 'kfunc:do_idle { @[comm] = count(); }'
sudo bpftrace -e 'kfunc:do_idle { @[pid] = count(); }'

## 如何访问全局变量: https://github.com/brendangregg/bpf-perf-tools-book/blob/master/originals/Ch14_Kernel/loads.bt

sudo bpftrace -e 'kfunc:exit_signals { printf("%px\n", args->tsk->cred); }'

## 可以获取到行号的
通过这个方法是找到 vmlinux 的，但是如使用 nixos 获取到准确的当前 linux 的 vmlinux 不知道
```sh
find /nix/store -name vmlinux
```

用 bpftrace 可以获取到 blk_mq_submit_bio+1301 ，这里的 1301 实际上是可以解析的。

## 如何使用 if else

就像是普通的使用:
```sh
sudo bpftrace -e "kfunc:filemap_alloc_folio { if (args->order > 0) { @[kstack]=count(); } }"
```

## 如何可以实时的输出
