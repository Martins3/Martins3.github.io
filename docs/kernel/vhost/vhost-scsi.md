## vhost-scsi 以及 vhost-user-scsi

vhost-scsi 居然不能热迁移，真奇怪的

https://github.com/spdk/spdk/issues/162

https://access.redhat.com/solutions/3394851

vhost 注入恐怕还是通过 qemu 的接口注入中断的

## vhost-scsi 的使用
https://zonedstorage.io/docs/tools/qemu

到底怎么用还是不知道:
```c
static const struct target_core_fabric_ops vhost_scsi_ops = {
	.module				= THIS_MODULE,
	.fabric_name			= "vhost",
	.max_data_sg_nents		= VHOST_SCSI_PREALLOC_SGLS,
	.tpg_get_wwn			= vhost_scsi_get_fabric_wwn,
	.tpg_get_tag			= vhost_scsi_get_tpgt,
	.tpg_check_demo_mode		= vhost_scsi_check_true,
	.tpg_check_demo_mode_cache	= vhost_scsi_check_true,
	.tpg_check_prot_fabric_only	= vhost_scsi_check_prot_fabric_only,
	.release_cmd			= vhost_scsi_release_cmd,
	.check_stop_free		= vhost_scsi_check_stop_free,
	.sess_get_initiator_sid		= NULL,
	.write_pending			= vhost_scsi_write_pending,
	.queue_data_in			= vhost_scsi_queue_data_in,
	.queue_status			= vhost_scsi_queue_status,
	.queue_tm_rsp			= vhost_scsi_queue_tm_rsp,
	.aborted_task			= vhost_scsi_aborted_task,
	/*
	 * Setup callers for generic logic in target_core_fabric_configfs.c
	 */
	.fabric_make_wwn		= vhost_scsi_make_tport,
	.fabric_drop_wwn		= vhost_scsi_drop_tport,
	.fabric_make_tpg		= vhost_scsi_make_tpg,
	.fabric_drop_tpg		= vhost_scsi_drop_tpg,
	.fabric_post_link		= vhost_scsi_port_link,
	.fabric_pre_unlink		= vhost_scsi_port_unlink,

	.tfc_wwn_attrs			= vhost_scsi_wwn_attrs,
	.tfc_tpg_base_attrs		= vhost_scsi_tpg_attrs,
	.tfc_tpg_attrib_attrs		= vhost_scsi_tpg_attrib_attrs,

	.default_submit_type		= TARGET_QUEUE_SUBMIT,
	.direct_submit_supp		= 1,
};
```

似乎是必须使用 iscsi 吗?

vhost_scsi_make_tport
```c
	ptr = strstr(name, "naa.");
	if (ptr) {
		tport->tport_proto_id = SCSI_PROTOCOL_SAS;
		goto check_len;
	}
	ptr = strstr(name, "fc.");
	if (ptr) {
		tport->tport_proto_id = SCSI_PROTOCOL_FCP;
		off = 3; /* Skip over "fc." */
		goto check_len;
	}
	ptr = strstr(name, "iqn.");
	if (ptr) {
		tport->tport_proto_id = SCSI_PROTOCOL_ISCSI;
		goto check_len;
	}
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
