# 实际上，当 bmbt 完成之后，我们可以重构出来 bmbt 了呀


## 改动

### 如何做一个 devices 的移植
1. qom
2. qdev
3. qdev_gpio_connect
4. memory region
  - isa_register_ioport
5. VMStateDescription 总是在被直接删除的

### fuck_trace
