 # wid crash 报告

2026-09-09 开始注意到 wid 在不断的 crash ，极有可能是奇怪的路由器环境导致的。

## deepseek-v4-flash 的分析
结论：iwd 3.10 引入的回归，收到"AP 定向漫游"帧时空指针解引用

崩溃点：src/station.c 的 station_ap_directed_roam()（由 ap_roam_frame_event() 帧监视回调进入）：

```c
  bool can_roam = !station_cannot_roam(station);
  bool ignore_candidates =
      station->connected_bss->vendor_quirks.ignore_bss_tm_candidates;  // <- 崩在这
  l_debug(...);
  if (station->state != STATION_STATE_CONNECTED) { ... return; }      // <- 检查在后面
```

处理 WNM BSS Transition Management 请求（category 0x0a / action 0x07，即 AP 的 band steering/负载均衡帧）时，station->connected_bss 在
未 CONNECTED 状态下是 NULL，而这行解引用放在了 STATION_STATE_CONNECTED 检查之前，导致 NULL 解引用。

这不是新引入：该行由上游 commit ffe79bfada333（"station: check vendor quirk for BSS TM request candidate list"，2025-08-27）加在函数开
头，随 iwd 3.10 发布。3.10 / 3.11 / 3.12（含你装的 3.12-1.fc44）全都有，上游 master 目前仍是 3.12，还没有任何修复版本。

证据链

- 从 8/31 到今天共有 14 个 iwd core，crash 完全一致：fault addr 0xd1、binary offset 0x2c889、error 4。今天 13:32 十秒内连崩两次（PID
  3787539 -> 4118389）后被 systemd 重启拉活。
- coredumpctl info 栈：ap_roam_frame_event (iwd+0x2c889) <- frame_watch_unicast_notify <- l_notifylist_notify_matches <-
  received_data（收到 action 帧）<- l_main_run。
- 反汇编 + gdb 现场：mov rax,[r12+0x28]; movzbl 0xd1(%rax),%r13d，寄存器 rax = 0（[r12+0x28] 取出的 station->connected_bss 是 NULL），
  随后读 vendor_quirks.ignore_bss_tm_candidates（struct bss 偏移 0xd1）崩掉。
- debuginfod 取到 DWARF 后 eu-addr2line 把 0x2c889 精确映射到 station.c:3275 那一行解引用表达式。
- git blame 确认该行来自 ffe79bfada333，且 3.9 没有、3.10 起有。

为什么"最近一直都在崩"

这是确定性的代码缺陷，不是内存随机损坏。触发条件：wlan0 处于未 CONNECTED 状态（connected_bss == NULL，比如 802.11 关联刚完成、正在
CONNECTING）时，收到一帧 BTM Request。这类帧的监视在 netdev 一创建就注册了，你的 AP（192.168.2.1 那个网络）如果带 band steering /
802.11v，会在客户端关联瞬间下发 BTM 把客户踢到另一频段——于是每次尝试连这个 WiFi 都会在关联窗口崩一次。8/27 旧系统残留 core、8/31 新系
统开始一路崩到今天，都是同一个原因（8/31 之前该内核日志没留这么久）。另外注意这也能被同网段伪造帧远程触发（地址校验 hdr->address_2 ==
connected_bss 在解引用之后），可作 DoS 用。

修复建议

1. 等上游/Fedora 修复：目前 master 就是 3.12，没人修。建议报给 Fedora bugzilla（附本分析）或 iwd 上游 linux-wireless 渠道。
2. 自己打补丁重装：最小修复是把状态检查挪到解引用之前：

```c
  static void station_ap_directed_roam(...)
  {
     ...
     bool can_roam = !station_cannot_roam(station);
     bool ignore_candidates =
        station->connected_bss->vendor_quirks.ignore_bss_tm_candidates;  // 删

     l_debug("ifindex: %u", ...);

     if (station->state != STATION_STATE_CONNECTED) {
        l_debug("roam: unexpected AP directed roam -- ignore");
        return;
     }
     ignore_candidates =
        station->connected_bss->vendor_quirks.ignore_bss_tm_candidates;  // 移到这里
     ...
```

station_cannot_roam() 本身不解引用 connected_bss，所以状态检查放最前、两个 bool 都留在 CONNECTED 分支后取即可。

3. 临时降级到 3.9（唯一没有该 bug 的最近版本）也可以，但少了不少改动。

需要的话我可以直接在你机器上用 Fedora SRPM 打上这个补丁、rpmbuild 出一个修复版 iwd 装上去验证不再崩。要我动手吗？


这个项目的源码在哪里?


Thinking...

说的是 iwd 的源码吧。刚才排查时我临时 clone 到 /tmp/iwd（上游 git.kernel.org），按惯例挪到 ~/data 下并 checkout 出来：


$ mv /tmp/iwd /home/martins3/data/iwd 2>/dev/null || echo "already moved"; cd /home/martins3/data/iwd && git checkout -q HEAD && git
log --oneline -1 && ls src/station.c src/frame-xchg.c 2>/dev/null && du -sh .git (timeout 120s)

d003d0e59332 Release 3.12
src/frame-xchg.c
src/station.c
12M   .git

Took 0.0s


Thinking...

iwd 源码已整理好，位置如下：

上游 iwd 仓库（已 clone 到本地）
- 路径：/home/martins3/data/iwd
- 当前 HEAD：d003d0e59332 Release 3.12（与机器上装的版本一致，即含 bug 的版本）
- 关键文件：src/station.c（station_ap_directed_roam() / ap_roam_frame_event()）、src/frame-xchg.c（frame_watch_unicast_notify()）
- 上游地址：https://git.kernel.org/pub/scm/network/wireless/iwd.git

与 Fedora 安装包完全一致的一份源码（排查时从 debuginfod 拉的，带行号对应关系）：
- /home/martins3/.cache/debuginfod_client/cd0a5db4.../source-c5edad12-#usr#src#debug#iwd-3.12-1.fc44.x86_64#src#station.c

如果你是想基于 Fedora 打包方式打补丁重装（对应我上一条提的修复方案），还需要 Fedora 的 SRPM（含 spec 和补丁体系）：
- https://kojipkgs.fedoraproject.org/packages/iwd/3.12/1.fc44/src/iwd-3.12-1.fc44.src.rpm

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
