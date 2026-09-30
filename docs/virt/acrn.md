# ACRN

## 为什么写这个

我从 2021 一直在注意到 acrn ，但是始终没上手看看，2026-09-21 问了下 ai ，
发现这个项目已经凉了
- https://github.com/projectacrn/acrn-hypervisor

> [!NOTE]
> 参考神奇海螺的意见，有待验证

Intel 2018 年 2 月开源的嵌入式/IoT 用轻量 Type-1 "参考 hypervisor"，Linux
Foundation 项目，实际开发主力是 Intel 上海 OTC 团队。2024 年 10
月发布最后一版后事实性停摆。本文为 2026-09-21 的调查记录（数据来自 clone 到
`~/data/acrn-hypervisor` 的本地 git 统计、GitHub API、官方 roadmap PDF 的
Wayback 快照）。

## 现状：项目已事实性死亡

master 每年提交数：

| 年份   | 2018 | 2019 | 2020-2022  | 2023 | 2024 | 2025 | 2026                 |
| ------ | ---- | ---- | ---------- | ---- | ---- | ---- | -------------------- |
| 提交数 | 2736 | 1912 | 约 1100/年 | 147  | 107  | 14   | 0（只剩 dependabot） |

时间线：

| 时间       | 事件                                                                                                                    |
| ---------- | ----------------------------------------------------------------------------------------------------------------------- |
| 2018-02    | Intel 开源，LF 项目，主打 IoT/嵌入式 "reference hypervisor"                                                             |
| 2023-07    | 按周滚动的 CI 发布（`acrn-2023wXX` tag）在 w29 停止，此后每年只发一个版本                                               |
| 2024-08    | v3.3 发布（最后一个特性版本：Main VM、vHWP、Celadon 支持）                                                              |
| 2024-10    | 最后一个 tag `acrn-2024w42`，之后再无 release                                                                           |
| 2025-09~11 | Intel 中国团队在 `multi-arch-dev` 分支做 RISC-V 移植：9 月 113 个、10 月 63 个提交，11 月 7 日戛然而止，从未合入 master |
| 2026       | issue 区只剩 dependabot；官网博客停在 v3.3/Zephyr 文章；官网上的 2024 roadmap PDF 链接本身已 404                        |

关键物证：官方 2024 年 9 月的
[roadmap PDF](https://web.archive.org/web/2024/https://projectacrn.org/wp-content/uploads/sites/63/2023/09/ACRN-Roadmap-External-2024.pdf)（Wayback
快照）明确规划了 v3.4：Intel Thread Director/HFI 虚拟化、QCOW2 镜像、TPM
虚拟化、Guest S3、"Continuous Project: RISC-V Support"。v3.3 如期发布，v3.4
永远没有出现。计划断在半路，是典型的 "团队被裁" 而非 "项目做完"。

## 为什么凉了

### 1. 单厂商项目，社区从未存在过

- 历史贡献者前 15 名清一色 `@intel.com`（David Kinder、Victor Sun、Junjie Mao 等）
- Hacker News 上关于 ACRN 的全部历史只有 2018 年的发布通告（4 个赞）
- Wikipedia 连独立词条都没有
- issue 区用户问题零回复就被 close，只剩 dependabot
- 名义会员（ADLINK、TTTech、LG 等）是客户不是开发者

Intel 一撤人，项目没有第二个引擎，直接熄火。

### 2. Intel 收缩周期与数据严丝合缝

- 2022-10 宣布大幅成本削减 -> ACRN 周发布恰好在 2023-07 停掉
- 2024-08 宣布 1.5 万人裁员 -> 最后 release 出现在 2024-10
- 同期 Intel 的 Clear Linux 仓库（`clearlinux/clr-bundles`）已被正式
  archived，Celadon（Intel 的 Android 参考栈，恰好是 ACRN v3.3
  刚支持的目标）也在 2025 年陆续停更
- 2025 年的 RISC-V 移植是最后的自救尝试，11 月无疾而终

ACRN 只是 Intel 开源瘦身清单上的一员。

### 3. 产品与架构的结构性问题

- "reference hypervisor" 模式：绑定特定 Intel 参考板（NUC、Whiskey Lake、Tiger
  Lake 等），每换一代平台都要团队手工做 board enablement，Intel 停手就没人能跟进
- 上游化程度极低（见下节），发行版不集成，用户必须按文档在固定的 Ubuntu 22.04
  上自己构建整套栈
- 配置复杂（board.xml + scenario.xml + configurator），上手成本远高于 "装个 KVM"
- 需求被替代：PREEMPT_RT 于 2024 年进入主线（6.12），KVM
  生态全面覆盖，真要功能安全认证的客户买 Wind River/QNX/PikeOS 这类商业方案
- 佐证：同品类的 thin x86 hypervisor Jailhouse 也在同期死亡，整个品类被市场淘汰

## 内核里的 ACRN 代码：两处，都不是本体

内核树里有两处 ACRN 相关代码，都不是 hypervisor 本体：

1. `arch/x86/kernel/cpu/acrn.c`：ACRN_GUEST，Linux 作为 ACRN guest
   运行时的探测与回调向量设置（5.3 进入主线）：

```txt
config ACRN_GUEST
	bool "ACRN Guest support"
	depends on X86_64
	select X86_HV_CALLBACK_VECTOR
	help
	  This option allows to run Linux as guest in the ACRN hypervisor. ACRN is
	  a flexible, lightweight reference open-source hypervisor, built with
	  real-time and safety-criticality in mind. It is built for embedded
	  IOT with small footprint and real-time features. More details can be
	  found in https://projectacrn.org/.
```

2. `drivers/virt/acrn`：HSM（Hypervisor Service Module），含
   `hsm.c`、`vm.c`、`mm.c`、`ioreq.c`、`irqfd.c`、`ioeventfd.c`、`hypercall.h`；ioctl
   接口在 `include/uapi/linux/acrn.h`。MAINTAINERS 条目 "ACRN HYPERVISOR SERVICE
   MODULE" 仍标 `S: Supported`（Fei Li, Intel）。

内核侧现状：仍在树内、被动维护（2024 年 follow_pfn API 迁移，2026-05 irqfd
use-after-free 修复均为社区顺手而为）；lore.kernel.org
上没有找到任何移除提案。真正对应 KVM 核心（`arch/x86/kvm` 里的
vCPU/VMCS/EPT/中断注入/指令模拟）的东西全部在树外的
[acrn-hypervisor](https://github.com/projectacrn/acrn-hypervisor) 仓库里。

## 与 KVM 的关系：并列互斥的两套栈

ACRN 不依赖 KVM，是完全平行的另一套虚拟化方案，同一台机器启动时二选一：

|                 | KVM 路线                                  | ACRN 路线                                                                                |
| --------------- | ----------------------------------------- | ---------------------------------------------------------------------------------------- |
| hypervisor 本体 | Linux 内核模块（`arch/x86/kvm/`），在树内 | 独立二进制（`acrn.bin`/`acrn.efi`），树外，由 GRUB/UEFI 经 multiboot2 在任何 OS 之前加载 |
| 内核里的角色    | 本体就在内核里                            | 主线里只有 `drivers/virt/acrn`（HSM），跑在 Service VM 内核里                            |
| 用户态          | QEMU                                      | acrn-dm                                                                                  |
| 宿主 OS         | Linux 自己就是宿主                        | Linux 跑在 ACRN 的第一个 VM（Service VM）里，用的是 acrn-kernel fork                     |

启动顺序：固件 -> GRUB 用 multiboot2 协议加载 ACRN 的 ELF/RAW
二进制（`acrn.32.out`/`acrn.bin`，见仓库文档 `doc/tutorials/using_grub.rst`）->
ACRN 在裸金属上起来，直接启动 Service VM（定制内核的 Linux），预启动
VM（RTOS、TEE 等）按分区配置拉起。

主线里那个 HSM 驱动角色上等价于 KVM 世界的 `/dev/kvm`：把用户态 acrn-dm 的 ioctl
翻译成 `vmcall` hypercall 去调下层的 ACRN 本体。

两个附加事实：

- 主线 HSM 只是缩水版。完整功能（asyncio、acrntrace、PIO 直通、hypervisor 日志）从来没进主线，只存在于
  [acrn-kernel](https://github.com/projectacrn/acrn-kernel) 这个 Linux fork
  里。也就是说 ACRN 的 Service VM 内核本身就是个必须维护的分支，从 4.19 一路 fork 到现在
- ACRN 不向 Guest 暴露 VMX，所以没法在 ACRN 的 VM 里再嵌套跑 KVM，两套生态没有互通的桥

这个架构差异正是生死分野的根源：KVM 的本体活在内核树内，Intel 撤了还有
AMD、Google、Red Hat 的工程师在养；ACRN 三层（hypervisor 本体、Service VM
内核、用户态 dm）全是 Intel 独家，上游社区最多顺手维护那个薄薄的 HSM
驱动。同一笔虚拟化预算，投进树内的部分有人接盘，投进树外的部分没人接盘。

## 残存维护（截至 2026-03）

- [acrn-kernel](https://github.com/projectacrn/acrn-kernel) 的 master 停在
  2025-02，但 `6.12/linux` 和 `6.17/linux` 分支在 2026-03 仍有 Yifan Liu
  的提交：给 PTL（Panther Lake，Intel 下一代平台）板子加 Service VM
  kconfig，以及跟着新内核 API 修 HSM 编译错误
- 结合 `multi-arch-dev` 分支 2025 年 9-11 月的 RISC-V
  移植冲刺（同为这批人：Yifan Liu、Jian Jun Chen、Haoyu Tang 等，213
  个提交，止于 2025-11-07），说明
  Intel（中国）还留着一个小分队在维护这条线，大概率服务某个还在用 ACRN
  的内部或客户项目

结论修正：主仓库 2026 年零提交、release 断在 2024-10 不变，但 "彻底无人"
应修正为
"退化成不再对外发布、跟着特定项目走的维护小组"。对一个开源项目而言这等于临床死亡：master
不再是开发主线，发布流程停摆，外人无从参与（要参与就得连内核 fork 一起接手）。

## 链接

- 仓库：<https://github.com/projectacrn/acrn-hypervisor>
- Service VM 内核 fork：<https://github.com/projectacrn/acrn-kernel>
- 官网：<https://projectacrn.org>（内容停在 2024）
- 2024 roadmap
  PDF（Wayback）：<https://web.archive.org/web/2024/https://projectacrn.org/wp-content/uploads/sites/63/2023/09/ACRN-Roadmap-External-2024.pdf>
- 2018
  发布通告：<https://www.linuxfoundation.org/press-release/the-linux-foundation-announces-an-open-source-reference-hypervisor-project-designed-for-iot-device-development/>

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
