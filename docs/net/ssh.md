# ssh
## 允许
```txt
# 允许
sudo sed -i "s/PermitRootLogin prohibit-password/PermitRootLogin yes/" /etc/ssh/sshd_config
# 不允许
sudo sed -i "s/PermitRootLogin yes/PermitRootLogin prohibit-password/" /etc/ssh/sshd_config
sudo systemctl restart sshd
```

ssh 的源码:

https://github.com/openssh/openssh-portable

## ssh keys 是必须一个机器一个么?
不需要


## tcp forward

sudo vim /etc/ssh/sshd_config
AllowTcpForwarding yes
sudo systemctl restart sshd


https://superuser.com/questions/667040/ssh-port-forwarding

简单看了下，原理很简单，就是让 server 可以访问到本地。

## 这个东西好，就是没有 windows 的实现
https://github.com/Adembc/lazyssh

## 有趣的
https://news.ycombinator.com/item?id=46723990

但是我自己的脚本也不错
https://github.com/Gu1llaum-3/sshm

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
