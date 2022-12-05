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
