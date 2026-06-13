  如果只是为了"把 PDF 内容丢给 AI 分析"，优先级如下：
   工具               安装包            用途               优先级
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   pdftotext          poppler-utils     最简洁的纯文本提   高
                                        取
   Python 库 (pymup   pip install ...   程序化提取文本、   高
   df / pypdf / pdf                     表格、图片
   plumber)
   mutool             mupdf-tools       提取文本、渲染图   中
                                        片、修复 PDF
   qpdf               qpdf              合并/拆分/解密 P   低（仅结构操作）
                                        DF
   pdfgrep            pdfgrep           在多个 PDF 里搜    低（可选）
                                        索关键词
  推荐组合：
  • 最快路线：安装 poppler-utils，获得 pdftotext。
  • 最灵活路线：安装 Python 库 pymupdf（也叫 PyMuPDF），提取文本最干净，还能
    页处理。



快速选择建议
要免费 + 本地 + 高质量 → Marker 或 MinerU
要隐私 + 内网部署 → PDF3MD
要在线即用 + 简单文档 → MDisBetter
要 AI 管道 / RAG 预处理 → MarkItDown 或 BlazeDocs
要扫描版 PDF（OCR） → BlazeDocs、Marker（带 OCR 选项）或 PDF Craft（专为扫描书籍设计）

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
