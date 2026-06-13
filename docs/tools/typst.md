## typst 工具

### 如何引入外部的包
```typst
#import "@preview/basic-resume:0.2.9": *
```

如何理解：

- #import：Typst 的导入语句，用于引入外部包或模块
- "@preview/basic-resume:0.2.9"：包标识符，由三部分组成
    - @preview：包来源命名空间，指 Typst 官方包仓库（packages.typst.org）
    - basic-resume：包名
    - 0.2.9：语义化版本号
- :*：通配导入，将该包导出的所有符号（函数、变量等）全部引入当前作用域

这样引入后，你就可以直接使用包提供的 resume、 edu、 work、 project 等函数，而不需要写前缀。

缓存位置：

已下载的包缓存在：

```sh
  ~/.cache/typst/packages/preview/basic-resume/0.2.9/
```

该目录下包含包的完整内容：

- typst.toml：包的元数据/清单
- src/lib.typ：包的入口文件
- src/resume.typ：实现代码
- template/main.typ：模板示例
- README.md、LICENSE 等

Typst 首次编译时会自动从官方仓库下载依赖，后续编译直接使用本地缓存，无需重复下载。

## 中文没字体解决方法

```sh
# 安装步骤

# 1. 创建字体目录
mkdir -p ~/.local/share/fonts

# 2. 下载 Noto CJK 字体
cd ~/.local/share/fonts
curl -L -o NotoSansCJKsc-Regular.otf "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"
curl -L -o NotoSansCJKsc-Bold.otf "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Bold.otf"
curl -L -o NotoSerifCJKsc-Regular.otf "https://github.com/notofonts/noto-cjk/raw/main/Serif/OTF/SimplifiedChinese/NotoSerifCJKsc-Regular.otf"
curl -L -o NotoSerifCJKsc-Bold.otf "https://github.com/notofonts/noto-cjk/raw/main/Serif/OTF/SimplifiedChinese/NotoSerifCJKsc-Bold.otf"

# 3. 更新字体缓存
fc-cache -fv ~/.local/share/fonts

# 验证

# 查看 Typst 识别的字体
typst fonts | grep -i "noto"

# 更新后的文档
# 文档已添加字体设置：
#set text(font: "Noto Serif CJK SC", size: 11pt)
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
