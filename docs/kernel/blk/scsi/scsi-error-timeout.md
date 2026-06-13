# 以 scsi 为例子分析 blk 层超时机制

直接释放已经提交的 request 是不可以的，如果设备忽然又醒过来如何办?
所以正确的操作是，首先 abort 之前的命令，然后重试，如果 abort 失败，
那么等待，如果再次超时，那么 reset 设备。

## 超时机制基本流程

如果要注入 io-timeout-fail 的话，那么 nvme_try_complete_req 中会直接跳过 `blk_mq_complete_request_remote` ，导致 永远不会成功。

- blk_mq_start_request : **驱动开始向硬件提交的时机** ，所以超时时间指的是硬件需要花费多少时间来返回，而不是一个 io 被用户态提交多久。
  - blk_add_timer : 开始计时，但是大多数 request 应该是走快速路径的，不会去维护 timer ，细节到时候分析吧

- blk_mq_timeout_work : `struct work_struct	timeout_work` 的 hook
  - blk_mq_handle_expired
    - blk_mq_rq_timed_out
      - blk_mq_queue_tag_busy_iter(q, blk_mq_handle_expired, &expired); : 对于所有的 request 执行 blk_mq_handle_expired
        - ret = req->q->mq_ops->timeout(req); : 具体的注册取决于驱动
          - blk_mq_ops::timeout -> scsi_timeout (如果不注册，那么就是无限重试)
            - scsi_host_template::eh_timed_out -> virtscsi_eh_timed_out
          - nvme_timeout

blk_mq_timeout_work 每 blk_mq_tag_set::timeout 运行一次，检查每一个 request 的 timeout 时间

如果想要测试 scsi ，可以尝试分析:
```c
static enum scsi_timeout_action virtscsi_eh_timed_out(struct scsi_cmnd *scmnd)
{

	if (time_after(jiffies, scmnd->jiffies_at_alloc +
				(90* 2) * HZ)) {
		return SCSI_EH_NOT_HANDLED;
	}

	return SCSI_EH_RESET_TIMER;
}
```


有趣的是:
```c
struct request {
  // ...

	/* Time that this request was allocated for this IO. */
	u64 start_time_ns;
	/* Time that I/O was submitted to the device. */
	u64 io_start_time_ns;

  // ...
}
```

io_start_time_ns 没有被利用起来监控硬件长时间不返回的情况，只是用于 cgroup


## 五个超时时间配置

- request_queue::rq_timeout
- scsi_device::eh_timeout : 默认 20s ，scsi_send_eh_cmnd 调用是一个同步的过程
- SD_TIMEOUT: 在 sd.c 中类似 scsi_execute_cmd 执行命令会修改默认 scsi cmd 的超时时间
- eh_deadline : 控制 eh 可以处理多长时间，如果 eh 处理超过了这个时间，直接 reset 整个 HBA 卡
- request::timeout : request::timeout 默认 request_queue::timeout ，但是 request 可以来控制自己的超时，例如在 sg_scsi_ioctl 中

更多参考

- eh_deadline redhat 的分析 https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_storage_devices/configuring-maximum-time-for-storage-error-recovery-with-eh_deadline_managing-storage-devices

从 scsi_eh_stu 到 scsi_eh_host_reset 都是调用 Fine-grained 的 hook 的，其实内部都是存在超时的，
例如 megasas_task_abort -> megasas_task_abort_fusion -> megasas_issue_tm 中设置的是 50S 的时间。

eh_deadline 的实现就是靠 scsi_eh_ready_devs 每一级的检测中检查 scsi_host_eh_past_deadline()

## 如何修改一个卡的超时时间设置

有的驱动可以调整

- megasas_set_static_target_properties
  - blk_queue_rq_timeout(sdev->request_queue, scmd_timeout \* HZ);

也可以通过 sysfs:

- queue_io_timeout_store

## 超时的核心流程

- scsi_abort_command `[previous abort failed]` `[abort scheduled]`

  - 异步启动: scmd_eh_abort_handler (异步执行，因为当前在 blk_mq_timeout_work 的大循环中，不能阻碍其他的 request 的检测 )
    - 尝试 abort 的命令 `[aborting command]`
      - scsi_try_to_abort_cmd
        - megasas_task_abort `[Command for which abort is issued is not found in outstanding commands]`
          `[finish aborted command]` `[retry aborted command]`
      - 如果成功，进行 retry `[retry aborted command]`
      - retry 次数超过 `[finish aborted command]`
    - 如果失败，`[cmd abort failed]` scsi_eh_scmd_add
      - scsi_eh_inc_host_failed : 等所有的 io 返回后调用 scsi_eh_wakeup 唤醒 scsi_error_handler

- scsi_error_handler
  - scsi_unjam_host
    - scsi_eh_ready_devs
      - scsi_eh_stu
        - scsi_eh_try_stu -> scsi_send_eh_cmnd
        - scsi_eh_tur
    - scsi_eh_flush_done_q : `[flush retry cmd]`

如果使用 timeout 注入，使用 virtio scsi ，将 virtscsi_eh_timed_out 的返回值直接修改 SCSI_EH_NOT_HANDLED

| 0  | 启动                                                                                                                          |
|----|-------------------------------------------------------------------------------------------------------------------------------|
| 30 | scsi_timeout 中执行 scsi_abort_command ，无论 abort 是否成功，scsi_timeout 会首先立刻返回成功，这里是启动一个异步进程来执行   |
| 60 | 在 scsi_timeout 中执行 scsi_abort_command 执行失败, 通过 scmd->eh_eflags & SCSI_EH_ABORT_SCHEDULED 知道该进程已经被 eh 过一次 |
| 90 | 在 eh 处理存在超时，第二次执行 sd_eh_action ，其中 scsi_device_set_state(sdev, SDEV_OFFLINE);                                 |

## 中断返回的检测过程

- scsi_complete
  - scsi_decide_disposition
    - switch (host_byte(scmd->result)) { # 首先检查 host byte
    - switch (get_status_byte(scmd)) {   # 然后检查 status byte ，如果是 SAM_STAT_CHECK_CONDITION ，那么调用 scsi_check_sense 进一步检查盘

- scsi_check_sense 首先会调用 scsi_report_sense


## [ ] 驱动在错误流程中的处理

scsi_try_host_reset
virtscsi_device_reset
virtscsi_abort
virtscsi_commit_rqs

```c
void scsi_eh_ready_devs(struct Scsi_Host *shost,
			struct list_head *work_q,
			struct list_head *done_q)
{
	if (!scsi_eh_stu(shost, work_q, done_q))
		if (!scsi_eh_bus_device_reset(shost, work_q, done_q))
			if (!scsi_eh_target_reset(shost, work_q, done_q))
				if (!scsi_eh_bus_reset(shost, work_q, done_q))
					if (!scsi_eh_host_reset(shost, work_q, done_q))
						scsi_eh_offline_sdevs(work_q,
								      done_q);
}
```

## scsi 错误处理机制的漏洞

### 如果返回成功，但是没有一个 bit 成功

验证犯法 1:

- map_cmd_status 中设置 data_length = 0

## 什么时候会自动 offline 盘

一共两个位置来 offline 盘，第一个是 sd_eh_action 中

路径 1:

- scsi_eh_stu
- scsi_eh_test_devices <--- target/bus/host 都会调用
- scsi_eh_bus_device_reset
- scsi_eh_bus_reset
  - scsi_eh_action
    - scsi_driver::eh_action -> sd_eh_action 在

路径 2:

- scsi_eh_offline_sdevs

这些函数调用地方是穿插到核心路径中的:

```c
void scsi_eh_ready_devs(struct Scsi_Host *shost,
			struct list_head *work_q,
			struct list_head *done_q)
{
	if (!scsi_eh_stu(shost, work_q, done_q))                // <----- 路径 1
		if (!scsi_eh_bus_device_reset(shost, work_q, done_q)) // <----- 路径 1
			if (!scsi_eh_target_reset(shost, work_q, done_q))   // <----- 路径 1
				if (!scsi_eh_bus_reset(shost, work_q, done_q))    // <----- 路径 1
					if (!scsi_eh_host_reset(shost, work_q, done_q)) // <----- 路径 1
						scsi_eh_offline_sdevs(work_q,                 // <---------- 路径 2
								      done_q);
}
```

如果去小心的控制那个 reset 成功，那个 reset 失败，最后可以实现总是不去 offline disk 的

```txt
[ 6907.605910] scsi host1: Waking error handler thread
[ 6907.608289] scsi host1: scsi_eh_1: waking up 0/14/14
[ 6907.609279] scsi host1: scsi_eh_prt_fail_stats: cmds failed: 0, cancel: 14
[ 6907.609958] scsi host1: Total of 14 commands on 1 devices require eh work
[ 6907.610561] sd 1:0:1:0: scsi_eh_1: Sending BDR
[ 6907.611141] sd 1:0:1:0: scsi_eh_1: BDR failed

[ 6907.611693] scsi host1: scsi_eh_1: Sending target reset to target 1                  <---- 配置 scsi_eh_target_reset 失败了
[ 6907.612245] scsi host1: scsi_eh_1: Target reset failed target: 1

[ 6907.612780] scsi host1: scsi_eh_1: Sending BRST chan: 0
[ 6907.613304] sd 1:0:1:0: [sdb] tag#1734 scsi_try_bus_reset: Snd Bus RST               <---- bus 也失败了
[ 6907.613808] scsi host1: scsi_eh_1: BRST failed chan: 0

[ 6907.614310] scsi host1: scsi_eh_1: Sending HRST
[ 6907.614782] scsi host1: Snd Host RST
[ 6907.615254] sd 1:0:1:0: [sdb] tag#1734 OCR is requested due to IO timeout!!
[ 6907.615718] sd 1:0:1:0: [sdb] tag#1734 SCSI host state: 5  FW outstanding: 0
[ 6907.616191] sd 1:0:1:0: [sdb] tag#1734 scmd: (0x00000000b095e7b3)  retries: 0x0  allowed: 0x5
[ 6907.616669] sd 1:0:1:0: [sdb] tag#1734 CDB: Write(10) 2a 00 0e dc 64 00 00 00 20 00
[ 6907.617142] megaraid_sas 0000:31:00.0: megasas_disable_intr_fusion is called outbound_intr_mask:0x40000009
[ 6907.617653] megaraid_sas 0000:31:00.0: megasas_enable_intr_fusion is called outbound_intr_mask:0x40000000
[ 6917.816878] sd 1:0:1:0: [sdb] tag#1734 scsi_eh_done result: 0                        <---- host rset

[ 6917.817475] sd 1:0:1:0: [sdb] tag#1734 scsi_send_eh_cmnd timeleft: 10000             <---- scsi_eh_host_reset 中
[ 6917.818273] sd 1:0:1:0: [sdb] tag#1734 scsi_send_eh_cmnd: scsi_eh_completed_normally 2002
[ 6917.818987] sd 1:0:1:0: [sdb] tag#1734 scsi_eh_done result: 0
[ 6917.820191] sd 1:0:1:0: [sdb] tag#1734 scsi_send_eh_cmnd timeleft: 9999
[ 6917.820970] sd 1:0:1:0: [sdb] tag#1734 scsi_send_eh_cmnd: scsi_eh_completed_normally 2002
[ 6917.821588] sd 1:0:1:0: [sdb] tag#1734 scsi_eh_tur return: 2002

[ 6917.822171] sd 1:0:1:0: [sdb] tag#1734 scsi_eh_1: flush retry cmd
[ 6917.822750] sd 1:0:1:0: [sdb] tag#1732 scsi_eh_1: flush retry cmd
[ 6917.829086] sd 1:0:1:0: [sdb] tag#1755 scsi_eh_1: flush retry cmd
[ 6917.829573] scsi host1: waking up host to restart
[ 6917.830148] scsi host1: scsi_eh_1: sleeping
```

## [ ] eh thread 的不同机制

1. eh thread 真的需要等待所有的 io 都出现问题的时候才会启动吗?

## scsi_send_eh_cmnd : 专门发送错误处理的命令的同步函数

调用者:

- scsi_eh_tur
- scsi_request_sense
- scsi_eh_try_stu

- scsi_send_eh_cmnd
  - rtn = shost->hostt->queuecommand(shost, scmd); : 使用常规路径发送命令
  - timeleft = wait_for_completion_timeout(&done, timeout); 但是会等待结果
  - scsi_eh_completed_normally : 如果成功很好

## raid 的 failfast 如何作用于 allowed maxretry 机制

```c
/*
 * These flags should really be called "NO_RETRY" rather than
 * "FAILFAST" because they don't make any promise about time lapse,
 * only about the number of retries, which will be zero.
 * REQ_FAILFAST_DRIVER is not included because
 * Commit: 4a27446f3e39 ("[SCSI] modify scsi to handle new fail fast flags.")
 * seems to suggest that the errors it avoids retrying should usually
 * be retried.
 */
#define	MD_FAILFAST	(REQ_FAILFAST_DEV | REQ_FAILFAST_TRANSPORT)
```
在 scsi_noretry_cmd 中对于 REQ_FAILFAST_DEV 和 REQ_FAILFAST_TRANSPORT 进行判断

## 错误处理中如何阻碍其他进程
参考 scsi_block_when_processing_errors

## 进入到 eh 的标志: scsi_eh_scmd_add

## fault injection 对于 scsi 错误处理命令无效

```c
static void scsi_done_internal(struct scsi_cmnd *cmd, bool complete_directly)
{
	struct request *req = scsi_cmd_to_rq(cmd);

	switch (cmd->submitter) {
	case SUBMITTED_BY_BLOCK_LAYER:
		break;
	case SUBMITTED_BY_SCSI_ERROR_HANDLER:
		return scsi_eh_done(cmd);          // <-- tur 命令直接成功返回
	case SUBMITTED_BY_SCSI_RESET_IOCTL:
		return;
	}

	if (unlikely(blk_should_fake_timeout(scsi_cmd_to_rq(cmd)->q))) // <--- 普通命令才会接受
		return;
	if (unlikely(test_and_set_bit(SCMD_STATE_COMPLETE, &cmd->state)))
		return;
	trace_scsi_dispatch_cmd_done(cmd);

	if (complete_directly)
		blk_mq_complete_request_direct(req, scsi_complete);
	else
		blk_mq_complete_request(req);
}
```

## Scsi_Host::host_blocked

如何起作用:
- scsi_queue_rq
  - scsi_host_queue_ready : 如果检查到 host_blocked > 0 ，那么挂到 Scsi_Host::starved_list 上, scsi_run_queue 中执行 scsi_starved_list_run 来

```c
static inline bool scsi_host_is_busy(struct Scsi_Host *shost)
{
	if (atomic_read(&shost->host_blocked) > 0) // <-
		return true;
	if (shost->host_self_blocked) // <--- 如果 HBA 驱动自己设置自己为 block
		return true;
	return false;
}
```

在函数 `scsi_set_blocked` 中时候设置 host_blocked ，而 `scsi_set_blocked` 的调用位置为
    - __scsi_queue_insert : 在 requeue 的时候
    - scsi_queue_rq : 如果 dispatch 失败

scsi_set_blocked 会将 Scsi_Host::host_blocked 设置为 Scsi_Host::max_host_blocked ，后者一般默认初始化为：
```c
#define SCSI_DEFAULT_HOST_BLOCKED	7
```

## 重新插入队列的 request 会获取一个新的队列吗
- scsi_queue_insert
  - blk_mq_requeue_request
    - __blk_mq_requeue_request
      - blk_mq_put_driver_tag : 有次可见，的确是会重建一个 driver 的
      - blk_mq_request_started
    - blk_mq_sched_requeue_request

## 关键参考

- Documentation/scsi/scsi_eh.rst
  - http://127.0.0.1:3434/scsi/scsi_eh.html
- https://zhuanlan.zhihu.com/p/152213307
- https://www.seagate.com/cn/zh/support/kb/scsi-sense-key-chart-196259en/
- http://events17.linuxfoundation.org/sites/events/files/slides/SCSI-EH.pdf
- https://en.wikipedia.org/wiki/SCSI_check_condition

> When the target returns a Check Condition in response to a command, the initiator usually then issues a SCSI Request Sense command in order to obtain more information.
> 如果接受到 Check Condition ，那么就发送一个

- https://www.t10.org/lists/2status.htm


## [ ] TUR 命令是否可以 retry
据说，不会 retry ， 因为 blk_rq_is_passthrough

测试了一下，似乎并不会，下面的这个 `#242` 似乎被连续发送了很多次
```txt
[   73.766810] FAULT_INJECTION: forcing a failure.
               name fail_io_timeout, interval 0, probability 100, space 0, times -1
[   76.836431] sd 0:0:3:4: [sda] tag#242 Done: TIMEOUT_ERROR Result: hostbyte=DID_OK driverbyte=DRIVER_OK cmd_age=15s
[   76.836764] sd 0:0:3:4: [sda] tag#242 CDB: Synchronize Cache(10) 35 00 00 00 00 00 00 00 00 00
[   76.836994] sd 0:0:3:4: [sda] tag#242 scsi host busy 1 failed 0
[   76.837159] sd 0:0:3:4: [sda] tag#242 previous abort failed
[   76.845477] scsi host0: Waking error handler thread
[   76.845613] scsi host0: scsi_eh_0: waking up 0/1/1
[   76.845756] scsi host0: scsi_eh_prt_fail_stats: cmds failed: 0, cancel: 1
[   76.845938] scsi host0: Total of 1 commands on 1 devices require eh work
[   76.846118] sd 0:0:3:4: scsi_eh_0: Sending BDR
[   76.846240] sd 0:0:3:4: device reset
[   76.846339] sd 0:0:3:4: scsi_eh_0: BDR failed
[   76.846459] scsi host0: scsi_eh_0: Sending target reset to target 3
[   76.846635] scsi host0: scsi_eh_0: Target reset failed target: 3
[   76.846807] scsi host0: scsi_eh_0: Sending BRST chan: 0
[   76.846951] sd 0:0:3:4: [sda] tag#242 scsi_try_bus_reset: Snd Bus RST
[   76.847124] scsi host0: scsi_eh_0: BRST failed chan: 0
[   76.847264] scsi host0: scsi_eh_0: Sending HRST
[   76.847390] scsi host0: Snd Host RST
[   76.847496] sd 0:0:3:4: [sda] tag#242 Send: scmd 0x00000000a41a6d52
[   76.847670] sd 0:0:3:4: [sda] tag#242 CDB: Test Unit Ready 00 00 00 00 00 00
[   76.848000] sd 0:0:3:4: [sda] tag#242 scsi_eh_done result: 0
[   76.848243] sd 0:0:3:4: [sda] tag#242 Done: SUCCESS Result: hostbyte=DID_OK driverbyte=DRIVER_OK cmd_age=15s
[   76.848511] sd 0:0:3:4: [sda] tag#242 CDB: Test Unit Ready 00 00 00 00 00 00
[   76.848709] sd 0:0:3:4: [sda] tag#242 scsi host busy 1 failed 1
[   76.848874] sd 0:0:3:4: [sda] tag#242 scsi_send_eh_cmnd timeleft: 10000
[   76.849054] sd 0:0:3:4: [sda] tag#242 scsi_send_eh_cmnd: scsi_eh_completed_normally 2002
[   76.849270] sd 0:0:3:4: [sda] tag#242 scsi_eh_tur return: 2002
[   76.849429] sd 0:0:3:4: [sda] tag#242 scsi_eh_0: flush retry cmd
[   76.849596] sd 0:0:3:4: [sda] tag#242 Inserting command 00000000a41a6d52 into mlqueue
[   76.849816] scsi host0: waking up host to restart
[   76.849947] scsi host0: scsi_eh_0: sleeping
[   76.857545] sd 0:0:3:4: unblocking device at zero depth
[   76.857714] sd 0:0:3:4: [sda] tag#242 Send: scmd 0x00000000a41a6d52
[   76.857883] sd 0:0:3:4: [sda] tag#242 CDB: Synchronize Cache(10) 35 00 00 00 00 00 00 00 00 00
```

## 到底可以重试多少次
- scsi_disk::max_medium_access_timeouts 默认为 2
- scsi_disk::max_retries 默认为 5
- 大多是时候，scsi_disk::max_retries 会传递给 scsi_cmnd::allowed

```c
static bool scsi_cmd_retry_allowed(struct scsi_cmnd *cmd)
{
	if (cmd->allowed == SCSI_CMD_RETRIES_NO_LIMIT)
		return true;

	return ++cmd->retries <= cmd->allowed;
}
```
- scmd_eh_abort_handler
- scsi_decide_disposition
- scsi_eh_flush_done_q


这里就是可能导致超时是 5 倍的时间，一般来说，不会允许无线超时的:
```c
static bool scsi_cmd_runtime_exceeced(struct scsi_cmnd *cmd)
{
	struct request *req = scsi_cmd_to_rq(cmd);
	unsigned long wait_for;

	if (cmd->allowed == SCSI_CMD_RETRIES_NO_LIMIT)
		return false;

	wait_for = (cmd->allowed + 1) * req->timeout;
	if (time_before(cmd->jiffies_at_alloc + wait_for, jiffies)) {
		scmd_printk(KERN_ERR, cmd, "timing out command, waited %lus\n",
			    wait_for/HZ);
		return true;
	}
	return false;
}
```

- scsi_done
  - scsi_done_internal : 在这里注入 timeout 错误，导致后面都无法执行
    - scsi_complete : 我靠，这是 multiqueue 的 hook，是存在中断返回的，但是 返回不太成功
      - scsi_finish_command
        - scsi_io_completion
          - scsi_io_completion_action : 使用 scsi_cmd_runtime_exceeced

scsi_eh_get_sense() 中看到了 `SCSI_CMD_RETRIES_NO_LIMIT`

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
