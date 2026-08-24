# QEMU migration parameter
<!-- a2fadc74-c29b-4e8b-8144-1b617e967b05 -->

Migration parameter 通过 QMP `migrate-set-parameters` 设置、通过 `query-migrate-parameters`
读取；一个请求可以只 携带要改动的字段：

```json
{
  "execute": "migrate-set-parameters",
  "arguments": {
    "max-bandwidth": 1073741824,
    "downtime-limit": 200,
    "multifd-channels": 8
  }
}
```

数值的单位不要想当然：带宽均为 **bytes/s**，时间均为 **ms**；HMP 显示时可能做了
人类可读转换。实际值和编译期可用的压缩 backend，应以两端的
`query-migrate-parameters` 与 `query-migrate-capabilities` 为准。

## 一览

| 类别         | 参数                         | 默认值           | 作用 / 关键前提                                                                                                                                                                                 |
| ------------ | ---------------------------- | ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 自动收敛     | `throttle-trigger-threshold` | 50 (%)           | 一秒窗口中 dirty bytes / transferred bytes 达到此比率、连续两次触发后，开始或加重节流；只在 `auto-converge` 或 `dirty-limit` 开启时有意义。范围 1--100。                                        |
| 自动收敛     | `cpu-throttle-initial`       | 20 (%)           | `auto-converge` 第一次触发时的 vCPU throttle。范围 1--99。                                                                                                                                      |
| 自动收敛     | `cpu-throttle-increment`     | 10 (%)           | 每次仍未收敛时增加的 throttle。范围 1--99。                                                                                                                                                     |
| 自动收敛     | `cpu-throttle-tailslow`      | `false`          | 尾段按估算的理想 throttle 缩小增量，避免固定增量过冲。                                                                                                                                          |
| 自动收敛     | `max-cpu-throttle`           | 99 (%)           | throttle 上限；不得小于 `cpu-throttle-initial`，不得大于 99。                                                                                                                                   |
| 脏页限速     | `x-vcpu-dirty-limit-period`  | 1000 ms          | KVM dirty-limit 的控制周期，1--1000；实验性。                                                                                                                                                   |
| 脏页限速     | `vcpu-dirty-limit`           | 1 MB/s           | 各 vCPU 的 dirty-rate 配额；要求 `dirty-limit` capability 和 KVM dirty ring。                                                                                                                   |
| 带宽/停机    | `max-bandwidth`              | 128 MiB/s        | precopy 的发送 rate limit；迁移运行中修改会立即更新当前限速器。                                                                                                                                 |
| 带宽/停机    | `max-postcopy-bandwidth`     | 0（不限）        | postcopy 后台推送的带宽上限；**不**限制按需 page request，因此瞬时总流量仍可超过它。                                                                                                            |
| 带宽/停机    | `downtime-limit`             | 300 ms           | 可接受的最大 stop-and-copy 时间；范围 0--2,000,000 ms。它是 switchover 决策目标，不是对真实 downtime 的硬保证。                                                                                 |
| 带宽/停机    | `avail-switchover-bandwidth` | 0（自动估计）    | 为 switchover 决策提供可用带宽。只影响剩余数据能否在 `downtime-limit` 内发送的计算，并不在 switchover 阶段限速。                                                                                |
| multifd      | `multifd-channels`           | 2                | 并行 RAM 数据通道数（1--255）。开启 `multifd` 后，源、目的端必须在 incoming 启动前设置为相同值。                                                                                                |
| multifd      | `multifd-compression`        | `none`           | `none`、`zlib`，以及构建支持时的 `zstd`、`qatzip`、`qpl`、`uadk`。这是 multifd 的编解码 backend，双方必须匹配。压缩与 TLS 会排斥 `zero-copy-send` 和 `mapped-ram`。                             |
| multifd      | `multifd-zlib-level`         | 1                | zlib level，0--9；只在 compression 为 zlib 时生效。                                                                                                                                             |
| multifd      | `multifd-zstd-level`         | 1                | zstd level，0--20；只在 compression 为 zstd 时生效。                                                                                                                                            |
| multifd      | `multifd-qatzip-level`       | 1                | QATzip level，1--9；只在 compression 为 qatzip 时生效。                                                                                                                                         |
| 页面编码     | `xbzrle-cache-size`          | 64 MiB           | XBZRLE 旧页 cache；必须是 target page size 的整数倍且为 2 的幂。迁移运行中改变会 resize cache。需先开启 `xbzrle`，且与 multifd / RDMA / mapped-ram 冲突。                                       |
| 页面编码     | `zero-page-detection`        | `multifd`        | `none` 不检查；`legacy` 由主迁移线程检查；`multifd` 在 multifd sender 中检查，否则退化为 legacy。检查到零页后发送零页标记而不是页面内容。                                                       |
| RDMA         | `x-rdma-chunk-size`          | 1 MiB            | RDMA registration 切块大小；必须是 1 MiB--1 GiB 的 2 次幂，且源、目的端必须相同；实验性。                                                                                                       |
| 文件         | `direct-io`                  | `false`          | 尽可能以 `O_DIRECT` 打开 migration file。有效条件是 `mapped-ram` **且** `multifd`，还取决于 QEMU 构建是否支持。也意味着文件/缓冲区对齐限制。                                                    |
| TLS          | `tls-creds`                  | 空字符串（明文） | TLS credential object ID。source 要是 client credential，destination 要是 server credential；两端各自设置。                                                                                     |
| TLS          | `tls-hostname`               | 空               | source 用它校验 destination X.509 身份；空时尝试从 migration URI 提取 hostname。`fd:`、`exec:` 等 URI 通常必须显式提供。                                                                        |
| TLS          | `tls-authz`                  | 空（拒绝）       | destination 使用的 `authz` object ID，用客户端证书 DN 作访问控制；对象在连接时解析，可替换。                                                                                                    |
| 网络切换     | `announce-initial`           | 50 ms            | 目的端恢复后发送第一批 gratuitous ARP/RARP 前的延迟。范围 0--100000。                                                                                                                           |
| 网络切换     | `announce-max`               | 550 ms           | self-announce 两包之间的最大延迟。范围 0--100000。                                                                                                                                              |
| 网络切换     | `announce-rounds`            | 5                | self-announce 总轮数。范围 0--1000。                                                                                                                                                            |
| 网络切换     | `announce-step`              | 100 ms           | 每轮增加的延迟。范围 1--10000。                                                                                                                                                                 |
| block bitmap | `block-bitmap-mapping`       | 未设置：同名映射 | 将本端 block node / bitmap name 映射成 stream alias，可选改变 bitmap 的 persistent 属性。映射必须一对一；源、目的两端各按本端名字配置到同一 alias。仅在 `dirty-bitmaps` capability 开启时消费。 |
| 工作流       | `mode`                       | `normal`         | `normal`，或同机 CheckPoint and Restart 的 `cpr-reboot`、`cpr-transfer`、`cpr-exec`。它改变迁移流程，不是传统 precopy 的性能开关；CPR 与 postcopy、background snapshot、COLO 冲突。             |
| 工作流       | `cpr-exec-command`           | 未设置           | `cpr-exec` 时替换当前 QEMU 的 argv（第一个元素为程序）。`mode=cpr-exec` 必须设置，且仅 source 使用。                                                                                            |
| COLO         | `x-checkpoint-delay`         | 20,000 ms        | periodic COLO checkpoint 间隔；实验性，只在 COLO 使用。                                                                                                                                         |

## 如何分端设置、何时设置

parameter 是每个 QEMU 进程本地的 `MigrationState.parameters`，**不会作为一套通用
参数表自动协商或同步到对端**。因此不能只在 source 调一次就假定 destination 已经
知道这些值。按消费者可分为：

| 设置位置                            | 参数                                                                                                                                                                                                              | 原因                                                                                                                                                                        |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 主要只在 source                     | `max-bandwidth`、`max-postcopy-bandwidth`、`downtime-limit`、`avail-switchover-bandwidth`、auto-converge / dirty-limit 参数、`xbzrle-cache-size`、`zero-page-detection`、`x-checkpoint-delay`、`cpr-exec-command` | 它们控制 source 的发包、脏页控制器或 source 的工作流。                                                                                                                      |
| 主要只在 destination                | `announce-*`、`tls-authz`                                                                                                                                                                                         | 它们在 destination 的网络重新宣告和入站 TLS 授权时消费。                                                                                                                    |
| 两端均需配置，但值/角色不一定相同   | `tls-creds`、`tls-hostname`、`block-bitmap-mapping`、`mode`                                                                                                                                                       | TLS 的 client/server credential 本来不同；bitmap mapping 的本端 node 名也可能不同；CPR incoming 也必须知道 mode。                                                           |
| 两端同值、且必须在连接/建通道前配置 | `multifd-channels`、`multifd-compression` 及其 level、`x-rdma-chunk-size`、通常还有文件场景的 `direct-io`                                                                                                         | 对端需以同一 channel 数、消息格式、压缩 backend 或 RDMA chunk 算法解析。`multifd` receiver 会用本地 `multifd-channels` 检查 channel ID，并按本地 compression 选择 decoder。 |

实操上，除少数明确可动态调的项外，均应在 source 执行 `migrate` 和 destination
开始 incoming 前设置完成。QMP 对 parameter 没有 capability 那样的统一“迁移中禁止
修改”门槛：它先将请求套到完整候选配置、检查值域和当前组合，再写入。因此，
`max-bandwidth`、`max-postcopy-bandwidth`、`xbzrle-cache-size`
有明确的运行时更新 路径；但已经创建的 multifd channel、TLS 连接、RDMA
registration 不会因为中途改参数 而被安全地重建，不能把“QMP
接受”理解为“本次迁移会按新拓扑运行”。

## 为什么要和 capability 区分

**capability 选择“使用哪一种迁移语义/协议/拓扑”，parameter 在已经选择的语义内指定“以什么数值和策略运行”。**

```text
管理面
  capability:  是否启用一种可选迁移机制？         ─┐
  parameter:   该机制的目标值、容量、阈值？       ─┼─> MigrationState
                                                   │
数据面
  protocol / worker topology / page ownership      │  由 capability 决定
  rate limiter / controller常数 / cache大小        │  由 parameter 决定
```

### capability 是“语义开关”

`migrate-set-capabilities` 操作的是 `bool capabilities[]`。一项 capability 通常会 改变下列至少一种事实：

- migration stream 或协议解释方式：`postcopy-ram` 改成 destination 缺页后由 return path 拉取；`mapped-ram` 让 RAM 落在文件的固定 offset；
- 连接、线程或数据通道拓扑：`multifd` 引入多个 RAM 通道；`return-path` 引入反向通道；
- 生命周期、安全性或正确性前提：`release-ram` 放弃 source 页，`x-ignore-shared` 假定两端共享后端一致，`dirty-limit` 需要 KVM dirty ring。

这类开关不能被当成普通整数调优：开启它可能要求 destination 预先有相应的 worker、
反向连接、host kernel 特性，或让旧的 stream parser 根本无法解析。QEMU 因而先把
请求合并成完整的候选 capability bitmap，再由 `migrate_caps_check()`
做依赖、冲突、 host 支持和时序检查；例如 `zero-copy-send` 要求 `multifd` 且禁止
TLS/压缩， `postcopy-preempt` 要求 `postcopy-ram`，`dirty-limit` 与
`auto-converge` 互斥。 capability 通常必须在迁移前、并在需要该语义的两端一致设置。

“通常”很重要：capability 不是一个自动协商协议。当前 `savevm.c` 只把 `x-ignore-shared` 和 `mapped-ram` 写入 configuration section
并强制核对；其他功能 是否应在对端启用，仍由管理层按版本、transport 和功能要求编排。不要把 `query-migrate-capabilities` 的结果误认为“对端已同意”。

### parameter 是“控制面数值”

`migrate-set-parameters` 操作的是有类型的 `MigrationParameters`
struct：带宽、比例、 毫秒、cache 大小、枚举或对象 ID。它通常不会独立创造新的
VMState wire protocol， 而是给已存在的算法输入常数，例如：

- `max-bandwidth` 改 token/rate limit；`downtime-limit` 和
  `avail-switchover-bandwidth` 参与“剩余页能否在目标停机时间内传完”的估算；
- auto-converge 已由 capability 选中后，`throttle-trigger-threshold`、initial、
  increment、max 构成它的控制律；dirty-limit 也由 capability 选中不同的 KVM
  控制器， 参数只指定周期与 MB/s 配额；
- `multifd` 由 capability 决定“存在多通道”，`multifd-channels` 决定通道数，
  `multifd-compression`/level 决定每通道 codec；
- `mapped-ram` 决定固定-offset 的文件布局，`direct-io` 只决定在此布局内是否尝试
  `O_DIRECT`。

这种划分避免了两种常见问题：把协议兼容性混成一堆可任意热改的数值，或为每个可调
阈值都增加一个 capability 位和组合爆炸的兼容矩阵。它也使 orchestration 的职责
更清晰：先协商/校验两端可以采用的 **capability 集合**，再按链路、CPU、脏页率与
业务 downtime SLO 调整 **parameter 数值**。

## 建议的配置顺序

1. 两端查询 capability 与 parameter，确认版本/构建/host 限制。
2. 先设置并校验 capability：例如 `multifd`、`postcopy-ram`、`mapped-ram`、TLS 相关约束；destination 必须先进入能接收该拓扑的状态。
3. 再设置属于该路径的 parameter：带宽、通道数、压缩、cache、收敛策略等；对需要一致解释的参数，显式在两端设定并回读核对。
4. 发起 migration；迁移中只动态改有明确运行时语义的 rate/cache 参数，并用 `query-migrate` 的实际 throughput、remaining、dirty-sync 与 downtime 结果闭环。

相关源码：`qapi/migration.json` 定义 QMP 类型和语义，`migration/options.c` 负责 parameter 的 defaults、完整候选配置校验与应用，`migration/ram.c`
消费脏页、零页、 XBZRLE、节流参数，`migration/multifd.c` 消费 multifd 相关参数。

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
