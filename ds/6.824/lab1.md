# Lab 1 : MapReduce
https://pdos.csail.mit.edu/6.824/labs/lab-mr.html


# 分析

The worker implementation should put the output of the X'th reduce task in the file `mr-out-X`
> 首先将任务被分给 worker , 第 i 个 work 产生的中间结果在 mr-i-X 中间
> 一个 worker 接受到 reduce 任务 X 的时候，将 `mr-*-X` 的中间结果全部收集起来，最后生成为 `mr-out-X`

当前的调用方式 server 总是被动的，worker 总是在等待任务。

- [ ] worker 在周期的询问 coordinator 有没有事情发生，然后 coordinator 在自己的 go routine 中间发送任务其
  - [x] 如果 map 的工作没有完成，那么是不会进行到 reduce 阶段的
    -  e.g. reduces can't start until the last map has finished.

Map 产生中间文件，是 json 格式的, sequential 版本没有如此使用，因为在一个进程中间，最后直接使用即可

- 当所有的事情都完成之后，worker 请求工作收到的消息就是 please exit 了
- [ ] 是不是 coordinator 需要确认一下 worker 的确 exit 的通知
  - 这是 RPC 的功能吧，如果 worker 没有接收到 coordinator 的消息，那么就自己需要再次发送
  - coordinator 如果感觉到任务都结束了（所有发送的任务都得到了回复）, 那么就应该停止了
  - worker 可能不断的发送消息请求结果, 所以正确的处理方法就是 worker 发送 rpc 失败，说明 coordinator exit 了，那么就可以结束了

- [x] 每次是回复一个文件还是多个文件啊 !
  - 一个文件吧，只是传输一个文件，没有什么损耗，只是和 coordinator 多交互几次
  - [ ] 使用 rename 有个问题 : 如果只是 rename 了一半，怎么办 ?
  - 真的需要 rename 吗 ? 只是完成了一般的文件怎么会被被人读去 ?
    - 也许只是用于 debug 的吧 !

- [ ] 如何创建一个 timer 来等待 worker, 防止有的 worker 在中途死掉了
  - For this lab, have the coordinator wait for ten seconds;

- 如果当前没有任务提供给 worker，在 coordinator 中间等待会不会导致 worker 认为自己的 RPC 死掉了 ?
  - 并不会，如果会，那就是 RPC 的设计是存在问题的，但是为什么 RPC 不会出现问题，现在还没有分析。

- 可以从 sequential 搞到获取所有的 input files 的方法

- 其实，应该是存在两次文件类型的装换

- The map part of your worker can use the ihash(key) function (in worker.go) to pick the reduce task for a given key.

- [ ] 如果我认为一个 worker 挂掉了，并且将任务交给其他人，怎么办 ?

- 一个 worker 请求工作 和 说自己的工作完成了，其实效果是相同的, 都需要 wait



# 记录
1. enum : https://stackoverflow.com/questions/14426366/what-is-an-idiomatic-way-of-representing-enums-in-go
2. https://stackoverflow.com/questions/40256161/exported-and-unexported-fields-in-go-language

3. go func( ) 的参数含义是什么 ?

4. go 的 memory model : 
  - https://go101.org/article/memory-model.html


- [ ] 为什么会报错啊
```golang
func main() {

	messages := make(chan string)

	messages <- "fuck"

	go func() {
			str := <-messages
			fmt.Println(str)
	}()
}
```

- https://stackoverflow.com/questions/53404305/when-to-use-var-or-in-go/53404567
