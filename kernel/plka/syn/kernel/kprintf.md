# kernel/kprintf

似乎 kprintf 在更高的版本几乎获得了一个重写级别的更新，所以，以后再说吧!
https://lwn.net/ml/linux-kernel/20190212143003.48446-1-john.ogness@linutronix.de/


## 疑惑的问题
1. 为什么用户态可以使用 dmsg 将其中的消息 kernel ring buffer 中间读取出来的 ?
2. 消息的级别 如何快速过滤
3. ringbuffer 如何管理 ?
4. 出现多个 wirter 如何协调 ?
