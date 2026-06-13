## nettrace

https://github.com/OpenCloudOS/nettrace

结果如下:

当在 10.0.0.2 ping 10.0.101.0 的时候，其中 10.0.101.0 的网卡是关联到 ovs 上的:

在 10.0.101.0 中观察:
sudo ./nettrace -p icmp --diag --diag-keep --date --detail

正好是一个收包和发包的过程:

```txt
***************** 11d0da00 ***************
[2025-4-22 11:49:14.755614] [11d0da00][napi_gro_receive_entry][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755623] [11d0da00][dev_gro_receive     ][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755627] [11d0da00][__netif_receive_skb_core][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755629] [11d0da00][ovs_vport_receive   ][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755631] [11d0da00][ovs_dp_process_packet][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15

[2025-4-22 11:49:14.755635] [11d0da00][enqueue_to_backlog  ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755639] [11d0da00][__netif_receive_skb_core][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755640] [11d0da00][ip_rcv              ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755643] [11d0da00][ip_rcv_core         ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755645] [11d0da00][nf_hook_slow        ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15 *ipv4 in chain: PRE_ROUTING*
[2025-4-22 11:49:14.755651] [11d0da00][ip_route_input_slow ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755654] [11d0da00][fib_validate_source ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755658] [11d0da00][ip_local_deliver    ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755660] [11d0da00][nf_hook_slow        ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15 *ipv4 in chain: INPUT*
[2025-4-22 11:49:14.755661] [11d0da00][ipt_do_table        ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15 *iptables table:filter, chain:INPUT* *packet is accepted*
[2025-4-22 11:49:14.755664] [11d0da00][ip_local_deliver_finish][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755667] [11d0da00][icmp_rcv            ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755669] [11d0da00][icmp_echo           ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755670] [11d0da00][icmp_reply          ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15
[2025-4-22 11:49:14.755711] [11d0da00][consume_skb         ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.0.2 -> 10.0.101.0 ping request, seq: 157, id: 15 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!

***************** 11d0db00 ***************
[2025-4-22 11:49:14.755677] [11d0db00][__ip_local_out      ][cpu:9  ][     ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755678] [11d0db00][nf_hook_slow        ][cpu:9  ][     ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15 *ipv4 in chain: OUTPUT*
[2025-4-22 11:49:14.755680] [11d0db00][ipt_do_table        ][cpu:9  ][     ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15 *iptables table:filter, chain:OUTPUT* *packet is accepted*
[2025-4-22 11:49:14.755683] [11d0db00][ip_output           ][cpu:9  ][     ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755684] [11d0db00][nf_hook_slow        ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15 *ipv4 in chain: POST_ROUTING*
[2025-4-22 11:49:14.755686] [11d0db00][ip_finish_output    ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755688] [11d0db00][ip_finish_output2   ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755691] [11d0db00][__dev_queue_xmit    ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755693] [11d0db00][dev_hard_start_xmit ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15 *skb is successfully sent to the NIC driver*
[2025-4-22 11:49:14.755694] [11d0db00][ovs_vport_receive   ][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755695] [11d0db00][ovs_dp_process_packet][cpu:9  ][   ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755696] [11d0db00][__dev_queue_xmit    ][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15
[2025-4-22 11:49:14.755697] [11d0db00][dev_hard_start_xmit ][cpu:9  ][  ][pid:0      ][swapper/9   ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15 *skb is successfully sent to the NIC driver*
[2025-4-22 11:49:14.755838] [11d0db00][consume_skb         ][cpu:11 ][  ][pid:0      ][swapper/11  ][ns:0] ICMP: 10.0.101.0 -> 10.0.0.2 ping reply, seq: 157, id: 15 *packet is freed (normally)*
---------------- ANALYSIS RESULT ---------------------
this is a good packet!
```

```txt
@[
    enqueue_to_backlog+1 <---

    netif_rx_internal+58
    netif_rx+50
    internal_dev_recv+184
    do_execute_actions+6529
    ovs_execute_actions+76
    ovs_dp_process_packet+168 <---- 这里，不知道为什么，这里调用了 consume_skb 了，这里的 pointer 也变化了
    ovs_vport_receive+132
    netdev_frame_hook+222
    __netif_receive_skb_core.constprop.0+540
    __netif_receive_skb_list_core+268
    netif_receive_skb_list_internal+472
    napi_complete_done+110
    virtnet_poll+1460
    __napi_poll+40
    net_rx_action+388
    handle_softirqs+220
    irq_exit_rcu+161
    common_interrupt+133
    asm_common_interrupt+38
    pv_native_safe_halt+15
    default_idle+19
    default_idle_call+48
    do_idle+437
    cpu_startup_entry+41
    start_secondary+247
    common_startup_64+318
]: 2
```

## ICMP 的 iq 和 sequence 是如何转递到

```c
struct icmphdr {
  __u8		type;
  __u8		code;
  __sum16	checksum;
  union {
	struct {
		__be16	id;
		__be16	sequence;
	} echo;
	__be32	gateway;
	struct {
		__be16	__unused;
		__be16	mtu;
	} frag;
	__u8	reserved[4];
  } un;
};
```
ts_print_packet 中是如何获取到这个 header 的 id 和 sequence 的?

## 解析工具
sudo ./nettrace -p icmp --diag --diag-keep --date --detail

```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

function node7() {

	read=/home/martins3/core/winshare/tmp_kernel/logs/node7_logs/nettrace.log.32
	first=$(sed -n '/2025-4-17 19:40:33/{=;q}' $read)
	second=$(sed -n '/2025-4-17 19:40:35/{=;q}' $read)
	sed -n "$first,${second}p" $read >output.txt

	if [[ -z $first  || -z $second ]]; then
		echo "bad time"
	fi

	mapfile -t id_array < <(grep -o -E "ICMP:.*id: [0-9]+" output.txt | sort -u)
	for i in "${id_array[@]}"; do
		rg "$i" output.txt >output.2.txt
		A=$(sed -n '1s/^\[\([^]]*\)\].*/\1/p' output.2.txt)
		B=$(sed -n '$s/^\[\([^]]*\)\].*/\1/p' output.2.txt)
		start_sec=$(date -d "$A" +%s.%6N)
		end_sec=$(date -d "$B" +%s.%6N)
		time_diff=$(echo "$end_sec - $start_sec" | bc)
		if (($(echo "$time_diff > 0.1" | bc -l))); then
			echo "Time difference ($time_diff) is greater than 0.2 seconds"
			mv output.2.txt "$i"
		fi
	done
}

node7
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
