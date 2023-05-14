http://www.ece.lsu.edu/ee4720/2014/lfp.s.html  浮点向量

https://github.com/YosysHQ/yosys
https://www.veripool.org/wiki/verilator

https://avrillion.com/stf/363/How-to-Build-1-Bit-of-RAM-Using-Transistors : 制造一个bit 的RAM

Dennard Scaling

## Memory Hierarchy
1. memory miss rate = 0 => less motivation of out-of-order
2. dma()　参数 tag 是做同步使用的 ? @todo

3. DRAM : 破坏性读，电容 -- 回填/flush
4. DRAM 为什么不用做关键字优先，到底什么是关键字优先?

5. DRAM 结构总结 :
    1. 128bit 的bus 访问两个
    2. DIMM
    3. RANK
    4. DRAM chip
    5. BANK

DIMM is a module that contains one or several random access memory (RAM) chips on a small circuit board with pins that connect it to the computer motherboard.

6. flash 的 table 是放到哪里的 ?
7. data-flow execution flow
