# 常见的调试工具及其原理

# 常用工具

https://github.com/util-linux/util-linux

## lsof

- 一个进程 cd bin
- 然后 lsof bin 可以看到这个内容:
```txt
~ lsof bin
COMMAND  PID USER   FD   TYPE DEVICE SIZE/OFF    NODE NAME
zsh     1582 root  cwd    DIR  253,3     4096 3016181 bin
sleep   1787 root  cwd    DIR  253,3     4096 3016181 bin
```

```sh
strace  -t -e trace=file lsof bin
```
只是访问 /proc/pdi/fd 和 /proc/pdi/fdinfo 而已
