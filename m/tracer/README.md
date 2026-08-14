tracer 只能在内核中直接修改

这些修改直接放到
```txt
git apply trace/extra.diff
cp trace/trace_martins3.c kernel/trace/trace_martins3.c
```

## 基本实验
cat available_tracers

```txt
mytracer blk function_graph function nop
```

echo mytracer > current_tracer

cat trace
```txt
# tracer: mytracer
#
# My Tracer Output
# =================
```

## 最终目的

1. 为什么 blktrace 需要一个单独的模块?
