## 资料
```txt
[root@nixos:/sys/kernel/debug/tracing]# cat available_tracers
blk function_graph wakeup_dl wakeup_rt wakeup function nop
```
- 可以勉强读读的内容:
  - https://static.lwn.net/images/conf/rtlws11/papers/proc/p02.pdf

这个是存在具体代码的跟踪的:
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/08/05/tracing-basic

## 先会使用再说吧

```sh
trace_fs=/sys/kernel/debug/tracing
function=__do_page_fault
pid=$1

set -e

cd $trace_fs
echo 0 > tracing_on
echo > trace
echo 0 > options/funcgraph-irqs || true
echo 7 > max_graph_depth
if [ -n "$pid" ]; then
        echo $pid > set_ftrace_pid
fi

echo function_graph > current_tracer
#echo func_stack_trace  > trace_options
echo $function > set_graph_function
echo -n "current trace functions:  "
cat set_graph_function
echo 1 > tracing_on

timeout 15 cat trace_pipe
echo 0 > tracing_on
echo nop > current_tracer
echo > set_graph_function

echo 1 > options/funcgraph-irqs || true
echo 0 > max_graph_depth
```
