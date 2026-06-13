## vscode 的调试环境
https://github.com/Digital-EDA/Digital-IDE

- [vscode-neovim](https://github.com/asvetliakov/vscode-neovim)(worth a try)

> vimrc/vim plugins/etc are supported (few plugins don't make sense with vscode, such as nerdtree)


## 给 gvisor 搭建环境
ref :  https://stackoverflow.com/questions/55411277/how-can-i-setup-vscode-for-go-project-built-with-bazel

1. genereate gopath style source code
```plain
bazel build :gopath
```
2. for clarity and simplisity, copy code somewhere else
```c
cp -Lr bazel-bin/gopath ~/test/gv
```
3. init go mod in `~/test/gv/gopath/gvisor.dev`
```plain
go mod init gvisor.dev
```

4. setup env:
  - for vscode:
```plain
➜  gvisor git:(master) ✗ cat .vscode/settings.json
{
    "go.toolsEnvVars": {
         "GOPATH": "~/test/gv/gopath"
    }
}%
```
  - for vim:
```plain
➜  gvisor git:(master) ✗ export GOPATH=~/test/gv/gopath
```
5. open directory `~/test/gv/gopath/gvisor.dev` as root directory with vim or vscode

## 内核的 vscode 配置
[DKernel-Plus](https://github.com/ShaoxunZeng/DKernel-Plus)

## llvm 环境配置
https://github.com/sunxfancy/vscode-llvm

## 远程环境搭建

需要在 server 配置 tcp forward 才可以

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
