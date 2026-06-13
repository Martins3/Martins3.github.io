# 文件系统功能分类

## 可以参考的资料
1. bcachefs 的分析


## 收集

| copy on write |
| read-only on io error |
| 压缩|
| 快照|

## read-only on io error

```txt
[ 1949.245253] Aborting journal on device vda2-8.
[ 1949.245487] I/O error, dev vda, sector 89368576 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245487] EXT4-fs error (device vda2): ext4_journal_check_start:84: comm zsh: Detected aborted journal
[ 1949.245891] I/O error, dev vda, sector 7917976 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245894] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989747)
[ 1949.245903] Buffer I/O error on device vda2, logical block 861491
[ 1949.245909] I/O error, dev vda, sector 7916488 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245910] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989561)
[ 1949.245912] Buffer I/O error on device vda2, logical block 861305
[ 1949.245914] I/O error, dev vda, sector 7915656 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245915] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989457)
[ 1949.245916] Buffer I/O error on device vda2, logical block 861201
[ 1949.245917] I/O error, dev vda, sector 7914000 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245919] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989250)
[ 1949.245920] Buffer I/O error on device vda2, logical block 860994
[ 1949.245921] I/O error, dev vda, sector 7913520 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245922] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989190)
[ 1949.245923] Buffer I/O error on device vda2, logical block 860934
[ 1949.245925] I/O error, dev vda, sector 7913224 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245926] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989153)
[ 1949.245927] Buffer I/O error on device vda2, logical block 860897
[ 1949.245928] I/O error, dev vda, sector 7912928 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245929] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 989116)
[ 1949.245930] Buffer I/O error on device vda2, logical block 860860
[ 1949.245932] I/O error, dev vda, sector 7909216 op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2
[ 1949.245933] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 988652)
[ 1949.245934] Buffer I/O error on device vda2, logical block 860396
[ 1949.245935] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 988640)
[ 1949.245936] Buffer I/O error on device vda2, logical block 860384
[ 1949.245938] EXT4-fs warning (device vda2): ext4_end_bio:343: I/O error 10 writing to inode 4063549 starting block 986359)
[ 1949.245939] Buffer I/O error on device vda2, logical block 858103
[ 1949.245974] EXT4-fs error (device vda2): ext4_journal_check_start:84: comm journal-offline: Detected aborted journal
[ 1949.246061] Buffer I/O error on dev vda2, logical block 11042816, lost sync page write
[ 1949.246085] JBD2: I/O error when updating journal superblock for vda2-8.
[ 1949.256645] Buffer I/O error on dev vda2, logical block 0, lost sync page write
[ 1949.256971] EXT4-fs (vda2): I/O error while writing superblock
[ 1949.256976] EXT4-fs (vda2): previous I/O error to superblock detected
[ 1949.257188] EXT4-fs (vda2): Remounting filesystem read-only
[ 1949.257972] Buffer I/O error on dev vda2, logical block 0, lost sync page write
[ 1949.258361] EXT4-fs (vda2): I/O error while writing superblock
```

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
