## dmesg 的基本使用

网上的各种回答基本都是误导的，
https://stackoverflow.com/questions/28936199/why-is-pr-debug-of-the-linux-kernel-not-giving-any-output
只需要看这两个 kernel 的文档就可以了

- https://www.kernel.org/doc/html/latest/core-api/printk-basics.html
- https://www.kernel.org/doc/html/latest/admin-guide/dynamic-debug-howto.html
- https://www.kernel.org/doc/html/latest/core-api/printk-formats.html

### printk 是 console level 而不是 dmesg 中控制的 level

文档中说明的是:
```txt
cat /proc/sys/kernel/printk
4        4        1        7
```
The result shows the `current`, `default`, `minimum` and `boot-time-default` log levels.

只有在 console 中展示的 dmesg :
echo 8 | sudo tee /proc/sys/kernel/printk 可以，那么 KERN_DEBUG 才可以展示
```c
	printk(KERN_DEBUG "[[printk debug]]\n");
```
```txt
[  194.027022] [[printk debug]]
```

而 dmesg 中，这个输出总是有的。也就是 dmesg 中的输出根本不受  /proc/sys/kernel/printk 的影响，
总是有的。dmesg 只会收到 dynamic_debug 的影响

### 而 console 会同时受到 dynamic_debug 和 /proc/sys/kernel/printk 的影响

## 配置一下启动参数
```txt
	loglevel=	[KNL,EARLY]
			All Kernel Messages with a loglevel smaller than the
			console loglevel will be printed to the console. It can
			also be changed with klogd or other programs. The
			loglevels are defined as follows:

			0 (KERN_EMERG)		system is unusable
			1 (KERN_ALERT)		action must be taken immediately
			2 (KERN_CRIT)		critical conditions
			3 (KERN_ERR)		error conditions
			4 (KERN_WARNING)	warning conditions
			5 (KERN_NOTICE)		normal but significant condition
			6 (KERN_INFO)		informational
			7 (KERN_DEBUG)		debug-level messages

	log_buf_len=n[KMG] [KNL,EARLY]
			Sets the size of the printk ring buffer, in bytes.
			n must be a power of two and greater than the
			minimal size. The minimal size is defined by
			LOG_BUF_SHIFT kernel config parameter. There
			is also CONFIG_LOG_CPU_MAX_BUF_SHIFT config
			parameter that allows to increase the default size
			depending on the number of CPUs. See init/Kconfig
			for more details.

```
以为可以影响 /proc/sys/kernel/printk ，但是其实并不可以的。

## 这里几个参数是做什么的
/sys/module/printk/parameters

在这里有两个函数: kernel/printk/sysctl.c

## 类似这里的调试接口，什么时候可以打开?

dev_dbg

```c
 */
static void virtscsi_complete_cmd(struct virtio_scsi *vscsi, void *buf)
{
	struct virtio_scsi_cmd *cmd = buf;
	struct scsi_cmnd *sc = cmd->sc;
	struct virtio_scsi_cmd_resp *resp = &cmd->resp.cmd;
	struct virtio_scsi_target_state *tgt =
				scsi_target(sc->device)->hostdata;

	dev_dbg(&sc->device->sdev_gendev,
		"cmd %p response %u status %#02x sense_len %u\n",
		sc, resp->response, resp->status, resp->sense_len);
```

## dynamic_debug
支持 module 加载时和内核启动的时候配置

其他细节在 code/trace/dynamic.sh 中

1. /proc/dynamic_debug/control 和 /sys/kernel/debug/dynamic_debug/control 是什么关系

### 这个东西是做什么的
include/net/net_debug.h 中偶然发现了这个东西
```c
#if defined(CONFIG_DYNAMIC_DEBUG) || \
	(defined(CONFIG_DYNAMIC_DEBUG_CORE) && defined(DYNAMIC_DEBUG_MODULE))
#define netif_dbg(priv, type, netdev, format, args...)		\
do {								\
	if (netif_msg_##type(priv))				\
		dynamic_netdev_dbg(netdev, format, ##args);	\
} while (0)
```
对于是对于 dynamic_debug 机制再添加了一些功能？

## 先将 man dmesg(1) 都看一遍吧

sudo sysctl -w kernel.printk=4
sudo sysctl kernel.printk

CONSOLE_LOGLEVEL_DEFAULT 做什么的?


## 总结下常见的使用技巧吧

再次，理解下 printk 的输出结果
```txt
  quiet           [KNL] Disable most log messages
```

## 为什么实时性和 printk 的关系这么大

## 太复杂了
- https://lwn.net/Articles/800946/
  - https://lpc.events/event/4/contributions/290/attachments/276/463/lpc2019_jogness_printk.pdf

## 如果想让启动的时候的 pr_debug 打开如何
```c
	pr_debug("v" INTEL_IDLE_VERSION " model 0x%X\n",
		 boot_cpu_data.x86_model);
```

总结不错:
https://www.cnblogs.com/pengdonglin137/p/5808373.html

## 有趣的调查
https://spectrum.library.concordia.ca/id/eprint/987184/1/Patel_MSc_F2020.pdf

## 这里总结了 options

- Documentation/admin-guide/sysctl/kernel.rst

cat /proc/sys/kernel/printk_delay
cat /proc/sys/kernel/printk_ratelimit : 在多长的时间里面
cat /proc/sys/kernel/printk_ratelimit_burst : 最多打印多少的信息出来

## 原来还可以这样啊
```sh
 sudo  dmesg --level=err,warn
```

## 如何把特定模块的日志屏蔽掉

例如 13900k 的行为:
```txt
[263498.057599] wlo1: disconnect from AP 20:ab:48:6c:8b:50 for new auth to 20:ab:48:6c:89:b0
[263498.099938] wlo1: authenticate with 20:ab:48:6c:89:b0 (local address=70:a8:d3:66:73:bc)
[263498.100769] wlo1: send auth to 20:ab:48:6c:89:b0 (try 1/3)
[263498.127029] wlo1: authenticated
[263498.127203] wlo1: associate with 20:ab:48:6c:89:b0 (try 1/3)
[263498.131187] wlo1: RX ReassocResp from 20:ab:48:6c:89:b0 (capab=0x1511 status=0 aid=7)
[263498.135735] wlo1: associated
[263498.167059] wlo1: Limiting TX power to 20 (23 - 3) dBm as advertised by 20:ab:48:6c:89:b0
[263531.932636] wlo1: disconnect from AP 20:ab:48:6c:89:b0 for new auth to 20:ab:48:6c:8b:50
[263531.974011] wlo1: authenticate with 20:ab:48:6c:8b:50 (local address=70:a8:d3:66:73:bc)
[263531.974832] wlo1: send auth to 20:ab:48:6c:8b:50 (try 1/3)
[263532.001477] wlo1: authenticated
[263532.002142] wlo1: associate with 20:ab:48:6c:8b:50 (try 1/3)
[263532.006334] wlo1: RX ReassocResp from 20:ab:48:6c:8b:50 (capab=0x1511 status=0 aid=3)
[263532.010993] wlo1: associated
[263532.018538] wlo1: Limiting TX power to 30 (33 - 3) dBm as advertised by 20:ab:48:6c:8b:50
[264123.191197] iwlwifi 0000:00:14.3: missed beacons exceeds threshold, but receiving data. Stay connected, Expect bugs.
[264123.191201] iwlwifi 0000:00:14.3: missed_beacons:19, missed_beacons_since_rx:1
[264123.293593] iwlwifi 0000:00:14.3: missed beacons exceeds threshold, but receiving data. Stay connected, Expect bugs.
[264123.293597] iwlwifi 0000:00:14.3: missed_beacons:20, missed_beacons_since_rx:1
[264123.396005] iwlwifi 0000:00:14.3: missed beacons exceeds threshold, but receiving data. Stay connected, Expect bugs.
[264123.396008] iwlwifi 0000:00:14.3: missed_beacons:21, missed_beacons_since_rx:1
[264123.498404] iwlwifi 0000:00:14.3: missed beacons exceeds threshold, but receiving data. Stay connected, Expect bugs.
[264123.498407] iwlwifi 0000:00:14.3: missed_beacons:22, missed_beacons_since_rx:1
[264123.600783] iwlwifi 0000:00:14.3: missed beacons exceeds threshold, but receiving data. Stay connected, Expect bugs.
[264123.600785] iwlwifi 0000:00:14.3: missed_beacons:23, missed_beacons_since_rx:1
[264123.703194] iwlwifi 0000:00:14.3: missed beacons exceeds threshold, but receiving data. Stay connected, Expect bugs.
```

似乎这些日志很难去掉发

## demsg -n 4 可以屏蔽 pr_info 日志

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
