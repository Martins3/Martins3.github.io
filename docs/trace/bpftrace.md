# bpftrace 的代码
sudo bpftrace -e 'kfunc:do_idle { @[comm] = count(); }'
sudo bpftrace -e 'kfunc:do_idle { @[pid] = count(); }'

## 如何访问全局变量: https://github.com/brendangregg/bpf-perf-tools-book/blob/master/originals/Ch14_Kernel/loads.bt

sudo bpftrace -e 'kfunc:exit_signals { printf("%px\n", args->tsk->cred); }'
