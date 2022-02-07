# 这一次我要学会 bash

<!-- vim-markdown-toc GitLab -->

* [命令行参数](#命令行参数)
* [重定向](#重定向)
* [一些高级技术](#一些高级技术)
* [一些资源](#一些资源)

<!-- vim-markdown-toc -->

## 命令行参数
1. -n
2. -e

https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html

https://blog.k8s.li/shell-snippet.html

- [ ] 单引号和双引号的区别
- [ ] 忽然发现从来没有理解清楚过 if [[]]
  - 到底是两个还是一个 [[ ]]
  - 可以省略吗>
- [ ] 数字计算

```sh
for file in ~/core/ppt/*.html; do
    echo "$(basename "$file" .html).txt"
done
```

- [ ] 复杂的 xargs
- [ ] 学会使用 eval
- [ ] 类似 $SHELL 之外的还有什么定义好的全局变量

## 重定向
参考[^1]
1. ls > a.txt
2. ls 2> a.txt
3. ls 2>&1
4. ls > a.txt 2>&1
5. ls | tee > a.txt

## 一些高级技术
[indirect expansion](https://unix.stackexchange.com/questions/41292/variable-substitution-with-an-exclamation-mark-in-bash)

## 一些资源
- [A utility tool powered by fzf for using git interactively](https://github.com/wfxr/forgit)

[^1]: https://wizardzines.com/comics/redirects/
