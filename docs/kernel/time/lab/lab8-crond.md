systemctl status crond

nixos 居然没有，但是 fedora 使用的还是这个服务。

```txt
 ./crontab.c                                                                       1261
 ./entry.c                                                                          751
 ./security.c                                                                       748
 ./cron.c                                                                           742
 ./misc.c                                                                           722
 ./do_command.c                                                                     684
 ./database.c                                                                       682
 ./cronnext.c                                                                       435
 ./env.c                                                                            306
 ./popen.c                                                                          221
 ./user.c                                                                           179
 ./pw_dup.c                                                                         128
 ./job.c                                                                            107
```

https://github.com/cronie-crond/cronie

## 这个不错
https://ittavern.com/cron-jobs-on-linux-comprehensive-guide/

## 看看
```txt
crond(1469)─┬─crond(4484)───bash(4489)───sleep(4491)
            ├─crond(4565)───bash(4570)───sleep(4572)
            ├─crond(4575)───bash(4580)───sleep(4582)
            ├─crond(4585)───bash(4590)───sleep(4592)
            ├─crond(4595)───bash(4600)───sleep(4602)
            ├─crond(4605)───bash(4610)───sleep(4612)
            ├─crond(5217)───bash(5222)───sleep(5224)
            ├─crond(5560)───bash(5565)───sleep(5567)
            ├─crond(5571)───bash(5582)───sleep(5584)
            ├─crond(5588)───bash(5594)───sleep(5596)
            ├─crond(5612)───bash(5617)───sleep(5619)
            ├─crond(5888)───bash(5893)───sleep(5895)
            └─crond(5898)───bash(5903)───sleep(5905)

F S UID          PID    PPID  C PRI  NI ADDR SZ WCHAN  STIME TTY          TIME CMD
5 S root        4484    1469  0  80   0 -  7874 -      11:52 ?        00:00:00 /usr/sbin/CROND -n
4 S root        4489    4484  0  80   0 -  5650 -      11:52 ?        00:00:00 bash /home/martins3/test.sh
0 S root        4491    4489  0  80   0 -  5281 -      11:52 ?        00:00:00 sleep 1000000
5 S root        4565    1469  0  80   0 -  7874 -      11:53 ?        00:00:00 /usr/sbin/CROND -n
4 S root        4570    4565  0  80   0 -  5650 -      11:53 ?        00:00:00 bash /home/martins3/test.sh
0 S root        4572    4570  0  80   0 -  5281 -      11:53 ?        00:00:00 sleep 1000000
5 S root        4575    1469  0  80   0 -  7874 -      11:54 ?        00:00:00 /usr/sbin/CROND -n
4 S root        4580    4575  0  80   0 -  5650 -      11:54 ?        00:00:00 bash /home/martins3/test.sh
0 S root        4582    4580  0  80   0 -  5281 -      11:54 ?        00:00:00 sleep 1000000
5 S root        4585    1469  0  80   0 -  7874 -      11:55 ?        00:00:00 /usr/sbin/CROND -n
4 S root        4590    4585  0  80   0 -  5650 -      11:55 ?        00:00:00 bash /home/martins3/test.sh
0 S root        4592    4590  0  80   0 -  5281 -      11:55 ?        00:00:00 sleep 1000000
5 S root        4595    1469  0  80   0 -  7874 -      11:56 ?        00:00:00 /usr/sbin/CROND -n
4 S root        4600    4595  0  80   0 -  5650 -      11:56 ?        00:00:00 bash /home/martins3/test.sh
0 S root        4602    4600  0  80   0 -  5281 -      11:56 ?        00:00:00 sleep 1000000
5 S root        4605    1469  0  80   0 -  7874 -      11:57 ?        00:00:00 /usr/sbin/CROND -n
4 S root        4610    4605  0  80   0 -  5650 -      11:57 ?        00:00:00 bash /home/martins3/test.sh
0 S root        4612    4610  0  80   0 -  5281 -      11:57 ?        00:00:00 sleep 1000000
1 I root        5196       2  0  80   0 -     0 -      11:57 ?        00:00:00 [kworker/0:0-cgroup_destroy]
5 S root        5217    1469  0  80   0 -  7874 -      11:58 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5222    5217  0  80   0 -  5650 -      11:58 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5224    5222  0  80   0 -  5281 -      11:58 ?        00:00:00 sleep 1000000
5 S root        5560    1469  0  80   0 -  7874 -      11:59 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5565    5560  0  80   0 -  5650 -      11:59 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5567    5565  0  80   0 -  5281 -      11:59 ?        00:00:00 sleep 1000000
5 S root        5571    1469  0  80   0 -  7874 -      12:00 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5582    5571  0  80   0 -  5650 -      12:00 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5584    5582  0  80   0 -  5281 -      12:00 ?        00:00:00 sleep 1000000
5 S root        5588    1469  0  80   0 -  7874 -      12:01 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5594    5588  0  80   0 -  5650 -      12:01 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5596    5594  0  80   0 -  5281 -      12:01 ?        00:00:00 sleep 1000000
1 S root        5607       1  0  80   0 -  5567 -      12:01 ?        00:00:00 /usr/sbin/anacron -s
5 S root        5612    1469  0  80   0 -  7874 -      12:02 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5617    5612  0  80   0 -  5650 -      12:02 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5619    5617  0  80   0 -  5281 -      12:02 ?        00:00:00 sleep 1000000
1 I root        5886       2  0  80   0 -     0 -      12:02 ?        00:00:00 [kworker/u4:0-events_unbound]
5 S root        5888    1469  0  80   0 -  7874 -      12:03 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5893    5888  0  80   0 -  5650 -      12:03 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5895    5893  0  80   0 -  5281 -      12:03 ?        00:00:00 sleep 1000000
5 S root        5898    1469  0  80   0 -  7874 -      12:04 ?        00:00:00 /usr/sbin/CROND -n
4 S root        5903    5898  0  80   0 -  5650 -      12:04 ?        00:00:00 bash /home/martins3/test.sh
0 S root        5905    5903  0  80   0 -  5281 -      12:04 ?        00:00:00 sleep 1000000
4 S root        5906    1435  0  80   0 -  4494 -      12:04 ?        00:00:00 sshd: martins3 [priv]
5 S martins3    5916    5906  0  80   0 -  4534 -      12:04 ?        00:00:00 sshd: martins3@pts/0
0 S martins3    5921    5916  0  80   0 -  6603 wait_w 12:04 pts/0    00:00:00 -zsh
1 I root        6150       2  0  80   0 -     0 -      12:04 ?        00:00:00 [kworker/u4:1]
1 I root        6152       2  0  80   0 -     0 -      12:04 ?        00:00:00 [kworker/0:1-cgroup_destroy]
5 S root        6169    1469  0  80   0 -  7874 -      12:05 ?        00:00:00 /usr/sbin/CROND -n
4 S root        6174    6169  0  80   0 -  5650 -      12:05 ?        00:00:00 bash /home/martins3/test.sh
0 S root        6176    6174  0  80   0 -  5281 -      12:05 ?        00:00:00 sleep 1000000
```
## 为什么这样的

发现这些都是没有 child
```txt
crash> ps -c 5571
PID: 5571   TASK: ffff99d40fb8df00  CPU: 9   COMMAND: "crond"
  (no children)
```

作为对比

```txt
crash> ps  2888 -c
PID: 2888   TASK: ffff945a49c64740  CPU: 0   COMMAND: "crond"
  PID: 2889   TASK: ffff945a4ded2f80  CPU: 0   COMMAND: "test.sh"
```

## 执行的方法
crontab -e
```txt
* * * * * /root/test.sh
```

chmod +x /root/test.sh
```sh
echo "Task executed at $(date)" >> /tmp/log.txt
sleep 1000000
```

内部环境中:
```txt
crash> ps -a automount   3273
PID: 3273   TASK: ffff945a48bedf00  CPU: 0   COMMAND: "crond"
ARG: /usr/sbin/CROND -n
ENV: LANG=en_US.UTF-8
     PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin
     CRONDARGS=
```

应该还可以获取到 stack 才对的

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
