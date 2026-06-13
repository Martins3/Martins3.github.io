研究下 scsi log 是不是放到一个特殊的位置上去了，还是存在单独的含义吗?

执行这个命令，将会看到 scsi 很多内容的
```sh
scsi_logging_level -s -a 7
```

内核中可以使用这个来显示各种 cmd 的内容是什么
```c
scsi_print_command(scmd);
```

基本的使用原则: https://www.ibm.com/docs/en/linux-on-z?topic=commands-scsi-logging-level

这个 log 是表示或的意思吗?
```txt
	SCSI_LOG_ERROR_RECOVERY(1, scsi_eh_prt_fail_stats(shost, &eh_work_q));
```

scsi_logging_level -s -E 7
```txt
[  337.080510] [martins3:scsi_unjam_host:2267]
[  337.080712] scsi host1: scsi_eh_prt_fail_stats: cmds failed: 0, cancel: 1
[  337.081052] scsi host1: Total of 1 commands on 1 devices require eh work
[  337.081370] [martins3:scsi_unjam_host:2269]
```

不知道为什么 scsi_logging_level -s -E 1 的时候，没有输出，因为希望是大于 1 的

## 实现原理
通过读写: /proc/sys/dev/scsi/logging_level

```txt
        -E, --error      specify SCSI_LOG_ERROR
        -T, --timeout    specify SCSI_LOG_TIMEOUT
        -S, --scan       specify SCSI_LOG_SCAN
        -M, --midlevel   specify SCSI_LOG_MLQUEUE and SCSI_LOG_MLCOMPLETE
            --mlqueue    specify SCSI_LOG_MLQUEUE
            --mlcomplete specify SCSI_LOG_MLCOMPLETE
        -L, --lowlevel   specify SCSI_LOG_LLQUEUE and SCSI_LOG_LLCOMPLETE
            --llqueue    specify SCSI_LOG_LLQUEUE
            --llcomplete specify SCSI_LOG_LLCOMPLETE
        -H, --highlevel  specify SCSI_LOG_HLQUEUE and SCSI_LOG_HLCOMPLETE
            --hlqueue    specify SCSI_LOG_HLQUEUE
            --hlcomplete specify SCSI_LOG_HLCOMPLETE
        -I, --ioctl      specify SCSI_LOG_IOCTL
```


## scmd_printk scsi_print_command

```txt
[   20.233480] sd 1:0:0:0: [sdc] tag#31 sd_setup_read_write_cmnd: block=0, count=8
```
展示 disk_name 和 mq 的 tag

```txt
sd 1:0:0:0: [sdc] tag#31 CDB: Read(10) 28 00 00 00 00 00 00 00 08 00
```

## scsi_print_command 的输出结果

```txt
sd 0:0:0:0: [sda] tag#73 CDB: Read(10) 28 00 00 00 00 00 00 00 08 00
```

## 如何理解这个输出的含义

```txt
[868824.456534] sd 0:0:1:0: [sg3] tag#3125 finish aborted command
[912520.340079] sd 0:0:1:0: [sg3] tag#3124 abort scheduled
[912520.351039] sd 0:0:1:0: [sg3] tag#3124 aborting command
[912520.351385] sd 0:0:1:0: attempting task abort! scmd(0x00000000a0ace467), outstanding for 6200 ms & timeout 5000 ms
[912520.351737] sd 0:0:1:0: [sg3] tag#3124 CDB: Inquiry 12 00 00 00 ff 00
```

这里的 sg3 是来自于 `scmd_name(scmd)` ，也就是 gendisk::disk_name ，但是为什么是 sg3 ，应该只是 legacy kernel 的问题吧！

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
