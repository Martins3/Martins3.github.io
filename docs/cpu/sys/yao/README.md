# 超标量处理器设计

姚永斌编著，清华大学出版社，2014 年 4 月第 1 版。ISBN 978-7-302-34707-1。

本目录是整理后的精简阅读版，包含全书 11 章正文、前后附文和正文所需图表，以及整理时新增的学习辅助、术语表和校注。采用已有的 Codex 整理结果，保留原文、公式、代码、表格与图注。

## 阅读入口

- [全书目录](SUMMARY.md)：各章及全部章内小节。
- [前言](Preface.md)、[出版信息](Frontmatter.md)、[参考文献](References.md)、[封底与作者的话](Backcover.md)。
- [学习辅助](study/README.md)、[术语表](study/Glossary.md)：整理时新增的讲解和自测问题。
- [校注与原书疑点](Notes.md)：整理时记录的原书疑点，与原文分开保留。

## 各章正文

- [第 1 章 超标量处理器概览](Ch01/Ch01.md)
- [第 2 章 Cache](Ch02/Ch02.md)
- [第 3 章 虚拟存储器](Ch03/Ch03.md)
- [第 4 章 分支预测](Ch04/Ch04.md)
- [第 5 章 指令集体系](Ch05/Ch05.md)
- [第 6 章 指令解码](Ch06/Ch06.md)
- [第 7 章 寄存器重命名](Ch07/Ch07.md)
- [第 8 章 发射](Ch08/Ch08.md)
- [第 9 章 执行](Ch09/Ch09.md)
- [第 10 章 提交](Ch10/Ch10.md)
- [第 11 章 真实世界的例子：Alpha 21264 处理器](Ch11/Ch11.md)

## 阅读说明

章节按原书编排，保留小节编号和页码锚点。PDF 页码从 1 开始；正文及参考文献的书内页码等于 PDF 页码减 12。

公式使用 LaTeX，适合在支持数学公式的 Markdown 阅读器中查看。复杂图形与时序图使用原书裁剪图，随正文保存在 `images/ChXX/` 中。全部本地链接均使用相对路径，可独立阅读或整体移动本目录。

原书内容按已有整理结果保留；学习辅助和校注见对应文档。

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
