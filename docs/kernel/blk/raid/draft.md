## 出现了问题

很奇怪的问题了:
```txt
[   11.353794] [martins3:lookup_bdev:1201] /dev/vda1
[   11.355044] block device autoloading is deprecated and will be removed.
[   11.685636] md: kicking non-fresh nvme0n1p3 from array!
[   11.719595] md127: WARNING: nvme0n1p1 appears to be on the same physical disk as nvme0n1p4.
[   11.720553] md127: WARNING: nvme0n1p2 appears to be on the same physical disk as nvme0n1p1.
[   11.721443] md127: WARNING: nvme0n1p2 appears to be on the same physical disk as nvme0n1p4.
[   11.722420] md127: WARNING: nvme1n1p1 appears to be on the same physical disk as nvme1n1p2.
[   11.723372] md127: WARNING: nvme1n1p1 appears to be on the same physical disk as nvme1n1p3.
[   11.724234] md127: WARNING: nvme1n1p2 appears to be on the same physical disk as nvme1n1p3.
[   11.725090] True protection against single-disk failure might be compromised.
[   11.726761] md/raid1:md127: active with 2 out of 2 mirrors
[   11.729545] md127: detected capacity change from 0 to 2093056
[   12.001985] [martins3:lookup_bdev:1201] /dev/mapper/openeuler-home
```


```txt
🧀  sudo mdadm --examine /dev/nvme0n1p3
/dev/nvme0n1p3:
          Magic : a92b4efc
        Version : 1.2
    Feature Map : 0x1
     Array UUID : 477c9626:1fa305d3:9c5a8c21:f40a309a
           Name : localhost.localdomain:1  (local to host localhost.localdomain)
  Creation Time : Sun Jul 27 15:29:39 2025
     Raid Level : raid1
   Raid Devices : 2

 Avail Dev Size : 2095104 sectors (1023.00 MiB 1072.69 MB)
     Array Size : 1046528 KiB (1022.00 MiB 1071.64 MB)
  Used Dev Size : 2093056 sectors (1022.00 MiB 1071.64 MB)
    Data Offset : 2048 sectors
   Super Offset : 8 sectors
   Unused Space : before=1968 sectors, after=2048 sectors
          State : clean
    Device UUID : 6e2f63e8:a00cfa3c:d7379e05:5becec69

Internal Bitmap : 8 sectors from superblock
    Update Time : Sun Jul 27 17:06:13 2025
  Bad Block Log : 512 entries available at offset 16 sectors
       Checksum : d6fdf688 - correct
         Events : 181


   Device Role : Active device 0
   Array State : AA ('A' == active, '.' == missing, 'R' == replacing)
```
他的 update 时间至少是一个正常数值

而此时挂上去的正常盘发是的 update 反而不太正常，1970 年了:
```txt
sudo mdadm --examine /dev/nvme1n1p3
/dev/nvme1n1p3:
          Magic : a92b4efc
        Version : 1.2
    Feature Map : 0x1
     Array UUID : 477c9626:1fa305d3:9c5a8c21:f40a309a
           Name : localhost.localdomain:1  (local to host localhost.localdomain)
  Creation Time : Sun Jul 27 15:29:39 2025
     Raid Level : raid1
   Raid Devices : 2

 Avail Dev Size : 2095104 sectors (1023.00 MiB 1072.69 MB)
     Array Size : 1046528 KiB (1022.00 MiB 1071.64 MB)
  Used Dev Size : 2093056 sectors (1022.00 MiB 1071.64 MB)
    Data Offset : 2048 sectors
   Super Offset : 8 sectors
   Unused Space : before=1968 sectors, after=2048 sectors
          State : clean
    Device UUID : 78a73beb:4965d7fd:9e1329fe:c30ca54d

Internal Bitmap : 8 sectors from superblock
    Update Time : Thu Jan  1 08:00:11 1970
  Bad Block Log : 512 entries available at offset 16 sectors
       Checksum : e70d7cf - correct
         Events : 269


   Device Role : Active device 1
   Array State : AA ('A' == active, '.' == missing, 'R' == replacing)
```

lrwxrwxrwx   - root 31 Jul 14:13 rd0 -> dev-nvme0n1p1
lrwxrwxrwx   - root 31 Jul 14:13 rd1 -> dev-nvme1n1p3

至少，这个 update time 是不正常的。

不过的确有一个问题，raid 是如何知道自己控制那些盘的?


的确有这个问题，这样的话，事情就很奇怪了，不是每次都落盘了吗?
```txt
bash ~/aa.sh
   Device Role : Active device 0
    Update Time : Thu Jan  1 08:00:11 1970
         Events : 26
   Device Role : Active device 1
    Update Time : Thu Jan  1 08:00:11 1970
         Events : 26
```
也许只是小插曲吧

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
