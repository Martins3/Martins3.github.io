## 保持当前进程运行退出
https://stackoverflow.com/questions/954302/how-to-make-a-programme-continue-to-run-after-log-out-from-ssh
ctrl+z
disown -h %1
bg 1
logout

## 密码
➜  vn git:(master) ✗ ssh-copy-id maritns3@192.168.12.34

- [ ] https://news.ycombinator.com/item?id=32486031 : ssh 技巧

- [ ] https://console.dev/articles/ssh-alternatives-for-mobile-high-latency-unreliable-connections/
- [ ] https://project-awesome.org/moul/awesome-ssh : 没有想到，原来 ssh 也是存在 awesome 项目的

> 对 ssh 设置做一些小优化可能是很有用的，例如这个 ~/.ssh/config 文件包含了防止特定网络环境下连接断开、压缩数据、多通道等选项：
>
>       TCPKeepAlive=yes
>       ServerAliveInterval=15
>       ServerAliveCountMax=6
>       Compression=yes
>       ControlMaster auto
>       ControlPath /tmp/%r@%h:%p
>       ControlPersist yes
> 一些其他的关于 ssh 的选项是与安全相关的，应当小心翼翼的使用。例如你应当只能在可信任的网络中启用 StrictHostKeyChecking=no，ForwardAgent=yes。
