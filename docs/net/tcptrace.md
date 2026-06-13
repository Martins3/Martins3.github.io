## tcptrace 如何使用
<!-- 0d2b8ecb-acbf-4453-a284-9a857995978f -->

https://lotabout.me/2025/wireshark-tcptrace/#fnref1

> [!NOTE]
> 参考神奇海螺的意见，有待验证

tcptrace 是一个用于分析 TCP 连接行为 的离线工具，输入通常是 tcpdump / Wireshark 生成的 pcap 文件。
它并不做逐包的协议细节解码，而是从 连接（flow） 的角度，统计和推断 TCP 的运行特性。


python3 -m  http.server (用这个测试)

sudo tcpdump -w one.pcap host 10.0.0.2 and port 8000

tcptrace -T one.pcap

```txt
🤒  tcptrace one.pcap
1 arg remaining, starting with 'one.pcap'
Ostermann's tcptrace -- version 6.6.7 -- Thu Nov  4, 2004

21 packets seen, 21 TCP packets traced
elapsed wallclock time: 0:00:10.056516, 2 pkts/sec analyzed
trace file elapsed time: 0:00:00.580002
TCP connection info:
  1: 10.0.0.8:52710 - 10.0.0.2:8000 (a2b)    5>    6<  (complete)
  2: 10.0.0.8:59369 - 10.0.0.2:8000 (c2d)    5>    5<  (complete)
```

### 那么这两个小老弟是什么作用?
- tcpflow
- tcproute

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
