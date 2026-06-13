## 编译 edk2

参考
- [官方文档](https://github.com/tianocore/tianocore.github.io/wiki/Common-instructions)
- [stackoverflow](https://stackoverflow.com/questions/63725239/build-edk2-in-linux)

## HelloWorld

执行完成之后，可以看到这个:
Build/EmulatorX64/DEBUG_GCC5/X64/HelloWorld.efi
![](./uefi/img/MdeModulePkg_hello_world.png)

## compile_commands.json
build -Y COMPILE_INFO 可以有 compile_commands

Build/OvmfX64/DEBUG_GCC5/CompileInfo/compile_commands.json

```txt
build -Y COMPILE_INFO -y BuildReport.log
```

- https://bugzilla.tianocore.org/show_bug.cgi?id=2850
- https://github.com/makaleks/edk2-tools/tree/master/compilation_database_patch
- https://github.com/tianocore/edk2-basetools/pull/88 : 官方支持了
- https://github.com/intel/Edk2Code/wiki/Index-source-code

## 经典的项目
想要具体构建什么可以通过 ACTIVE_PLATFORM 来控制:

1. EmulatorPkg/EmulatorPkg.dsc

```txt
ACTIVE_PLATFORM       = EmulatorPkg/EmulatorPkg.dsc
```
参考 EmulatorPkg/Readme.md ，居然可以直接在 unix 环境中:
```txt
build -p EmulatorPkg/EmulatorPkg.dsc -a X64 -t GCC5

cd Build/EmulatorX64/DEBUG_GCC5/X64/ && ./Host
```

2. MdeModulePkg.dsc

```txt
ACTIVE_PLATFORM       = MdeModulePkg/MdeModulePkg.dsc
```

build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC

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
