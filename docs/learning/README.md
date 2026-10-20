# 视频学习教程

- [羽毛球基础精要：15 P 学习笔记](./BV17t411j7Qr/index.html)
- [饥荒新手指南：10 集学习笔记](./BV1n8411d7SL/index.html)

## 源码与发布

本目录只保留 Markdown、页面模板、构建与验证脚本，以及使用和来源说明。生成的 `index.html`、`share/` 不保存、不提交，由 GitHub Pages CI 在 Jekyll 构建之前运行 `node docs/learning/build.cjs --publish` 生成。

使用现有 `docs/pub/sync-blog.py` 同步源码，再按原流程推送目标仓库。首次新增源码仍需由使用者纳入 Git 跟踪，才能进入发布审计；生成文件不需要跟踪。本地构建仅用于临时验证，验证后删除产物。

构建只需要 Node.js，无需安装 npm 依赖，也无需下载视频素材。羽毛球教程发布版默认使用 B 站内嵌播放器；本地视频素材仍保存在与 vn 同级的 vn-resources 仓库。饥荒教程已扩展到原站合集十集，也支持本地/B 站播放，发布无需上传本地素材。

## BV17t411j7Qr 来源与整理边界


原片：[羽毛球基础精要（全集）高清晰度](https://www.bilibili.com/video/BV17t411j7Qr/)，上传者“大师兄哔哩哔哩”。来源接口列出 15 P、总计约 3 小时 27 分钟；上传者不等于各集讲师。

全部 15 P 都完成了本机 faster-whisper small 中文语音识别，再结合内嵌中文字幕 OCR、章节标题与相应画面整理。所有 P 都下载了完整视频与音频。公开字幕接口未返回独立字幕；OCR 与 ASR 原始文本保留在资源目录，可能有错字、重复、繁简混用和近似时间。

正文保留概览，并逐项展开准备姿势、脚步顺序、拍面与发力变化、学员纠错、战术条件及后续衔接。重复示范不逐次复述，讲师实际讲解的区别与纠正点分别保留；不是逐字转录，也没有声称逐帧看完全部视频。文字中的问题速查、自查提示、连续回合拆解和跨章节关联属于整理者归纳。片段边界近似；细微动作以原片连续画面为准。

旧教学片的发球高度与停顿说法已在 P09 单独校正，并链接 BWF 官方发球条款。讲师示范中的腿部角度与距离没有转换成人人必须照量的标准；混双示例中的分工按实际能力理解。

## BV1n8411d7SL 来源与整理边界

来源是亚丹Ardent的“饥荒新手指南”合集，入口视频 BV1n8411d7SL 本身只有一 P。2026-10-09 的公开接口列出十集、总计约 81 分 40 秒；冬季、春季、回复、料理、建家、战斗、驯牛、种地与航海各有独立 BV。正文中的 P01–P10 是合集顺序，不是原视频分 P 参数，播放器按各集自己的 BV 和第一 P 引用。

全部十集保存完整视频与音频，每两秒提取内嵌中文字幕进行 Windows OCR，并结合语音识别和关键画面核对。字幕、语音识别可能出错；正文不是逐字稿，也没有逐帧观看全部视频。重复展示合并，保留操作顺序、条件、配方、错误与后续衔接；问题索引、跨集串联和自查属于整理者归纳。数值沿用原片，未验证当前游戏版本；片段边界近似。原作者的地貌简化、出海地形判断与延迟经验不作为所有世界设置的绝对规律。

素材在 vn 同级 vn-resources/bilibili/ 下，各 BV 对应自己的文件夹；p01–p10 子目录按合集顺序存储，含 video.mp4、来源信息、probe.json、captions-ocr.json 与 transcript-asr.json。第一个视频原来的素材及截图仍保留，但网页已改为视频入口。download.ps1、extract.ps1、transcribe.ps1 可重做下载、字幕识别和语音转写；语音识别使用 faster-whisper 1.2.1 small CPU int8，音视频解码固定 av<17，避免不兼容接口。网页 CI 不运行这些素材步骤，不需要视频或识别模型。

## 统一使用方法

每个视频目录的 README.md 是正文与笔记唯一内容源，直接在编辑器中修改。页面只读，点击片段后在右侧查看视频，顶部选择本地视频或 B 站视频；本地视频支持片段终点暂停和循环，B 站按分 P 和起始秒数定位。两套教程均使用连续视频片段，不依赖截图。Esc 收起画面，方括号切换上一处、下一处。

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
