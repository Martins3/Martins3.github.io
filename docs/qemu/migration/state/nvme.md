# 为什么 QEMU 中不支持

2026-07-27 询问 codex 关于，简单看了，基本都是对的。

## 为什么 NVMe 迁移困难

NVMe 运行状态不仅是几个 PCI 寄存器，还包括：

- SQ/CQ 的地址、head/tail、phase、SQ→CQ 关系
- guest RAM 中尚未处理的 SQE
- 已经完成但因 CQ 满而尚未写入 guest RAM 的 CQE
- 正在执行的异步块 I/O
- PRP/SGL DMA 映射、bounce buffer
- MSI-X、Doorbell Buffer、ioeventfd
- AER 请求和待上报事件
- namespace attachment、Zoned、FDP、CMB、PMR、SR-IOV 等状态

其中 BlockAIOCB *、QEMUSGList、BH、host
指针不能直接序列化。更严重的是，如果把一条已经落盘、但 CQE 尚未送给 guest 的
WRITE 在目标端重新执行，会产生重复写； COPY、DSM、Zone Append 的重复执行风险更大

所以迁移点必须建立下面的边界：

停止获取新 SQE ↓ 排空所有已提交的后端 AIO ↓ 把能写入的 CQE 写入 guest RAM ↓
保存仍因 CQ 满而无法投递的 CQE ↓ 迁移设备状态 ↓ 目标端重建队列/BH/ioeventfd ↓
继续投递 CQE、继续处理未消费的 SQE

## 当前代码是怎么支持的

2026 年 7 月合入的核心提交是：

- 5320d335fb8d：为未支持的 NVMe 功能安装 migration blocker
- 77c2c3eb01d0：拆分 SQ/CQ 初始化，以便目标端重建
- 5a1f41c66ad7：更早设置 CQE 的 SQ ID
- 4e868b806e51：完成请求时尽早解除 SGL 映射
- 9d378cab537d：基础 NVMe live migration

迁移前，hw/nvme/ctrl.c:nvme_ctrl_pre_save 做了这些工作：

1. 在 BQL 保护下取消所有 SQ BH，停止获取新命令
2. 对 namespace 调用 blk_drain()，等待所有在途后端 I/O 完成
3. 尽量把完成请求写入 guest CQ
4. CQ 已满时，把还不能投递的 NvmeRequest 保存到迁移流
5. 单独检查和保存 outstanding AER
6. 验证不存在不可序列化的 aiocb、SGL 映射、opaque context

VMState 在 static const VMStateDescription nvme_vmstate = { 中保存：

- PCI/MSI-X 状态
- NVMe BAR 寄存器
- 控制器 feature
- SQ/CQ 的地址、大小、head/tail/phase
- CQ 满时未投递的 completion
- AER 请求和事件
- namespace Identify/format/cache/atomic 状态

目标端的 hw/nvme/ctrl.c:nvme_ctrl_post_load 会：

- 先重建 CQ，再重建 SQ
- 重新创建 BH、SQ/CQ 关联和 ioeventfd
- 把迁移来的 completion 放回 SQ 的 request pool
- 恢复 AER
- 重新 attachment namespace
- 调度 CQE 投递和 SQ 处理

这套设计的重点是：不迁移后端 AIO，也不重放已经完成的 I/O；所有在途
I/O都在源端排空

## 当前能迁移什么

目前实际支持的是最基础的这种配置：

-device nvme,drive=drv0,serial=...

也就是：

- 单个 controller
- controller 内嵌的单个传统 namespace
- 普通 NVM read/write/flush/compare/copy/DSM 等命令
- SQ/CQ、MSI-X、AER、DBBUF 等基础状态

尤其要注意，hw/nvme/ctrl.c:10271 要求 namespace 必须是
n->namespace。因此即使只有一个独立的 nvme-ns
设备，也可能在迁移最后阶段失败，不仅仅是“数量不能超过一个”

当前 blocker 明确禁止以下配置，见 hw/nvme/ctrl.c:9366：

- 多 namespace / Namespace Attachment
- Zoned Namespace
- FDP
- CMB
- PMR
- SPDM
- SR-IOV
- 未知的新 capability

另外 SMART 的 I/O 统计来自 BlockAcctStats，当前不会完整保留；温度和 critical
warning 等部分状态会迁移

## 如果要把支持做完整

建议按下面顺序扩展

### 1. 多 namespace

这是最适合首先完成的部分：

- 不再只迁移内嵌的 n->namespace
- 为 subsystem 建立 VMState
- 保存 controller 的 namespaces[] attachment bitmap、changed_nsids
- 保存每个 namespace 的 NSID、attached/private/shared 关系
- 目标端按 NSID 找到已 realize 的 namespace，再重建指针关系
- shared namespace 只保存一次，避免多个 controller 重复序列化
- 删除 pre-save 的单 namespace 限制，最后再解除 blocker

配置拓扑仍应由目标端命令行预先创建，迁移流只恢复运行状态

### 2. Zoned Namespace

需要迁移的不只是当前 nvme_vmstate_ns 中的几个 ZRWA 参数，还包括：

- 整个 zone_array
- 每个 zone 的 state、write pointer、attributes
- zone descriptor extension
- open/closed/full 队列的成员关系
- nr_open_zones、nr_active_zones
- ZRWA 资源状态

必须保持当前“源端 drain 后再保存”策略，不能在目标端重新执行 Zone Append

### 3. FDP

应在 NvmeSubsystem 层保存：

- RU handle 和 placement handle 映射
- 每个 reclaim group 的 RU 状态及 ruamw
- FDP host/controller event buffer
- hbmw/mbmw/mbe 等计数器
- namespace 的 fdp.phs

### 4. CMB/PMR

BAR 寄存器已经在 VMState 中，但实际内存内容没有迁移。需要：

- 将 CMB buffer 注册为可迁移 RAMBlock，或显式保存 buffer
- 恢复 CMB/PMR mapping 和 enable 状态
- 保证目标端在重建 SQ/CQ 前恢复内存
- 如果 PMR 使用共享 host backend，需要定义共享存储语义；否则迁移实际内容

### 5. SR-IOV 和 SPDM

这两项最复杂：

- SR-IOV 需要同时迁移 PF/VF PCI、MSI-X、队列、secondary controller resource
  assignment，并严格控制加载顺序
- SPDM 涉及外部
  socket、认证会话、密码学状态和密钥，通常需要后端配合迁移协议，不能简单保存几个结构体字段

## 关联代码

当前已有两个很有价值的测试：

- tests/functional/x86_64/test_nvme_migration.py：迁移期间持续产生读 I/O
- tests/qtest/nvme-test.c：验证 CQ 满时尚未投递的 completion 能恢复

## 测试结果

QEMU emulator version 11.0.91 (v11.1.0-rc1-48-g05e27e70df42-dirty)

是支持的 nvme 热迁移的，和我再次调查这个问题也就是几周时间。

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
