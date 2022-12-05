# 我所知道 systemd 的全部

## 官方文档
- https://www.freedesktop.org/wiki/Software/systemd/
- https://systemd.io/

## ruanyf 的教程
https://www.ruanyifeng.com/blog/2016/03/systemd-tutorial-commands.html

## 常用命令
- 查看依赖
  - sudo systemctl list-dependencies
  - https://serverfault.com/questions/617398/is-there-a-way-to-see-the-execution-tree-of-systemd
- 检查日志
  - https://unix.stackexchange.com/questions/20399/view-stdout-stderr-of-systemd-service
    - sudo journalctl -u [unitfile]

## 各种 init
- https:/github.com/troglobit/finit : Finit is a simple alternative to SysV init and systemd.
- https://github.com/krallin/tini : A tiny but valid init for containers

## blog
- https://blog.k8s.li/systemd.html

## 原理上的疑惑
- [ ] 如何将程序转换为 deamon
- [ ] D-bus 的工作原理
- [ ] 如何和 cgroup 交互
- [ ] /var 下的 journal 到底是谁生成的，是 systemd 管理的吗?

## 不要在参数上引入多余的双引号

```sh
cat /etc/systemd/system/hugepage.service

[Unit]
Description=A simple echo

[Service]
Type=oneshot
ExecStart="/bin/echo 1000"
TimeoutStopSec=10
KillMode=process

[Install]
WantedBy=multi-user.target
```

如果使用 pipeline 等复杂的 shell 操作，应该使用上 /bin/sh -c "cmd"

## rc.local

不要使用 rc.local [^1]

如果非要使用，记得
```sh
chmod +x /etc/rc.d/rc.local
```




- [ ] 为什么内核参数可以管理 systemd

systemctl list-units --type=service

- [ ] TimeoutStopSec=10 似乎没用

## 问题
启动这个服务实际上会等待 10s 的:
```sh
[Unit]
Description=MountSmokeScreen

[Service]
Type=oneshot
ExecStart=/bin/sleep 10
TimeoutStopSec=1

[Install]
WantedBy=multi-user.target
```

- [ ] 为什么 systemd 挂掉之后，reboot 也不能正常使用了。

## [ ] 没有理解 target 和 wanted by 是什么意思

## [ ] 到底是守护进程还是一个 oneshot 的似乎是存在区别的

## [ ] WantedBy 和 RequiredBy 的区别是什么？

## [ ] service 的 Type 是 dbus 该如何理解


## 如何让内核不断 reboot

在 /root 中:
[root@localhost ~]# cat reboot.sh
```sh
cat << 'EOF' > /root/reboot.sh
#!/bin/bash

C=/root/counter

if [[ ! -f $C ]];then
echo 1 > $C
fi

a=$(cat $C)
echo $a
a=$((a+1))
echo $a > $C
if [[ $a == 1000 ]];then
exit 1
fi
reboot
EOF

cat << 'EOF'  > /etc/systemd/system/reboot.service
[Unit]
Description=reboot

[Service]
Type=oneshot
ExecStart=/root/reboot.sh

[Install]
WantedBy=getty.target
EOF

systemctl enable reboot
chmod +x /root/reboot.sh
```

结束重启: 修改 grub 中的 init=/bin/bash
此时如果遇到了磁盘 readonly 的情况，参考这里:
- https://askubuntu.com/questions/197459/how-to-fix-sudo-unable-to-open-read-only-file-system

```sh
mount -o remount,rw /
```

### 如果是检测另一个 sytemd 进程

reboot.sh 中增加
```sh
if [[ $(/bin/systemctl is-active caixukun) == "active" ]];then
reboot
fi
```

在 reboot.service 中增加:
```sh
After=cauxukun.service
```


## oneshot vs simple
https://trstringer.com/simple-vs-oneshot-systemd-service/

## systemd 的 before after 应该是不能实现只有 exit = 0 才可以继续的操作
但是，为什么曾经见过磁盘服务没有启动，然后后面都没有启动的情况

## systemd unit file 的位置
- sys: /etc/systemd/system
- user: /etc/systemd/user or $HOME/.config/systemd/user

## journalctl
- https://www.loggly.com/ultimate-guide/using-journalctl/
- https://unix.stackexchange.com/questions/139513/how-to-clear-journalctl

## 常见疑问
- https://unix.stackexchange.com/questions/506347/why-do-most-systemd-examples-contain-wantedby-multi-user-target

[^1]: https://unix.stackexchange.com/questions/471824/what-is-the-correct-substitute-for-rc-local-in-systemd-instead-of-re-creating-rc
[^2]: https://support.huaweicloud.com/intl/en-us/trouble-ecs/ecs_trouble_0349.html
