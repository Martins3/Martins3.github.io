# 视频学习教程

- [羽毛球基础精要：15 P 学习笔记](./BV17t411j7Qr/index.html)
- [饥荒开局笔记](./BV1n8411d7SL/index.html)

## 源码与发布

本目录只保留 Markdown、页面模板、构建与验证脚本，以及使用和来源说明。生成的 `index.html`、`share/` 不保存、不提交，由 GitHub Pages CI 在 Jekyll 构建之前运行 `node docs/learning/build.cjs --publish` 生成。

使用现有 `docs/pub/sync-blog.py` 同步源码，再按原流程推送目标仓库。首次新增源码仍需由使用者纳入 Git 跟踪，才能进入发布审计；生成文件不需要跟踪。本地构建仅用于临时验证，验证后删除产物。

构建只需要 Node.js，无需安装 npm 依赖，也无需下载视频素材。羽毛球教程发布版默认使用 B 站内嵌播放器；本地视频素材仍保存在与 vn 同级的 vn-resources 仓库。饥荒旧版仍依赖本地图片与视频，当前只迁移和清理源码，尚未改为在线素材。

## BV17t411j7Qr 来源与整理边界


原片：[羽毛球基础精要（全集）高清晰度](https://www.bilibili.com/video/BV17t411j7Qr/)，上传者“大师兄哔哩哔哩”。来源接口列出 15 P、总计约 3 小时 27 分钟；上传者不等于各集讲师。

全部 15 P 都完成了本机 faster-whisper small 中文语音识别，再结合内嵌中文字幕 OCR、章节标题与相应画面整理。所有 P 都下载了完整视频与音频。公开字幕接口未返回独立字幕；OCR 与 ASR 原始文本保留在资源目录，可能有错字、重复、繁简混用和近似时间。

正文保留概览，并逐项展开准备姿势、脚步顺序、拍面与发力变化、学员纠错、战术条件及后续衔接。重复示范不逐次复述，讲师实际讲解的区别与纠正点分别保留；不是逐字转录，也没有声称逐帧看完全部视频。文字中的问题速查、自查提示、连续回合拆解和跨章节关联属于整理者归纳。片段边界近似；细微动作以原片连续画面为准。

旧教学片的发球高度与停顿说法已在 P09 单独校正，并链接 BWF 官方发球条款。讲师示范中的腿部角度与距离没有转换成人人必须照量的标准；混双示例中的分工按实际能力理解。

## BV1n8411d7SL 来源与整理边界


原片：[饥荒萌新终极入门攻略！](https://www.bilibili.com/video/BV1n8411d7SL/)，亚丹Ardent，约9分16秒。

依据画面与内嵌中文字幕整理，覆盖原片主要教学信息，合并重复叙述、删去笑话，不是逐字转录，也未做语音逐字转录。作者已说明地貌、猪人房分布、楼梯入口地形有所简化。游戏数值按原片记录，未验证当前版本；片段边界为近似值，截图每5秒抽帧。首次下洞临时照明建议与核心装备材料合计是明确标注的整理补充。

素材在同级 `vn-resources/bilibili/BV1n8411d7SL/`：视频、来源元信息、截图、每秒抽帧联系表均保留。公开字幕接口为空，没有独立字幕文件。 [完整原视频](../../../vn-resources/bilibili/BV1n8411d7SL/BV1n8411d7SL.mp4) · [来源信息](../../../vn-resources/bilibili/BV1n8411d7SL/source-api.json)

发布时由 GitHub Pages CI 生成 index.html，仓库只保存源码；本地构建仅用于临时验证，不保留或提交生成文件。

## 统一使用方法

每个视频目录的 README.md 是正文与笔记唯一内容源，直接在编辑器中修改。页面只读，点击片段后在右侧查看视频，顶部选择本地视频或 B 站视频；本地视频支持片段终点暂停和循环，B 站按分 P 和起始秒数定位。饥荒旧模板仍使用本地截图与视频。Esc 收起画面，方括号切换上一处、下一处。

CI 在 Jekyll 前运行 node docs/learning/build.cjs --publish。本地临时验证可在视频目录运行 node build.cjs 或 node build.cjs --watch，验证后清除产物。无需 npm 依赖。素材保存在 vn 同级 vn-resources；片段用相对路径 MP4#t=起点,终点表示。

verify.cjs 和 verify-share.cjs 使用隔离浏览器调试端口检查构建方向、视频定位、搜索与来源切换。download、extract、ocr 脚本用于素材获取和识别，不参与 CI 网页构建。

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
