## 源码阅读

### generators/boom/src/main/scala/ifu/frontend.scala
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
