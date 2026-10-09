# 饥荒开局笔记

唯一内容源是 `教程.md`。在 Neovim 等编辑器中阅读、写笔记和修改正文；`index.html` 是由它生成的只读页面，提供紧凑排版与画面对照。没有浏览器笔记、掌握程度状态或 localStorage 写入，也不需要 `lesson.json`。

## 修改与生成

笔记直接写入 `教程.md`：可以记录哪里卡住、当时的装备与资源、下次怎么改，或追加在对应段落旁。

在本目录运行：

```text
node build.cjs
```

持续编辑时运行 `node build.cjs --watch`，保存 Markdown 后页面自动重新生成，刷新浏览器即可。生成过程只写 `index.html`，不会覆盖 Markdown。网页离线运行，无需服务、安装依赖或登录。

## 文字与画面对照

正文连续显示，配方、资源获取途径用表格归并；页面中点击文字旁的图片或视频链接，在右侧查看原画面，阅读位置保持不动。视频只播放所标片段，结束暂停，可循环；图片可放大。可以收起画面让文字占满阅读区。窄屏时画面按需从底部展开。

快捷键：`Esc` 收起画面；`[` / `]` 切换上一处 / 下一处。

在 Markdown 中新增素材，直接使用普通链接：

```markdown
[待放置预览](../../../../vn-resources/bilibili/BV1n8411d7SL/frames/047.jpg)
[打包操作 03:48–04:23](../../../../vn-resources/bilibili/BV1n8411d7SL/BV1n8411d7SL.mp4#t=228,263)
```

片段时间以秒表示，语法是 MP4 URL 后的 `#t=起点,终点`。在独立 Markdown 查看器里仍是普通素材链接；片段结束暂停与右侧对照由生成页面提供。需要编辑器内直接显示图像时也可使用 `![说明](截图路径)`，网页仍把它呈现为紧凑画面入口。

当前渲染器支持标题、段落、粗体、斜体、普通链接、图片链接、表格、单层列表、引用、行内代码和代码块；不是完整的 CommonMark 实现。不支持任意内嵌 HTML、嵌套列表和复杂表格转义。

## 文件与素材

| 文件 | 用途 |
| --- | --- |
| `教程.md` | 教程正文、素材入口与个人笔记的唯一内容源 |
| `index.html` | 生成的离线只读页面 |
| `template.html` | Markdown 渲染与画面对照模板 |
| `build.cjs` | 单向生成 / 监听 Markdown 修改 |
| `verify.cjs` | 本地资源、生成方向与浏览器交互检查；需隔离浏览器调试端口 |
| 同级 `vn-resources/bilibili/BV1n8411d7SL/` | 视频、截图、原片信息及完整性清单 |

各机器上将 `vn` 与 `vn-resources` 放在同一个父目录，路径不依赖盘符。此次构建不会清除旧浏览器存储，但新页面不读取旧版个人复习记录；此前导出的 Markdown 笔记可以直接粘贴到正文中。

## 来源与覆盖范围

原片：[饥荒萌新终极入门攻略！](https://www.bilibili.com/video/BV1n8411d7SL/)，亚丹Ardent，约9分16秒。

依据画面与内嵌中文字幕整理，覆盖原片主要教学信息，合并重复叙述、删去笑话，不是逐字转录，也未做语音逐字转录。作者已说明地貌、猪人房分布、楼梯入口地形有所简化。游戏数值按原片记录，未验证当前版本；片段边界为近似值，截图每5秒抽帧。首次下洞临时照明建议与核心装备材料合计是明确标注的整理补充。

素材在同级 `vn-resources/bilibili/BV1n8411d7SL/`：视频、来源元信息、截图、每秒抽帧联系表均保留。公开字幕接口为空，没有独立字幕文件。 [完整原视频](../../../../vn-resources/bilibili/BV1n8411d7SL/BV1n8411d7SL.mp4) · [来源信息](../../../../vn-resources/bilibili/BV1n8411d7SL/source-api.json)

发布时由 GitHub Pages CI 生成 index.html，仓库只保存源码；本地构建仅用于临时验证，不保留或提交生成文件。

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
