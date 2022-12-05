# iscsi

- https://linux.vbird.org/linux_server/centos6/0460iscsi.php

## 基本原理
- https://wiki.nix-pro.com/view/ISCSI

- target : server

## 使用

目前找到的最好操作方法:
- https://www.cnblogs.com/xiangsikai/p/10876534.html

```sh
vim /etc/iscsi/initiatorname.iscsi
```

将原来的数值替换为:
iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d:xsk

# https://github.com/open-iscsi/open-iscsi
