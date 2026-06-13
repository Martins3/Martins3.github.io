# windows 下的基本编程

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
