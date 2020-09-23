# kernel/audit.md
1. 主要处理
    1. kauditd
    2. 利用 net 和用户层交流
    3. log 将消息保存
    4. 配置audit(不是添加rule, 修改 rule 是从用户层到达，经过 audit_receive_msg 到达的)

## question & ref 
1. 似乎内核态的audit 都是基于 auditsc.c 的
    2. 用户态的将消息利用net 发送过来，然后处理

#### [audit-documentation](https://github.com/linux-audit/audit-documentation/wiki)

## audit_receive_msg
1. 接收到消息之后，然后对于其进行具体的处理

```c
/* The netlink messages for the audit system is divided into blocks:
 * 1000 - 1099 are for commanding the audit system
 * 1100 - 1199 user space trusted application messages
 * 1200 - 1299 messages internal to the audit daemon
 * 1300 - 1399 audit event messages
 * 1400 - 1499 SE Linux use
 * 1500 - 1599 kernel LSPP events
 * 1600 - 1699 kernel crypto events
 * 1700 - 1799 kernel anomaly records
 * 1800 - 1899 kernel integrity events
 * 1900 - 1999 future kernel use
 * 2000 is for otherwise unclassified kernel audit messages (legacy)
 * 2001 - 2099 unused (kernel)
 * 2100 - 2199 user space anomaly records
 * 2200 - 2299 user space actions taken in response to anomalies
 * 2300 - 2399 user space generated LSPP events
 * 2400 - 2499 user space crypto events
 * 2500 - 2999 future user space (maybe integrity labels and related events)
 *
 * Messages from 1000-1199 are bi-directional. 1200-1299 & 2100 - 2999 are
 * exclusively user space. 1300-2099 is kernel --> user space 
 * communication.
 */
```

```c
static int audit_receive_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
  // ...
	case AUDIT_FIRST_USER_MSG ... AUDIT_LAST_USER_MSG:
	case AUDIT_FIRST_USER_MSG2 ... AUDIT_LAST_USER_MSG2:
		if (!audit_enabled && msg_type != AUDIT_USER_AVC)
			return 0;

    // todo 如果 audit 消息来自于内核中间，比如 syscall 之类的
		err = audit_filter(msg_type, AUDIT_FILTER_USER);
		if (err == 1) { /* match or error */
			err = 0;
			if (msg_type == AUDIT_USER_TTY) {
				err = tty_audit_push();
				if (err)
					break;
			}
			audit_log_common_recv_msg(&ab, msg_type);
			if (msg_type != AUDIT_USER_TTY)
				audit_log_format(ab, " msg='%.*s'",
						 AUDIT_MESSAGE_TEXT_MAX,
						 (char *)data);
			else {
				int size;

				audit_log_format(ab, " data=");
				size = nlmsg_len(nlh);
				if (size > 0 &&
				    ((unsigned char *)data)[size - 1] == '\0')
					size--;
				audit_log_n_untrustedstring(ab, data, size);
			}
			audit_log_end(ab);
		}
		break;
  // ...
```

## audit_log
1. 似乎就是简单的向 sk_buff 中间 buffer 中间写入数据，当然，按照类似于 printf 的感觉

```c
/**
 * audit_log_start - obtain an audit buffer
 * @ctx: audit_context (may be NULL)
 * @gfp_mask: type of allocation
 * @type: audit message type
 *
 * Returns audit_buffer pointer on success or NULL on error.
 *
 * Obtain an audit buffer.  This routine does locking to obtain the
 * audit buffer, but then no locking is required for calls to
 * audit_log_*format.  If the task (ctx) is a task that is currently in a
 * syscall, then the syscall is marked as auditable and an audit record
 * will be written at syscall exit.  If there is no associated task, then
 * task context (ctx) should be NULL.
 */
struct audit_buffer *audit_log_start(struct audit_context *ctx, gfp_t gfp_mask,
				     int type)

static struct audit_buffer *audit_buffer_alloc(struct audit_context *ctx,
					       gfp_t gfp_mask, int type)

EXPORT_SYMBOL(audit_log_start);
EXPORT_SYMBOL(audit_log_end);
EXPORT_SYMBOL(audit_log_format);
EXPORT_SYMBOL(audit_log);
```
