## ceph
- https://www.youtube.com/watch?v=c4sgV_FEb4I

## 先看看历史文章
https://hn.algolia.com/?q=ceph

## 常用命令

sudo ceph -s
sudo ceph status



## 基本词汇


- Pool：存储对象的逻辑分区（logical partitions used to store objects），有独立的 resilience/placement-groups/CRUSH-rules/snaphots 管理能力；
- Image: 一个块，类似 LVM 中的一个 logical volume
- PG (placement group): 存储 objects 的副本的基本单位，一个 PG 包含很多 objects，例如 3 副本的话就会有 3 个 PG，存放在三个 OSD 上；

ref : https://arthurchiao.art/blog/storage-advanced-notes-2-zh/

### RBD

RADOS Block Device

### MDS

> [!NOTE]
> 参考 Deepseeek ，有待验证

Metadata Server ，用于文件系统

ceph fs status
```txt
xfs - 0 clients
===
RANK   STATE            MDS           ACTIVITY   DNS    INOS   DIRS   CAPS
 0    creating  cephfs.fedora.aknjry              10     13     12      0
      POOL         TYPE     USED  AVAIL
cephfs.xfs.meta  metadata     0    379G
cephfs.xfs.data    data       0    379G
cephfs - 0 clients
======
RANK   STATE            MDS           ACTIVITY   DNS    INOS   DIRS   CAPS
 0    creating  cephfs.fedora.ceeppm              10     13     12      0
       POOL           TYPE     USED  AVAIL
cephfs.cephfs.meta  metadata     0    379G
cephfs.cephfs.data    data       0    379G
MDS version: ceph version 19.2.2 (0eceb0defba60152a8182f7bd87d164b639885b8) squid (stable)

```

### OSD

可以简单理解为盘了
```txt
root@fedora:/home/martins3# ceph osd status
ID  HOST     USED  AVAIL  WR OPS  WR DATA  RD OPS  RD DATA  STATE
 0  fedora  27.3M   599G      0        0       0        0   exists,up
 1  fedora  26.7M   599G      0        0       0        0   exists,up
```

### PG
放置组

数据分布的逻辑单元，介于 OSD 和对象之间的中间层

## 内核的 ceph 支持

## 先用起来吧
- https://news.ycombinator.com/item?id=26759185

- https://docs.ceph.com/en/octopus/install/ceph-deploy/quick-cephfs/ : 是这样吗?

算了

### ubuntu 的入门教程 : https://ubuntu.com/ceph/what-is-ceph

```txt
sudo snap install microceph
sudo microceph cluster bootstrap
sudo microceph.ceph status
sudo microceph.ceph osd crush rule rm replicated_rule
sudo microceph.ceph osd crush rule create-replicated single default osd

sudo microceph disk add /dev/vda --wipe

sudo microceph.ceph status
sudo microceph.ceph osd status
```


```txt
sudo microceph cluster config set cluster_network 10.0.32.0/16
sudo microceph cluster config get cluster_network
```

- https://ubuntu.com/ceph/install
  - https://github.com/canonical/microceph

## 看看先可不可以部署起来吧

https://docs.ceph.com/en/reef/cephadm/install/#cephadm-deploying-new-cluster


## 还是初级阶段
https://gitee.com/openeuler/fastblock

## 尝试一下
https://www.lvbibir.cn/posts/tech/ceph-v16-cpehadm-openeuler/

## 看看 oem 厂商的 ceph 讲解
https://www.sangfor.com.cn/knowledge/ceph


## 在 Fedora 中尝试使用

sudo yum update --refresh
sudo yum install cephadm

用这个修改默认的 hostname
sudo hostnamectl set-hostname fedora

mkdir -p /etc/ceph
cephadm bootstrap --mon-ip 10.0.147.0

```txt
Ceph Dashboard is now available at:

             URL: https://fedora:8443/
            User: admin
        Password: by5cusr8mu

Enabling client.admin keyring and conf on hosts with "admin" label
Saving cluster configuration to /var/lib/ceph/22eb455e-2e10-11f0-871b-525400123456/config directory
You can access the Ceph CLI as following in case of multi-cluster or non-default config:

        sudo /usr/sbin/cephadm shell --fsid 22eb455e-2e10-11f0-871b-525400123456 -c /etc/ceph/ceph.conf -k /etc/ceph/ceph.client.admin.keyring

Or, if you are only running a single cluster on this host:

        sudo /usr/sbin/cephadm shell

Please consider enabling telemetry to help improve Ceph:

        ceph telemetry on

For more information see:

        https://docs.ceph.com/en/latest/mgr/telemetry/

Bootstrap complete.
```


```txt
martins3@fedora:~$ sudo ceph status
  cluster:
    id:     22eb455e-2e10-11f0-871b-525400123456
    health: HEALTH_WARN
            OSD count 0 < osd_pool_default_size 3

  services:
    mon: 1 daemons, quorum fedora (age 5m)
    mgr: fedora.yloizj(active, since 4m)
    osd: 0 osds: 0 up, 0 in

  data:
    pools:   0 pools, 0 pgs
    objects: 0 objects, 0 B
    usage:   0 B used, 0 B / 0 B avail
    pgs:

  progress:
    Updating prometheus deployment (+1 -> 1) (0s)
      [............................]
```


添加主机:
https://docs.ceph.com/en/latest/cephadm/host-management/#cephadm-adding-hosts

ssh-copy-id -f -i /etc/ceph/ceph.pub root@10.0.150.0
sudo ceph orch host add ceph2 10.0.150.0
sudo ceph orch host label add ceph2 _admin
sudo ceph orch host ls

sudo ceph orch host maintenance enter ceph2
sudo ceph orch host rescan ceph2

添加盘
sudo ceph orch apply osd --all-available-devices



### 尝试挂载 cephfs

```txt

 sudo ceph auth get-key client.admin

echo "$(sudo ceph auth get-key client.admin)" | sudo tee /etc/ceph/admin.secret
sudo chmod 600 /etc/ceph/admin.secret
sudo mount -t ceph 10.0.147.0:6789:/ /mnt/cephfs -o name=admin,secretfile=/etc/ceph/admin.secret
```

```txt
mount error: no mds (Metadata Server) is up. The cluster might be laggy, or you may not be authorized
```

sudo ceph fs status


```txt
sudo ceph config set global osd_pool_default_size 2
sudo ceph config set global osd_pool_default_min_size 1
```
再次失败，完全懵逼了

重启一下，为什么还是失败了

然后从 https://docs.ceph.com/en/latest/cephadm/services/mds/#orchestrator-cli-cephfs

sudo ceph fs volume create foo --placement="label:mds" # 实际测试，在这里永远的卡主

### 尝试使用 rbd


```sh
# 创建存储池
ceph osd pool create rbd_pool 128

# 初始化池为 RBD 使用
rbd pool init rbd_pool

# 创建 10GB 的块设备镜像
sudo rbd create --size 10240 rbd_pool/myimage # 实际上测试，会在这里永远的卡主

# 映射到本地主机
rbd map rbd_pool/myimage

# 格式化并挂载
mkfs.ext4 /dev/rbd0
mount /dev/rbd0 /mnt
```

## 先看看这个吧
https://github.com/wuhongsong/ceph-deep-dive
https://zhuanlan.zhihu.com/c_1267088333848641536

## ceph 10 年
- https://fuis.me/category/%E8%AE%BA%E6%96%87

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
