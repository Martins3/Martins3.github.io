# 这一次我要学会 bash

<!-- vim-markdown-toc GitLab -->

  * [问题](#问题)
  * [命令行参数](#命令行参数)
  * [基本符号](#基本符号)
  * [扩展模式](#扩展模式)
  * [here doc 和 here string](#here-doc-和-here-string)
  * [变量的默认值](#变量的默认值)
  * [字符串](#字符串)
  * [数组](#数组)
  * [eval 和 exec 的区别](#eval-和-exec-的区别)
  * [算术运算](#算术运算)
  * [[ ] xargs](#-xargs)
  * [bible](#bible)
  * [重定向](#重定向)
  * [资源和工具](#资源和工具)
  * [具体问题的链接](#具体问题的链接)
* [一些链接](#一些链接)
  * [一些高级技术](#一些高级技术)
  * [一些资源](#一些资源)
  * [一些博客](#一些博客)

<!-- vim-markdown-toc -->

## 问题
- [ ] 学会使用 eval
- [ ] 类似 $SHELL 之外的还有什么定义好的全局变量

## 命令行参数
1. -n 参数可以取消末尾的回车符
2. -e 参数会解释引号（双引号和单引号）里面的特殊字符（比如换行符\n

- ctrl + w 删除光标前的单个域
- ctrl + k：从光标位置删除到行尾

## 基本符号
```sh
# \
echo a \
b
# space
echo a             b
echo "a             b"
# ;
echo a; ls
```

## 扩展模式
> 模式扩展与正则表达式的关系是，模式扩展早于正则表达式出现，可以看作是原始的正则表达式。它的功能没有正则那么强大灵活，但是优点是简单和方便。

- `?` 一个字符
- `*` 任意字符
- `[ ]` 方括号的中的字符
  - `[^...]` 和 `[!...]`。它们表示匹配不在方括号里面的字符，这两种写法是等价的。
- `{a,b}`
- `{a...z}`
- 变量扩展 `${!string*}` 或 `${!string@}` 返回所有匹配给定字符串 `string` 的变量名。
- 字符类
  - [![:upper:]]
- 量词语法
  - `?(pattern-list)`：匹配零个或一个模式。
  - `*(pattern-list)`：匹配零个或多个模式。
  - `+(pattern-list)`：匹配一个或多个模式。
  - `@(pattern-list)`：只匹配一个模式。
  - `!(pattern-list)`：匹配给定模式以外的任何内容。

## here doc 和 here string
```sh
this=aaa
cat << LUA
"this"
\$this
$this
\b
LUA

cat << 'LUA'
"this"
\$this
$this
\b
LUA
```
\ 的确是可以使用的，但是

```sh
cat <<< "fuck"
```
## 变量的默认值

## 字符串
```sh
数组长度
${#name}
```

```sh
STR="/path/to/foo.cpp"
echo ${STR%.cpp}    # /path/to/foo
echo ${STR##*.}     # cpp (extension)
echo ${STR##*/}     # foo.cpp (basepath)
```

## 数组
拷贝:
hobbies=( "${activities[@]}" )
增加一项:
hobbies=( "${activities[@]}" diving )
myIndexedArray+=('six')
用 unset 命令来从数组中删除一个元素：
unset fruits[0]

使用 @ 和 `*` 来循环数组是否存在双引号的情况各不相同。
- 如果没有，忽视双引号，逐个拆开
- 如果有，`*` 是一个，而 @ 不会逐个拆开

```sh
function xxx () {
echo "Using \"\$*\":"
for a in "$*"; do
    echo $a;
done

echo -e "\nUsing \$*:"
for a in $*; do
    echo $a;
done

echo -e "\nUsing \"\$@\":"
for a in "$@"; do
    echo $a;
done

echo -e "\nUsing \$@:"
for a in $@; do
    echo $a;
done

}
xxx one two "three four"
```
参考:
- https://unix.stackexchange.com/questions/129072/whats-the-difference-between-and
- https://stackoverflow.com/questions/12314451/accessing-bash-command-line-args-vs

https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html
https://blog.k8s.li/shell-snippet.html
- [x] 单引号和双引号的区别
  - https://stackoverflow.com/questions/6697753/difference-between-single-and-double-quotes-in-bash
  - 但是实际上，这个解释是有问题的, 实际上是三个特殊字符除外：美元符号（`$`）、反引号（`\`）和反斜杠（`\`) 其余都是在双引号被取消掉
  - 而单引号会取消掉任何，甚至包括反斜杠

## eval 和 exec 的区别
https://unix.stackexchange.com/questions/296838/whats-the-difference-between-eval-and-exec/296852

eval 相当于执行这个函数
exec 继续执行程序

## 算术运算
使用这个，而不是 let expr 之类的 $((1+2))

## [ ] xargs
git grep -l 'apples' | xargs sed -i 's/apples/oranges/g'
- https://stackoverflow.com/questions/6758963/find-and-replace-with-sed-in-directory-and-sub-directories

## bible
[Pure bash bible](https://github.com/dylanaraps/pure-bash-bible)

## 重定向
参考[^1]
1. ls > a.txt
2. ls 2> a.txt
3. ls 2>&1
4. ls 2>&1 > a.txt
5. ls | tee > a.txt

## 资源和工具
1. https://explainshell.com/
2. https://wangchujiang.com/linux-command/

## 具体问题的链接
1. [obscure but useful](https://github.com/jlevy/the-art-of-command-line/blob/master/README.md)

# 一些链接
1. [The art of command line](https://github.com/jlevy/the-art-of-command-line/blob/master/README-zh.md#%E4%BB%85%E9%99%90-os-x-%E7%B3%BB%E7%BB%9F)


## 一些高级技术
- [indirect expansion](https://unix.stackexchange.com/questions/41292/variable-substitution-with-an-exclamation-mark-in-bash)

## 一些资源
- [A utility tool powered by fzf for using git interactively](https://github.com/wfxr/forgit)

## 一些博客
- [window powershell 和 bash 的对比](https://vedipen.com/2020/linux-bash-vs-windows-powershell/)

[^1]: https://wizardzines.com/comics/redirects/
