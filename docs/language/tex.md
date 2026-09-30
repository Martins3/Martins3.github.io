https://artofproblemsolving.com/wiki/index.php/LaTeX:Symbols

## 教程
https://github.com/luong-komorebi/Begin-Latex-in-minutes/blob/master/Translation-Chinese.md : 最基本的教程，什么时候可以看看

## 模板
https://github.com/tuna/thuthesis

## Github Action
https://github.com/xu-cheng/latex-action

## vim
如果想要书写中文，需要修改默认的 latex engine，在 ~/.latexmkrc 中设置:
```txt
$pdf_mode = 5;
```

虽然，但是我看了下我机器这个文件中的内容:
```txt
$pdflatex="xelatex %O %S";
$pdf_mode = 5;
```

参考:
- https://tex.stackexchange.com/questions/429274/chinese-on-mactex2018-simple-example
- https://tex.stackexchange.com/questions/501492/how-do-i-set-xelatex-as-my-default-engine

# 使用 neovim 写毕业论文

- https://github.com/mohuangrui/ucasthesis
- https://askubuntu.com/questions/35155/how-to-uninstall-latex
- https://www.tug.org/texlive/acquire-netinstall.html
  - https://www.tug.org/texlive/quickinstall.html

安装代码:
```c
perl install-tl
```

## 如何写伪代码

## 消除 ucasthesis 上 的所有的 warning
- https://github.com/mohuangrui/ucasthesis/wiki/%E5%AD%97%E4%BD%93%E9%85%8D%E7%BD%AE
  - 到这个地方下载字体: https://github.com/mingchen/mac-osx-chinese-fonts

## 也许这个是一个更加酷的
https://typst.app/docs/guides/


https://github.com/KaTeX/KaTeX : web 上写数学

## 整理
- [latex game](https://texnique.xyz/)
- [latex](https://andreas-zeller.blogspot.com/2017/01/twelve-latex-packages-to-get-your-paper.html)
- [Latex 教程](https://www.overleaf.com/learn/latex/Learn_LaTeX_in_30_minutes)
- [latex 符号搜索](https://latex.guide/)

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
