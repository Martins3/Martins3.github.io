We were motivated to develop a new hardware language by years of struggle with existing hardware description languages in our research projects and hardware design courses. Verilog and VHDL were developed as hardware simulation languages, and only later did they become a basis for hardware synthesis


In Chisel, a raw collection of bits is represented by the *Bits* type.

A Scala apply constructor can be defined so that a user datatype also does not require new, as described in Function Constructor.

One important note is that Chisel distinguishes between types (like UInt) and hardware nodes (like the literal 2.U, or the output of myReg). While
```
val myReg = Reg(UInt(2.W))
```
is legal because a Reg needs a data type as a model,
```
val myReg = Reg(2.U)
```
is an error because 2.U is already a hardware node and can't be used as a model.

[what does flipped do in chisel3](https://stackoverflow.com/questions/48343073/what-does-flipped-do-in-chisel3)


## 问题
1. Verilog是如何实现shifter的

从数字电路的角度，是如何实现shifter的?
> in--------> 触发器A -------> 触发器B
> 如果在一个时钟，in的信号首先修改了 A的输出，并且由于时钟含有延迟，导致B接受到时钟的时候，A的输出已经变化，那么AB的数值都是in的信号

2. 为什么需要使用RegNext函数，不就是普通的赋值吗?

3. 2.5 exercise的最后部分完全没有看懂啊

4.来自Scala和JVM的问题

Type matching has some limitations. Because Scala runs on the JVM, and the JVM does not maintain polymorphic types, you cannot match on them at runtime (because they are all erased). Note that the following example always matches the first case statement, because the [String], [Int], and [Double] polymorphic types are erased, and the case statements are actually matching on just a Seq.
```
val sequence = Seq(Seq("a"), Seq(1), Seq(0.0))
sequence.foreach { x =>
  x match {
    case s: Seq[String] => println(s"$x is a String")
    case s: Seq[Int]    => println(s"$x is an Int")
    case s: Seq[Double] => println(s"$x is a Double")
  }
}
```

5. `<>`
[尚未阅读](https://github.com/freechipsproject/chisel3/wiki/Interfaces-Bulk-Connections)


## Print调试
1. 2.2_comb_logic.ipynb 为什么在这里simulation阶段没有输出
```
class Arbiter extends Module {
  val io = IO(new Bundle {
    // FIFO
    val fifo_valid = Input(Bool())
    val fifo_ready = Output(Bool())
    val fifo_data  = Input(UInt(16.W))
    
    // PE0
    val pe0_valid  = Output(Bool())
    val pe0_ready  = Input(Bool())
    val pe0_data   = Output(UInt(16.W))
    
    // PE1
    val pe1_valid  = Output(Bool())
    val pe1_ready  = Input(Bool())
    val pe1_data   = Output(UInt(16.W))
  })
    io.fifo_ready := io.pe0_ready || io.pe1_ready
    io.pe0_valid := io.fifo_valid && io.pe0_ready
    io.pe1_valid := io.fifo_valid && (!io.pe0_ready)
    
    printf("Print during simulation: Input is %d\n", io.fifo_valid)
    // chisel printf has its own string interpolator too
    printf(p"Print during simulation: IO is $io\n")

    
    io.pe0_data := Mux(io.pe0_valid, io.fifo_data, 0.U)
    io.pe1_data := Mux(io.pe1_valid, io.fifo_data, 0.U)
    
}
class ArbiterTester(c: Arbiter) extends PeekPokeTester(c) {
  import scala.util.Random
  val data = Random.nextInt(65536)
  poke(c.io.fifo_data, data)
  
  for (i <- 0 until 8) {
    poke(c.io.fifo_valid, (i>>0)%2)
    poke(c.io.pe0_ready,  (i>>1)%2)
    poke(c.io.pe1_ready,  (i>>2)%2)

    expect(c.io.fifo_ready, i>1)
    expect(c.io.pe0_valid,  i==3 || i==7)
    expect(c.io.pe1_valid,  i==5)
    println(s"Print during testing: Input is ${peek(c.io.fifo_valid)} "  )
    println(s"Print during testing: Input is ${peek(c.io.pe0_ready)} "  )
    println(s"Print during testing: Input is ${peek(c.io.pe1_ready)} "  )
    
    if (i == 3 || i ==7) {
      expect(c.io.pe0_data, data)
    } else if (i == 5) {
      expect(c.io.pe1_data, data)
    }
  }
}
assert(Driver(() => new Arbiter) {c => new ArbiterTester(c)})
println("SUCCESS!!")
```

@todo chisel3 ub 14 的文档 yinkunzhou 可以阅读

bundle : struct

sodor repo

