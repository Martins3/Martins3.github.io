# 实现两个终端通信

1. tty 设备 : how ?
2. posix 和 sysv 的 IPC
    1. semafore
3. socket : tcp udp
4. epoll + 普通的文件

5. 添加内核模块
6. signal 机制

7. 修改终端模拟器 / libcurse 库 ？

介绍一种神奇的方法echo "shit" > /dev/pts/3
