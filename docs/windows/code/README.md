# Windows 系统编程 Demo

## Build

```powershell
cmake -S . -B build
cmake --build build
```

生成的可执行文件在 `build/bin/<Config>/` 下，后缀为 `.exe`，例如：

## Demo 顺序

建议按这个顺序阅读和运行：

1. `process_demo`：`CreateProcess`、process handle、wait、exit code，对照 `fork/exec/waitpid`。
2. `thread_demo`：`CreateThread`、thread handle、`WaitForMultipleObjects`。
3. `ctrl_c_demo`：Console control event、`SetConsoleCtrlHandler`、Ctrl+C 默认终止过程。
4. `sync_demo`：Critical Section、SRWLOCK、Condition Variable、Event、Semaphore。
5. `affinity_demo`：`GetProcessAffinityMask`、`SetProcessAffinityMask`、`SetThreadAffinityMask`，对照 `sched_setaffinity`。
6. `named_semaphore_demo`：named kernel semaphore，跨进程 `wait/post`，对照 POSIX `sem_open`。
7. `file_io_demo`：`CreateFile`、`ReadFile`、`WriteFile`、`SetFilePointerEx`、`DeleteFile`。
8. `file_mapping_demo`：`CreateFileMapping`、`MapViewOfFile`，对照 `mmap`。
9. `overlapped_file_demo`：`FILE_FLAG_OVERLAPPED`、`OVERLAPPED`、`GetOverlappedResult`。
10. `memory_demo`：process heap、private heap、`VirtualAlloc`。
11. `named_pipe_demo`：Windows named pipe，本机 IPC。
12. `echo_server_select` / `echo_client`：Winsock `select` 模型。
13. `echo_server_iocp` / `echo_client`：IOCP completion 模型。

## 运行跨进程 named semaphore

Windows named semaphore 是命名内核对象。两个进程用同一个名字 `Local\vn_demo_named_semaphore` 就能打开同一个 semaphore：

```powershell
# terminal 1
.\build\bin\Debug\named_semaphore_demo.exe wait

# terminal 2
.\build\bin\Debug\named_semaphore_demo.exe post
```

这对应 `user/shm-posix/sem.c` 里的 `sem_open` / `sem_wait` / `sem_post`。Windows 没有 `sem_unlink` 这一步；最后一个 handle 关闭后，对象就消失。

## windows 有绑定核心的操作么?

有。看 `affinity_demo`：

- process 级别：`SetProcessAffinityMask`
- thread 级别：`SetThreadAffinityMask`
- 查询当前可用 mask：`GetProcessAffinityMask`

注意传统 mask 是 processor group 内的 mask，超过 64 个逻辑 CPU 的机器要进一步看 group-aware API，例如 `SetThreadGroupAffinity`。

## windows 这种跨进程的 sem 如何实现?

看 `named_semaphore_demo`。核心 API 是：

- `CreateSemaphoreW(nullptr, initial, max, L"Local\\name")`
- `OpenSemaphoreW(...)`
- `WaitForSingleObject(...)`
- `ReleaseSemaphore(...)`

这个就是 Windows 对 `user/shm-posix/sem.c` 里 named POSIX semaphore 的对应实现。

## Notes

- Windows `HANDLE` 是非常核心的抽象，但不是所有看起来像 handle 的值都能用 `CloseHandle` 关闭。
例如 socket 要用 `closesocket`，heap 要用 `HeapDestroy`。
- IOCP 是 completion queue，不是 readiness notification。这个点和 `select`/`poll`/`epoll` 的思维差别很大。

## TODO
4. 关于内存
5. process
6. 同步原语
  5. 我还是希望有类似 src/c 这个 API 测试才可以，巨大的 sln 项目对于我来说，很难受
7. 异步 io 模型
  - 存储
  - 网络

有无类似 ioctl 的机制，不然那么多设备该如何解决
例如nvmecli

### 补充下这个 rcu ，是用户态的
如果写内核驱动的话，就可以确认了。

不用了，直接可以找到，而且找到了更多的 windows 相关的 api :
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kercureadlock

## windows 下的基本编程

## 阅读书籍
https://book.douban.com/subject/3235659/

pdf :
- https://ptgmedia.pearsoncmg.com/images/9780735663770/samplepages/9780735663770.pdf
- https://github.com/ckjbug/Windows-Core-Programming?tab=readme-ov-file


## 代码
https://gitee.com/martins3/windows

```txt
msbuild .\04-ProcessInfo.vcxproj -t:Rebuild /t:ClangTidy  -p:Configuration=Debug -p:Platform=X64
```

这些例子其实都过于复杂了:
- https://github.com/microsoft/Windows-universal-samples
- https://github.com/microsoft/Windows-classic-samples

## 在看一个例子

https://github.com/gammasoft71/Examples_Win32

```txt
PS C:\Users\97936\data\Examples_Win32\build> cmake ..
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.22631.
-- The C compiler identification is MSVC 19.43.34810.0
-- The CXX compiler identification is MSVC 19.43.34810.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64/cl.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (4.5s)
-- Generating done (0.5s)
-- Build files have been written to: C:/Users/97936/data/Examples_Win32/build
```

而且这个是一个很经典的例子，就是通过 cmake 来生成 sln 和 build\ALL_BUILD.vcxproj
的例子

如果想要构建的话，可以使用 msbuild ，也可以使用这个方法，就像是 llvm 那样的:
```sh
cmake --build .
```

## windows 编程的头文件在哪里

C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\winternl.h


不过似乎引入了太多的图形相关的东西:
C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\winuser.h

C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\processthreadsapi.h

好家伙，微软的实力还是强啊:
```txt
C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0>tokei
===============================================================================
 Language            Files        Lines         Code     Comments       Blanks
===============================================================================
 C                      32         3835         1756          142         1937
 C Header             4338      7573168      5326791      1018714      1227663
 C++ Header             41        87088        64755        11522        10811
 Plain Text              1           24            0           19            5
 XML                     4         9451         7860          178         1413
===============================================================================
 Total                4416      7673566      5401162      1030575      1241829
===============================================================================
```

windows 没有 proc fs 之类的，也就是所有的类似的 api 都是通过类似的机制暴露的么?

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
