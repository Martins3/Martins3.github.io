## bpftrace 的代码
sudo bpftrace -e 'kfunc:do_idle { @[comm] = count(); }'
sudo bpftrace -e 'kfunc:do_idle { @[pid] = count(); }'
