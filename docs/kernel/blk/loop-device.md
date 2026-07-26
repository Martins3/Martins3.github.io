# loop device

## 基本使用

测试 img
```sh
dd if=/dev/null of=x.ext4 bs=1000M seek=100
mkfs.ext4 -F x.ext4
mkdir -p tmp
sudo mount -t ext4 -o loop x.ext4 tmp
sudo chown martins3 tmp
```

loop 定位就是一个普通的 block 设备，类似其他的所有的 block 设备一样，也会其特殊的属性:
```txt
# cd /sys/block/loop0/loop

# grep . *
autoclear:1
backing_file:/home/martins3/x.ext4
dio:0
offset:0
partscan:0
sizelimit:0
```

每次做  sudo mount virtio-win.iso tmp2 的操作，都会自动的创建
出来 loop device ，应该是 mount 把工作都完成了:

```txt
🧀  ls -la /dev/loop*
crw-rw---- 10,237 root  1 Jul 10:32 /dev/loop-control
brw-rw----    7,0 root  3 Jul 17:44 /dev/loop0
brw-rw----    7,1 root  3 Jul 17:48 /dev/loop1
brw-rw----    7,2 root  3 Jul 17:49 /dev/loop2
```

```txt
losetup -a
# 等价于
cat /sys/block/loop*/loop/backing_file
```

## 基本代码流程

loop 就是让 mount 一个文件作为盘的，就像是 swap file 一样

```c
static const struct blk_mq_ops loop_mq_ops = {
	.queue_rq       = loop_queue_rq,
	.complete	= lo_complete_rq,
};
```

loop 设备上运行一个 ext4 的时候:

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_mount
        - __se_sys_mount
          - __do_sys_mount
            - do_mount
              - path_mount
                - do_new_mount
                  - vfs_get_tree
                    - get_tree_bdev
                      - ext4_fill_super
                        - __ext4_fill_super
                          - ext4_load_super
                            - ext4_sb_bread_unmovable
                              - __ext4_sb_bread_gfp
                                - ext4_read_bh
                                  - __ext4_read_bh
                                    - submit_bio_noacct_nocheck
                                      - submit_bio_noacct_nocheck
                                        - __submit_bio_noacct_mq
                                          - __submit_bio
                                            - blk_mq_submit_bio
                                              - blk_mq_try_issue_directly
                                                - __blk_mq_try_issue_directly
                                                  - __blk_mq_issue_directly
                                                    - loop_queue_rq

如果 loop 上运行一个 iso 的时候:
```txt
@[
        loop_queue_rq+5
        __blk_mq_issue_directly+68
        blk_mq_issue_direct+137
        blk_mq_dispatch_queue_requests+332
        blk_mq_flush_plug_list+136
        __blk_flush_plug+274
        __submit_bio+412
        submit_bio_noacct_nocheck+255
        __bread_gfp+125
        do_isofs_readdir+719
        isofs_readdir+86
        iterate_dir+187
        __x64_sys_getdents64+136
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 212
```

- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - loop_process_work
          - loop_handle_cmd
            - do_req_filebacked
              - lo_write_simple
                - lo_write_bvec

## 细节问题

### loop device 的容量

#### 两个获取方法
- blockdev --getsize64 /dev/loop0
    - 打开块设备并调用 BLKGETSIZE64 ioctl。
    - 内核返回 bdev_nr_bytes(bdev)，单位是字节。
    - 实现在 blkdev_ioctl 中的 BLKGETSIZE64

- lsblk -bno SIZE /dev/loop0
    - 主要读取 sysfs： /sys/class/block/loop0/size

```c
ssize_t part_size_show(struct device *dev,
		       struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%llu\n", bdev_nr_sectors(dev_to_bdev(dev)));
}
```

#### 容量变化不是自动的检测的

如果增大了 backing_file 的大小，需要手动的配置
```txt
  sudo losetup --set-capacity /dev/loop0
  sudo losetup -c /dev/loop0
```

它触发 LOOP_SET_CAPACITY ioctl

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
