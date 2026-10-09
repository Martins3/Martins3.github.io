# 视频学习教程

- [羽毛球基础精要：15 P 学习笔记](./BV17t411j7Qr/index.html)
- [饥荒开局笔记](./BV1n8411d7SL/index.html)

## 源码与发布

本目录只保留 Markdown、页面模板、构建与验证脚本，以及使用和来源说明。生成的 `index.html`、`share/` 不保存、不提交，由 GitHub Pages CI 在 Jekyll 构建之前运行 `node docs/learning/build.cjs --publish` 生成。

使用现有 `docs/pub/sync-blog.py` 同步源码，再按原流程推送目标仓库。首次新增源码仍需由使用者纳入 Git 跟踪，才能进入发布审计；生成文件不需要跟踪。本地构建仅用于临时验证，验证后删除产物。

构建只需要 Node.js，无需安装 npm 依赖，也无需下载视频素材。羽毛球教程发布版默认使用 B 站内嵌播放器；本地视频素材仍保存在与 vn 同级的 vn-resources 仓库。饥荒旧版仍依赖本地图片与视频，当前只迁移和清理源码，尚未改为在线素材。

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
