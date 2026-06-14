# samba 配置简单记录

总体来说，用起来挺简单的，不过需要注意，nfs

## linux 作为 samba server

### linux 配置
nix 参考: https://gist.github.com/vy-let/a030c1079f09ecae4135aebf1e121ea6

下面仅仅分析 fedora 作为 server

将 fedora 的文件共享给 windows

```sh
sudo dnf install samba
sudo systemctl enable smb --now
sudo systemctl status smb
```

sudo smbpasswd -a martins3

在 /etc/samba/smb.conf 的结尾地方添加:
```txt
[public]
        path = /home/martins3/data/
        browseable = yes
        read only = no
        guest ok = yes
```

### windows 中的配置
在 windows 虚拟机中，打开文件浏览器, 右键 `网络`，选择 `映射网络驱动器`，
在文件夹中填写路径 `\\10.0.0.2\public` 即可。
注意，这里的 public 和配置文件中对应的，也就是 `[public]` 的配置。

如果遇到需要密码的时候，但是密码不对

```txt
sudo smbpasswd -a martins3
```

在 windows 那一侧使用 martins3 和新设置的密码来登录。

### 性能测试
让从 windows 拷贝大文件到 linux 上

可以观测到的 thread:
```txt
4 S root        4150       1  0  80   0 - 11593 -      Sep14 ?        00:00:00 /nix/store/kj8qz24gbr2hz5my4k39gzzv85ys2lk3-samba-4.20.1/sbin/smbd --foreground --no-process-group
5 S root        4259    4150  0  80   0 - 11060 -      Sep14 ?        00:00:00 smbd: notifyd                                                     .
1 S root        4260    4150  0  80   0 - 11064 -      Sep14 ?        00:00:00 smbd: cleanupd                                                    .
5 S martins3 2123125    4150  0  80   0 - 21147 -      Sep23 ?        00:00:10 smbd: client [10.0.0.8]
```

```txt
🧀  pstree 4150 -p
smbd(4150)─┬─smbd-cleanupd(4260)
           ├─smbd-notifyd(4259)
           └─smbd[10.0.0.8](2123125)─┬─{smbd[10.0.0.8]}(2720888)
                                     ├─{smbd[10.0.0.8]}(2720889)
                                     ├─{smbd[10.0.0.8]}(2725297)
                                     ├─{smbd[10.0.0.8]}(2726043)
                                     ├─{smbd[10.0.0.8]}(2726044)
                                     ├─{smbd[10.0.0.8]}(2726045)
                                     ├─{smbd[10.0.0.8]}(2727630)
                                     └─{smbd[10.0.0.8]}(2727631)
```



对于 `smbd: client [10.0.0.8]` 进行 perf ，可以看到:

```txt
sudo perf record --pid 2123125 -g -- sleep 60
```

看来完全走的用户态，没有使用内核态 smb server :
```txt
-   67.04%     0.00%  smbd[10.0.0.8]  [k] entry_SYSCALL_64_after_hwframe                                                                                                                       ◆
     entry_SYSCALL_64_after_hwframe                                                                                                                                                            ▒
   - do_syscall_64                                                                                                                                                                             ▒
      - 33.74% __sys_recvmsg                                                                                                                                                                   ▒
         - 32.99% ___sys_recvmsg                                                                                                                                                               ▒
            - 32.77% ____sys_recvmsg                                                                                                                                                           ▒
               - sock_recvmsg                                                                                                                                                                  ▒
                  - 32.39% inet_recvmsg                                                                                                                                                        ▒
                     - tcp_recvmsg                                                                                                                                                             ▒
                        - 32.28% tcp_recvmsg_locked                                                                                                                                            ▒
                           - 22.16% skb_copy_datagram_iter                                                                                                                                     ▒
                              - __skb_datagram_iter                                                                                                                                            ▒
                                   10.58% _copy_to_iter                                                                                                                                        ▒
                                 - 9.90% __skb_datagram_iter                                                                                                                                   ▒
                                      8.88% _copy_to_iter                                                                                                                                      ▒
                                    - 0.60% simple_copy_to_iter                                                                                                                                ▒
                                         __check_object_size                                                                                                                                   ▒
                                 - 0.87% simple_copy_to_iter                                                                                                                                   ▒
                                      __check_object_size                                                                                                                                      ▒
                           - 9.13% __tcp_transmit_skb                                                                                                                                          ▒
                              - __ip_queue_xmit                                                                                                                                                ▒
                                 - 5.13% ip_finish_output2                                                                                                                                     ▒
                                    - __dev_queue_xmit                                                                                                                                         ▒
                                       - 4.80% dev_hard_start_xmit                                                                                                                             ▒
                                          - internal_dev_xmit                                                                                                                                  ▒
                                             - ovs_vport_receive                                                                                                                               ▒
                                                - 4.12% ovs_dp_process_packet                                                                                                                  ▒
                                                   - 3.22% ovs_execute_actions                                                                                                                 ▒
                                                      - do_execute_actions                                                                                                                     ▒
                                                         - 2.89% __dev_queue_xmit                                                                                                              ▒
                                                            - 2.35% sch_direct_xmit                                                                                                            ▒
                                                               - 1.26% dev_hard_start_xmit                                                                                                     ▒
                                                                  - 1.21% rtl8169_start_xmit                                                                                                   ▒
                                                                       0.57% rtl8169_tx_map                                                                                                    ▒
                                                                       0.54% __skb_pad                                                                                                         ▒
                                                                 0.88% _raw_spin_lock                                                                                                          ▒
                                                   - 0.76% ovs_flow_tbl_lookup_stats                                                                                                           ▒
                                                      - flow_lookup.isra.0                                                                                                                     ▒
                                                        masked_flow_lookup                                                                                                                     ▒
                                                  0.50% ovs_flow_key_extract                                                                                                                   ▒
                                 - 1.64% ip_local_out                                                                                                                                          ▒
                                    - __ip_local_out                                                                                                                                           ▒
                                       - 1.22% nf_hook_slow                                                                                                                                    ▒
                                            0.99% nf_conntrack_update                                                                                                                          ▒
                                 - 1.23% ip_finish_output                                                                                                                                      ▒
                                    - __cgroup_bpf_run_filter_skb                                                                                                                              ▒
                                         __bpf_prog_run_save_cb
      - 26.12% __x64_sys_pwrite64                                                                                                                                                              ▒
         - vfs_write                                                                                                                                                                           ▒
            - 26.01% ext4_buffered_write_iter                                                                                                                                                  ▒
               - 25.78% generic_perform_write                                                                                                                                                  ▒
                  - 11.14% ext4_da_write_begin                                                                                                                                                 ▒
                     - 5.73% __filemap_get_folio                                                                                                                                               ▒
                        - 3.00% filemap_add_folio                                                                                                                                              ▒
                             1.37% __filemap_add_folio                                                                                                                                         ▒
                             0.83% __mem_cgroup_charge                                                                                                                                         ▒
                           - 0.69% folio_add_lru                                                                                                                                               ▒
                              - folio_batch_move_lru                                                                                                                                           ▒
                                lru_add_fn                                                                                                                                                     ▒
                        - 1.89% folio_alloc_noprof                                                                                                                                             ▒
                           - alloc_pages_mpol_noprof                                                                                                                                           ▒
                              - 1.52% __alloc_pages_noprof                                                                                                                                     ▒
                                   0.83% get_page_from_freelist                                                                                                                                ▒
                          0.54% filemap_get_entry                                                                                                                                              ▒
                     - 5.15% ext4_block_write_begin                                                                                                                                            ▒
                        - 2.66% ext4_da_get_block_prep                                                                                                                                         ▒
                           - 0.85% ext4_da_reserve_space                                                                                                                                       ▒
                                0.51% __dquot_alloc_space                                                                                                                                      ▒
                             0.76% ext4_es_insert_delayed_block                                                                                                                                ▒
                        - 2.33% create_empty_buffers                                                                                                                                           ▒
                           - 1.38% folio_alloc_buffers                                                                                                                                         ▒
                              - 1.23% alloc_buffer_head                                                                                                                                        ▒
                                   kmem_cache_alloc_noprof                                                                                                                                     ▒
                    10.92% copy_page_from_iter_atomic                                                                                                                                          ▒
                  - 2.10% ext4_da_write_end                                                                                                                                                    ▒
                     - 1.96% block_write_end                                                                                                                                                   ▒
                        - __block_commit_write                                                                                                                                                 ▒
                           - 1.11% mark_buffer_dirty                                                                                                                                           ▒
                                0.84% __folio_mark_dirty                                                                                                                                       ▒
                  - 1.21% fault_in_iov_iter_readable                                                                                                                                           ▒
                       fault_in_readable                                                                                                                                                       ▒
      - 4.95% __x64_sys_epoll_wait                                                                                                                                                             ▒
         - 4.78% do_epoll_wait                                                                                                                                                                 ▒
            - 3.43% schedule_hrtimeout_range_clock                                                                                                                                             ▒
               - 3.09% schedule                                                                                                                                                                ▒
                  - __schedule
                     - 1.25% dequeue_task_fair                                                                                                                                                 ▒
                        - dequeue_entity                                                                                                                                                       ▒
                             0.52% update_curr                                                                                                                                                 ▒
              0.53% ep_item_poll.isra.0                                                                                                                                                        ▒
        0.67% syscall_exit_to_user_mode                                                                                                                                                        ▒
      - 0.58% __sys_sendmsg                                                                                                                                                                    ▒
           ___sys_sendmsg
```

这么看，smbd 完全是用户态的啊！但是我记得 kernel 中是有这些东西的啊!

## windows 作为 samba server

### windows 配置
创建目录
New-Item -Path "C:\Share" -ItemType Directory -Force

然后在图形界面中，右键配置共享

在 linux 中执行如下即可:

```txt
smbclient -L //10.0.0.8/ -U "martins3\97936"
```

如果可以看到如下的结果，那么说明是可以联通的:
```txt
Password for [MARTINS3\97936]:

        Sharename       Type      Comment
        ---------       ----      -------
        ADMIN$          Disk      远程管理
        C$              Disk      默认共享
        IPC$            IPC       远程 IPC
        Share           Disk
```

手动连接:
```txt
sudo mount -t cifs //10.0.0.8/Share mnt \
    -o username=97936,password=1@3456aB,domain=MARTINS3,vers=3.0,iocharset=utf8
```

### client 配置

#### 不使用密码
sudo mkdir -p /etc/samba
sudo vim /etc/samba/credentials_martins3

```txt
username=97936
password=你的实际密码
domain=MARTINS3
```

sudo chmod 600 /etc/samba/credentials_martins3
sudo chown root:root /etc/samba/credentials_martins3


sudo mount -t cifs //10.0.0.8/Share mnt \
    -o credentials=/etc/samba/credentials_martins3,vers=3.0,iocharset=utf8,uid=$(id -u),gid=$(id -g)

#### 自动 mount
最后添加到 /etc/fstab 中如下内容:
```txt
//10.0.0.8/Share  /home/martins3/hack cifs  credentials=/etc/samba/credentials_martins3,vers=3.0,iocharset=utf8,uid=1000,gid=1000,file_mode=0755,dir_mode=0755,nofail  0  0
```
执行
sudo mount -a

### 基本观察

可以看看 linux 上的 Samba 的源码是哪里的
```txt
  ├─samba-smbd.service
  │ ├─  4169 /nix/store/mcyfbsbxr2hjr53v2yyvqnsnlg894xk2-samba-4.20.1/sbin/smbd
  │ ├─  4229 smbd: notifyd
  │ ├─  4231 smbd: cleanupd
  │ └─760872 smbd: client [10.0.0.8]
```

### 性能测试

```txt
[global]
time_based=1
runtime=1000
ioengine=io_uring
iodepth=1024
numjobs=2
bs=4k

[trash]
rw=randread
filename=/home/martins3/mnt/a.dump
size=10G
```

```txt
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=1024
...
fio-3.39
Starting 2 processes
trash: Laying out IO file (1 file / 10240MiB)
q^Cs: 2 (f=2): [r(2)][28.7%][r=84.1MiB/s][r=21.5k IOPS][eta 11m:54s]
```

考虑到网络只有 1Gb ，所以其实还是还不错的。

### 问题

#### 在映射的目录中，linux 无法构建软连接

```txt
ln: failed to create symbolic link '/home/martins3/hack/vm/fedora/iso': Operation not permitted
```

#### 在映射的目录中，linux 无法构建unix domain socket

这些报错都是完全看不懂的
```txt
🧀   nc -l -U a.sock
nc: Permission denied

 sudo nc -l -U a.sock
[sudo] password for martins3:
Ncat: bind to a.sock: Permission denied. QUITTING.
```

#### 有时候遇到这个错误
```txt
[Sun Sep 21 00:17:16 2025] CIFS: VFS: \\10.0.0.8 has not responded in 180 seconds. Reconnecting...
[Sun Sep 21 09:38:10 2025] CIFS: VFS: \\10.0.0.8 has not responded in 180 seconds. Reconnecting...
[Sun Sep 21 10:00:51 2025] CIFS: VFS: Autodisabling the use of server inode numbers on \\10.0.0.8\Share
[Sun Sep 21 10:00:51 2025] CIFS: VFS: The server doesn't seem to support them properly or the files might be on different servers (DFS)
[Sun Sep 21 10:00:51 2025] CIFS: VFS: Hardlinks will not be recognized on this mount. Consider mounting with the "noserverino" option to silence this message.
```

#### 如果 linux 虚拟机和 client 断开之后，那么就会遇到这个错误
```txt
[2218377.222530] CIFS: VFS: \\10.0.0.8 has not responded in 180 seconds. Reconnecting...
```

## 资源
https://github.com/samba-team/samba

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
