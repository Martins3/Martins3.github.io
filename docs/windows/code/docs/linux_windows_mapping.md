# Linux / Windows 系统编程对照

本文按 Linux 系统程序员的视角解释本仓库 demo。重点不是 API 名字一一翻译，而是找出 Windows 的对象模型和执行模型。

## 大图景

| Linux/POSIX 经验 | Windows 近似物 | 关键差异 |
| --- | --- | --- |
| file descriptor | `HANDLE` / `SOCKET` | `HANDLE` 覆盖进程、线程、文件、event、semaphore、pipe 等对象；`SOCKET` 属于 Winsock，要用 `closesocket`。 |
| `errno` | `GetLastError()` / `WSAGetLastError()` | Win32 和 Winsock 错误通道不同。 |
| `fork` + `execve` | `CreateProcess` | Windows 没有 fork 复制当前进程映像的常规模型。 |
| `waitpid` | `WaitForSingleObject(process_handle)` | process/thread 都是 waitable object。 |
| `pthread_create` | `CreateThread` / `_beginthreadex` | C/C++ runtime 代码更偏向 `_beginthreadex`；本仓库基础 demo 用 `CreateThread` 展示内核对象。 |
| `open/read/write/lseek` | `CreateFile` / `ReadFile` / `WriteFile` / `SetFilePointerEx` | `CreateFile` 也打开 pipe、device 等对象。 |
| `mmap` | `CreateFileMapping` + `MapViewOfFile` | Windows 显式区分 mapping object 和 mapped view。 |
| `malloc` / `mmap` | `HeapAlloc` / `VirtualAlloc` | heap API 和 page-level virtual memory API 分层明显。 |
| `pthread_mutex` | Critical Section / Mutex | Critical Section 适合同进程；Mutex 是 kernel object，可跨进程命名。 |
| `pthread_cond` | Condition Variable | Windows condition variable 通常配合 SRWLOCK 或 Critical Section。 |
| `sem_t` / `sem_open` | Semaphore / named Semaphore | Windows semaphore 是 waitable handle，可命名，跨进程通过相同名字打开。 |
| `sched_setaffinity` | `SetProcessAffinityMask` / `SetThreadAffinityMask` | 传统 affinity mask 是 processor group 内的 mask，超大机器要看 group-aware API。 |
| `eventfd` 的部分用途 | Event | Windows Event 是常见的一次性/广播 wait primitive。 |
| FIFO / Unix domain socket | Named Pipe | Windows named pipe 使用 `\\.\pipe\...` 命名空间，可本机或远程。 |
| BSD socket | Winsock | API 形状相近，但需要 `WSAStartup`，socket 关闭和错误处理不同。 |
| `select` | Winsock `select` | `nfds` 参数被忽略；`fd_set` 存 Winsock socket。 |
| `epoll` readiness | IOCP completion | IOCP 投递操作后等待“完成”，不是等待“可读/可写”。 |

## Windows 有、Linux 没有直接对应物

- 统一 waitable object 模型：process、thread、event、semaphore、mutex 等都能进入 `WaitForSingleObject` / `WaitForMultipleObjects`。
- Named kernel object：很多内核对象可以命名并跨进程打开。
- IOCP：以完成事件为中心的高并发 I/O 模型，worker 从 completion port 取已完成操作。
- Job Object：用于把进程组当作资源控制和生命周期管理单位，概念上不像普通 Unix process group。
- Security Descriptor / ACL：Windows 对象访问控制非常普遍，不只是文件系统权限。

## Windows 没有直接对应 Linux 语义的地方

- `fork`：Windows 常规进程创建是 `CreateProcess`，不会复制当前地址空间后从同一位置继续执行。
- Unix signal 语义：Windows 有 console control event、APC、exception、event object 等，但没有一套完全等价的 signal 机制。
- `procfs` 风格接口：Windows 有 Toolhelp、PSAPI、ETW、WMI 等多套接口，不是一个统一的 `/proc` 文件树。
- `epoll` 完全同构物：IOCP 更像完成队列，设计目标和调用方式不同。

## 本仓库 demo 对应关系

| Demo | Windows API | Linux 心智锚点 |
| --- | --- | --- |
| `process_demo` | `CreateProcessW`, `WaitForSingleObject`, `GetExitCodeProcess`, Toolhelp snapshot | `fork/exec/waitpid`, `/proc` 枚举 |
| `thread_demo` | `CreateThread`, `WaitForMultipleObjects`, `GetExitCodeThread` | `pthread_create`, `pthread_join` |
| `ctrl_c_demo` | `SetConsoleCtrlHandler`, console control event | terminal `SIGINT` / Ctrl+C |
| `sync_demo` | Critical Section, SRWLOCK, Condition Variable, Event, Semaphore | pthread mutex/cond, semaphore, eventfd |
| `affinity_demo` | `GetProcessAffinityMask`, `SetProcessAffinityMask`, `SetThreadAffinityMask` | `sched_setaffinity` |
| `named_semaphore_demo` | `CreateSemaphoreW`, `OpenSemaphoreW`, `WaitForSingleObject`, `ReleaseSemaphore` | `sem_open`, `sem_wait`, `sem_post` |
| `file_io_demo` | `CreateFileW`, `ReadFile`, `WriteFile`, `SetFilePointerEx` | `open/read/write/lseek` |
| `file_mapping_demo` | `CreateFileMappingW`, `MapViewOfFile` | `mmap` |
| `overlapped_file_demo` | `OVERLAPPED`, `GetOverlappedResult` | async I/O concepts |
| `memory_demo` | `HeapAlloc`, `HeapCreate`, `VirtualAlloc` | malloc arenas, `mmap` |
| `named_pipe_demo` | `CreateNamedPipeW`, `ConnectNamedPipe` | FIFO / Unix domain socket |
| `echo_server_select` | Winsock `select` | `select` |
| `echo_server_iocp` | `CreateIoCompletionPort`, `WSARecv`, `WSASend` | completion-based async I/O |

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
