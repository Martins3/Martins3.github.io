## 有时候，中文没字体

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
