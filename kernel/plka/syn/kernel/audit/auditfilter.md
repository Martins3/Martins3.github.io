# kernel/auditfilter.c

## todo & question

1. 负责是什么模块
2. 谁来调用我 : 捕获一个信号之后，然后查询当前是否存在对于其的
3. lsm tree 和 watch 分别是什么 ?

## filter_list

```c
/* Audit filter lists, defined in <linux/audit.h> */
struct list_head audit_filter_list[AUDIT_NR_FILTERS] = {
	LIST_HEAD_INIT(audit_filter_list[0]),
	LIST_HEAD_INIT(audit_filter_list[1]),
	LIST_HEAD_INIT(audit_filter_list[2]),
	LIST_HEAD_INIT(audit_filter_list[3]),
	LIST_HEAD_INIT(audit_filter_list[4]),
	LIST_HEAD_INIT(audit_filter_list[5]),
	LIST_HEAD_INIT(audit_filter_list[6]),
#if AUDIT_NR_FILTERS != 7
#error Fix audit_filter_list initialiser
#endif
};
static struct list_head audit_rules_list[AUDIT_NR_FILTERS] = {
	LIST_HEAD_INIT(audit_rules_list[0]),
	LIST_HEAD_INIT(audit_rules_list[1]),
	LIST_HEAD_INIT(audit_rules_list[2]),
	LIST_HEAD_INIT(audit_rules_list[3]),
	LIST_HEAD_INIT(audit_rules_list[4]),
	LIST_HEAD_INIT(audit_rules_list[5]),
	LIST_HEAD_INIT(audit_rules_list[6]),
};

/* Rule flags */
#define AUDIT_FILTER_USER	0x00	/* Apply rule to user-generated messages */
#define AUDIT_FILTER_TASK	0x01	/* Apply rule at task creation (not syscall) */
#define AUDIT_FILTER_ENTRY	0x02	/* Apply rule at syscall entry */
#define AUDIT_FILTER_WATCH	0x03	/* Apply rule to file system watches */
#define AUDIT_FILTER_EXIT	0x04	/* Apply rule at syscall exit */
#define AUDIT_FILTER_EXCLUDE	0x05	/* Apply rule before record creation */
#define AUDIT_FILTER_TYPE	AUDIT_FILTER_EXCLUDE /* obsolete misleading naming */
#define AUDIT_FILTER_FS		0x06	/* Apply rule at __audit_inode_child */
```

> @todo 所以，audit_rules_list 的作用是什么 ? 为什么 audit_add_rule 是将 rule 添加到到其上，而不是 filter 上


## audit_filter
调用各种 comparator 在对应的 audit_filter_list 上进行移动

两个位于 audit.c 中间的 ref :
1. audit_log_start
2. audit_receive_msg

## comparator

```c
audit_uid_comparator
audit_gid_comparator
audit_comparator
```


## core 

```c
audit_add_rule 
audit_del_rule 
  audit_find_rule
      audit_compare_rule
```

## audit_init_entry
1. audit_entry 持有 audit_krule , audit_krule 持有 audit_field
2. 创建特定数量的 filterlist
```c
/* Initialize an audit filterlist entry. */
static inline struct audit_entry *audit_init_entry(u32 field_count)
{
	struct audit_entry *entry;
	struct audit_field *fields;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (unlikely(!entry))
		return NULL;

	fields = kcalloc(field_count, sizeof(*fields), GFP_KERNEL);
	if (unlikely(!fields)) {
		kfree(entry);
		return NULL;
	}
	entry->rule.fields = fields;

	return entry;
}
```


## audit_data_to_entry
1. 这就是转换函数
2. 从下到上全部都是被唯一调用
3. sk_buff : 内核 和 userspace 之间使用网络沟通 ?


```c
static int __net_init audit_net_init(struct net *net)
{
	struct netlink_kernel_cfg cfg = {
		.input	= audit_receive,
		.bind	= audit_bind,
		.flags	= NL_CFG_F_NONROOT_RECV,
		.groups	= AUDIT_NLGRP_MAX,
	};

	struct audit_net *aunet = net_generic(net, audit_net_id);

	aunet->sk = netlink_kernel_create(net, NETLINK_AUDIT, &cfg);
	if (aunet->sk == NULL) {
		audit_panic("cannot initialize netlink socket in namespace");
		return -ENOMEM;
	}
	aunet->sk->sk_sndtimeo = MAX_SCHEDULE_TIMEOUT;

	return 0;
}


/**
 * audit_receive - receive messages from a netlink control socket
 * @skb: the message buffer
 *
 * Parse the provided skb and deal with any messages that may be present,
 * malformed skbs are discarded.
 */
static void audit_receive(struct sk_buff  *skb)

/**
 * audit_rule_change - apply all rules to the specified message type
 * @type: audit message type
 * @seq: netlink audit message sequence (serial) number
 * @data: payload data
 * @datasz: size of payload data
 */
int audit_rule_change(int type, int seq, void *data, size_t datasz)

/**
 * audit_rule_change - apply all rules to the specified message type
 * @type: audit message type
 * @seq: netlink audit message sequence (serial) number
 * @data: payload data
 * @datasz: size of payload data
 */
int audit_rule_change(int type, int seq, void *data, size_t datasz)

/* Translate struct audit_rule_data to kernel's rule representation. */
static struct audit_entry *audit_data_to_entry(struct audit_rule_data *data,
					       size_t datasz)

/* Common user-space to kernel rule translation. */
static inline struct audit_entry *audit_to_entry_common(struct audit_rule_data *rule)
```

## audit_rule_change

```c
/**
 * audit_rule_change - apply all rules to the specified message type
 * @type: audit message type
 * @seq: netlink audit message sequence (serial) number
 * @data: payload data
 * @datasz: size of payload data
 */
int audit_rule_change(int type, int seq, void *data, size_t datasz)
{
	int err = 0;
	struct audit_entry *entry;

	entry = audit_data_to_entry(data, datasz);
	if (IS_ERR(entry))
		return PTR_ERR(entry);

	switch (type) {
	case AUDIT_ADD_RULE:
		err = audit_add_rule(entry);
		audit_log_rule_change("add_rule", &entry->rule, !err);
		break;
	case AUDIT_DEL_RULE:
		err = audit_del_rule(entry);
		audit_log_rule_change("remove_rule", &entry->rule, !err);
		break;
	default:
		err = -EINVAL;
		WARN_ON(1);
	}

	if (err || type == AUDIT_DEL_RULE) {
		if (entry->rule.exe)
			audit_remove_mark(entry->rule.exe);
		audit_free_rule(entry);
	}

	return err;
}
```
