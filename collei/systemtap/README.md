# 使用 SystemTap 跟踪 collei virtme QEMU

SystemTap 运行在 host，跟踪对象是 host 上承载 `virtme` 的 QEMU 进程，不是在
guest 内运行 SystemTap。

## 生成与构建树 QEMU 匹配的 tapset

系统的 `/usr/share/systemtap/tapset/qemu-system-x86_64.stp` 硬编码
`/usr/bin/qemu-system-x86_64`，不能直接跟踪 collei 使用的
`/home/martins3/data/qemu/build/qemu-system-x86_64`。

运行：

```bash
cd /home/martins3/data/vn
./collei/systemtap/generate-tapsets.sh
```

脚本只生成 tapset，不负责构建 QEMU。输出在：

```text
collei/systemtap/generated/qemu-virtme.stp
collei/systemtap/generated/qemu-virtme-log.stp
collei/systemtap/generated/qemu-virtme-simpletrace.stp
```

三份文件使用独立的 `qemu.virtme.*` 命名空间，避免与系统 tapset 中的
`qemu.system.x86_64.*` 冲突。`generated/` 是构建产物，已由 `.gitignore` 忽略。

查看可用的 QMP probe 及参数：

```bash
stap -L 'qemu.virtme.log.qmp_*query_status' \
  -I /home/martins3/data/vn/collei/systemtap/generated
```

## 实时文本 trace

终端 A 读取 collei pidfile，并跟踪这一个 QEMU：

```bash
virtme_pid=$(< /home/martins3/data/hack/vm/virtme/s/pid)

sudo stap \
  -s 8 \
  -I /home/martins3/data/vn/collei/systemtap/generated \
  -x "${virtme_pid}" \
  -e '
probe qemu.virtme.log.qmp_enter_query_status,
      qemu.virtme.log.qmp_exit_query_status {}
'
```

终端 B 通过 collei 创建的 QMP socket 触发只读 `query-status`：

```bash
printf '%s\n%s\n' \
  '{"execute":"qmp_capabilities"}' \
  '{"execute":"query-status"}' |
  socat - \
    UNIX-CONNECT:/home/martins3/data/hack/vm/virtme/s/qmp-no-pretty
```

终端 A 应看到类似：

```text
486959@1786543000000000000 qmp_enter_query_status {}
486959@1786543000000010000 qmp_exit_query_status {"status": "running", "running": true} 1
```

PID 每次启动都会改变，必须从 pidfile 读取。`-x PID` 也能避免跟踪使用同一 QEMU
binary 的其他 collei VM。

## simpletrace 二进制输出

`qemu-virtme-simpletrace.stp` 本身是可以直接 `cat` 的 SystemTap 文本脚本。
下面 `stap -o` 生成的 `.out` 才是 simpletrace 二进制事件数据：

```bash
virtme_pid=$(< /home/martins3/data/hack/vm/virtme/s/pid)

sudo stap \
  -s 8 \
  -o /home/martins3/data/vn/collei/systemtap/qemu-virtme-simpletrace.out \
  -I /home/martins3/data/vn/collei/systemtap/generated \
  -x "${virtme_pid}" \
  -e '
probe qemu.virtme.simpletrace.qmp_enter_query_status,
      qemu.virtme.simpletrace.qmp_exit_query_status {}
'
```

采集期间使用上一节的 QMP 命令触发事件，用 Ctrl-C 结束 stap。解码时必须使用生成
tapset 时同一次 QEMU 构建的 `trace-events-all`，并指定 `--no-header`：

```bash
cd /home/martins3/data/qemu
build/pyvenv/bin/python3 scripts/simpletrace.py --no-header \
  build/trace/trace-events-all \
  /home/martins3/data/vn/collei/systemtap/qemu-virtme-simpletrace.out
```

解码结果类似：

```text
qmp_enter_query_status 0.000 pid=486959 json=b'{}'
qmp_exit_query_status 32.209 pid=486959 result=b'{"status": "running", "running": true}' succeeded=0x1
```

实时文本适合确认事件和短时间排障；simpletrace 二进制更适合保存较多事件后离线分析。
