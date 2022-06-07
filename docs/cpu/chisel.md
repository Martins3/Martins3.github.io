- [入门](https://github.com/freechipsproject/chisel-bootcamp)
  - [chisel 项目模板](https://github.com/freechipsproject/chisel-template)
  - https://stackoverflow.com/questions/53414115/how-to-import-getverilog-function-from-the-bootcamp-examples
- [实战](https://github.com/ucb-bar/chisel-tutorial)
1. [Chisel Document](https://chisel.eecs.berkeley.edu/documentation.html)
> include a Chinese document
2. [Chisel api](https://chisel.eecs.berkeley.edu/api/latest/index.html)

https://github.com/d0iasm/rvemu : 模拟器，可以同时在浏览器和终端中运行(浏览器中运行！！！！
https://marz.utk.edu/my-courses/cosc230/book/example-risc-v-assembly-programs/ : 汇编教程

- https://learnxinyminutes.com/docs/scala/ : scala 语法 check sheet

## 问题
- scala 中 := 和 = 是什么区别?
  - https://stackoverflow.com/questions/51644857/when-we-should-use-not-in-chisel3-same-case-is-when-and-if
    - 前者是将两个线连接起来，后者赋值操作
- 什么是 Bundle 啊?

- [ ] 似乎 scala 的 match 不是很会使用，这个代码为什么会报错

```scala
def states = Map("idle" -> 0, "coding" -> 1, "writing" -> 2, "grad" -> 3)

val nextState = states("idle")
match nextState = {
case states("idle") => states("idle")
}
```

## bootcamp
### 2
- [x] 常量赋值
- [x] 对于 bool 变量如何进行裸机操作
  - [ ] 是没有 if 操作吗?
  - 使用 when .elsewhen 和 .otherwise 维持生活
  - 感觉 sum := a + b 之类的操作，实际上，sum 就是一个 wire 的
  - val sum = Wire(UInt(5.W)) : 只是初始化，而不声明
  - MUX 比 When 更加好用的哦
### 3
- 注意 == 和 === ，前者是 scala 的基本类型的比较，后者才是

Any unlabelled transition (i.e. when there are no inputs) returns a grad student to the Idle state instead of staying in the current state.

- val 和 var 的区别
- [ ] Enum 的用途是什么
```scala
val idle :: coding :: writing :: grad :: Nil = Enum(4)
```

### 4
