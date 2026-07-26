## Virual Studio 简记

### sln 和 vcproj 作用是什么
https://stackoverflow.com/questions/7133796/what-are-sln-and-vcproj-files-and-what-do-they-contain

### 下载主题
工具，选项，调整字体

点 extension 就可以了
Catppuccin

### 使用 msbuild

https://stackoverflow.com/questions/39798321/generate-clang-compilation-database-for-a-visual-studio-project
```txt
msbuild .\01-ErrorShow.vcxproj /t:ClangTidy -t:Rebuild -p:Configuration=Release -p:Platform=X64
```
可以自动生成 compile_commands.json

这个工具不靠谱的:
https://clangpowertools.com/

如何使用:
- https://learn.microsoft.com/en-us/visualstudio/msbuild/walkthrough-using-msbuild?view=vs-2022
- https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference?view=vs-2022


解决 msbuild 的环境变量问题:
- https://stackoverflow.com/questions/6319274/how-do-i-run-msbuild-from-the-command-line-using-windows-sdk-7-1

目前在这个路径:
```txt
PS C:\Program Files> fzf
Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
```

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
