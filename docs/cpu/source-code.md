## 源码阅读

- [ ] 整个 配置系统是如何工作的
  - common/config-mixins.scala 中初始化了好几个 class 来实现 predictor class

- [ ] 每一个 stage 之间的 buffer 在哪里
  - 应该是通过 RegNext 来实现的
- [ ] 分析 Icache 是如何被使用的
  - [ ] Icache 如何和下级的 Cache 沟通的
- [ ] 从 ICache 检查指令的流动

## [ ] 将 frontend 每一个阶段读清楚
- F0: 发送请求到 ICache 中
  - 如果 ICache 没有命中，如何?
- F1: 翻译 vpc
  - [ ] 如果 miss, 如何 ?
  - 这里的 TLB 只是翻译指令的吧，因为还存在其他位置吧
  - `val s1_ppc  = Mux(s1_is_replay, RegNext(s0_replay_ppc), tlb.io.resp.paddr)` : 似乎 TLB 是 1 cycle 就可以出来的
- F2: 等待 ICache 的结果

## [ ] 观测 pc 的流动
- FetchBundle::pc 中的数值
- [ ] 翻译
- [ ] 随着指令的执行的更新

## 分析一下 predictor
- [ ] 是不是多个 predictor 在给同一个位置预测
- [ ] 是不是存在多个 predictor 的位置
- [ ] ghist : 有 global history 的，那么岂不是还存在 local history 的?
  - 一个 pc 下的就是 local history 吗?

###  ifu/frontend.scala
```scala
class FrontendResp(implicit p: Parameters) extends BoomBundle()(p) {
```
- [ ] BoomBundle ?
- [ ] Resp 是什么缩写的含义

```scala
class GlobalHistory(implicit p: Parameters) extends BoomBundle()(p)
  with HasBoomFrontendParameters
```
- [ ] with ?

```scala
/**
 * IO for the BOOM Frontend to/from the CPU
 */
class BoomFrontendIO(implicit p: Parameters) extends BoomBundle
{
  // Give the backend a packet of instructions.
  val fetchpacket       = Flipped(new DecoupledIO(new FetchBufferResp))
```
- [ ] 调查一下 val 后面可以增加什么类型，以及各自都是表示什么含义
  - [ ] Flipped
  - [ ] Wire
  - [ ] WireInit
  - [ ] Mux

```scala
  val ras = Module(new BoomRAS)
```
- [ ] 这里的 Module 是什么含义呀

```scala
  // --------------------------------------------------------
  // **** F3 ****
  // --------------------------------------------------------
  val f3_clear = WireInit(false.B)
  val f3 = withReset(reset.asBool || f3_clear) {
    Module(new Queue(new FrontendResp, 1, pipe=true, flow=false)) }
```
- [ ] 这里的 WireInit 和 withReset 无法正确跳转到

### ifu/icache.scala
- [ ] 为什么这个文件中，总是下面的范式:
  - ICacheResp
  - ICacheBundle
  - ICacheModule

```scala
class ICache(
  val icacheParams: ICacheParams,
  val staticIdForMetadataUseOnly: Int)(implicit p: Parameters)
  extends LazyModule
```
- [ ] 什么是 LazyModule 啊？

```scala
class BoomICacheLogicalTreeNode(icache: ICache, deviceOpt: Option[SimpleDevice], params: ICacheParams) extends LogicalTreeNode(() => deviceOpt) {
  override def getOMComponents(resourceBindings: ResourceBindings, children: Seq[OMComponent] = Nil): Seq[OMComponent] = {
    Seq(
      OMICache(
```
- [ ] 这里的 Seq 如何理解哇
- [ ] 似乎 BoomICacheLogicalTreeNode 从来没有被使用过

```scala
  val resp = Valid(new ICacheResp(outer))
```
- [ ] Valid

```scala
  val dataArrays = if (nBanks == 1) {
    // Use unbanked icache for narrow accesses.
    (0 until nWays).map { x =>
      DescribedSRAM(
        name = s"dataArrayWay_${x}",
        desc = "ICache Data Array",
        size = nSets * refillCycles,
        data = UInt((wordBits).W)
      )
    }
```
- [ ] 有趣的 DescribedSRAM 来构建 SRAM

### common/config-mixins.scala
```scala
class WithAlpha21264BPD extends Config((site, here, up) => {
  case TilesLocated(InSubsystem) => up(TilesLocated(InSubsystem), site) map {
    case tp: BoomTileAttachParams => tp.copy(tileParams = tp.tileParams.copy(core = tp.tileParams.core.copy(
      bpdMaxMetaLength = 64,
      globalHistoryLength = 32,
      localHistoryLength = 32,
      localHistoryNSets = 128,
      branchPredictor = ((resp_in: BranchPredictionBankResponse, p: Parameters) => {
        val btb = Module(new BTBBranchPredictorBank()(p))
        val gbim = Module(new HBIMBranchPredictorBank()(p))
        val lbim = Module(new HBIMBranchPredictorBank(BoomHBIMParams(useLocal=true))(p))
        val tourney = Module(new TourneyBranchPredictorBank()(p))
        val preds = Seq(lbim, btb, gbim, tourney)
        preds.map(_.io := DontCare)

        gbim.io.resp_in(0) := resp_in
        lbim.io.resp_in(0) := resp_in
        tourney.io.resp_in(0) := gbim.io.resp
        tourney.io.resp_in(1) := lbim.io.resp
        btb.io.resp_in(0)  := tourney.io.resp

        (preds, btb.io.resp)
      })
    )))
    case other => other
  }
})
```

- [ ] class WithAlpha21264BPD extends Config((site, here, up) => { 的这种写法没见过
