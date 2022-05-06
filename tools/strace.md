# strace 基本使用

过滤掉特定的 syscall
```sh
#!/bin/bash
strace -e 'trace=!read,writev' tcpdump -A -s0 port 80
```
