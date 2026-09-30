## how to debug neovim
- print(vim.inspect(the_table_you_want_to_show))
  - https://github.com/glepnir/nvim-lua-guide-zh
- print(debug.backtrace())
  - https://stackoverflow.com/questions/10838961/lua-find-out-calling-function

## 有趣的
https://news.ycombinator.com/item?id=38515004


## vim 的小技巧
<!-- 48df588c-aa10-4bb3-932c-6e4bda0ab4ad -->

- 翻滚屏幕

| key binding | function                               |
| ----------- | -------------------------------------- |
| H           | 保持屏幕内容不动, 移动到屏幕最上方     |
| M           | 保持屏幕内容不动, 移动到屏幕中间       |
| L           | 保持屏幕内容不动, 移动到屏幕最下面     |
| zt          | 将当前行移动到屏幕最上方               |
| zz          | 将当前行移动到屏幕中间                 |
| zb          | 将当前行移动到屏幕最下方               |
| Ctrl + f    | 向前滚动一屏，但是光标在顶部           |
| Ctrl + d    | 向前滚动一屏，光标在屏幕的位置保持不变 |
| Ctrl + b    | 向后滚动一屏，但是光标在底部           |
| Ctrl + u    | 向后滚动半屏，光标在屏幕的位置保持不变 |
| Ctrl + e    | 丝般顺滑地向上滚动                     |
| Ctrl + y    | 丝般顺滑地向下滚动                     |

https://stackoverflow.com/questions/351161/removing-duplicate-rows-in-vi
排序，并且删掉重复的行

:sort u

vimscript 的调试方法 echom 然后 :message 查看，注意不能是 echo

## nvim 的常用技巧
<!-- 3216b776-462d-40d6-b389-4bd75dc026e0 -->

- vim.api.nvim_err_writeln("hello \n") -- 不要忘记 \n
- nvim "+let g:auto*session_enabled = v:false" -c ":e mm/gup.c" -c "lua vim.loop.new_timer():start(1000 * 60 \_ 30, 0, vim.schedule_wrap(function() vim.api.nvim_command(\"exit\") end))"
- \r 是换行
- :%s/$/abc/ 来给每一行的最后增加 abc

- 如果
https://stackoverflow.com/questions/351161/removing-duplicate-rows-in-vi
```sh
:sort u
```

删掉空行:
```txt
:g/^$/d
```

## yazi 的小技巧
<!-- 29580537-c62b-47ff-8746-56b5da1ff356 -->

如何批量的修改文件。

使用空格选中，然后 jk 移动，最后使用 r 来 rename ，会进入
到 vim 中编辑

## 这种项目到底在解决什么问题
https://github.com/SuperCuber/dotter

有趣的东西看看:
https://www.reddit.com/r/neovim/comments/1q3tnz5/10_builtin_neovim_features_youre_probably_not/

## 可以改善的东西
- nvim 中 失效的 symbol link 需要展示

https://neovim.io/doc/user/quickref.html#Q_qf

## 试试这个命令吧
`:r!figlet vim`

## operator
这类键叫 操作符（operator），后面接“移动范围”或“文本对象”：

操作符 + 范围

常用操作符有：

```txt
 操作符    含义                  示例
━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 d         删除                  diw 删除整个单词
────────  ────────────────────  ────────────────────────────────
 c         删除并进入插入模式    ci" 修改引号内部
────────  ────────────────────  ────────────────────────────────
 y         复制                  yip 复制当前段落
────────  ────────────────────  ────────────────────────────────
 >         增加缩进              >ip 缩进当前段落
────────  ────────────────────  ────────────────────────────────
 <         减少缩进              <ip 取消当前段落一层缩进
────────  ────────────────────  ────────────────────────────────
 =         按缩进规则重新缩进    =ip 调整当前段落缩进
────────  ────────────────────  ────────────────────────────────
 gu        转小写                guiw 将单词转小写
────────  ────────────────────  ────────────────────────────────
 gU        转大写                gUiw 将单词转大写
────────  ────────────────────  ────────────────────────────────
 g~        大小写互换            g~iw 切换单词大小写
────────  ────────────────────  ────────────────────────────────
 !         用外部命令过滤文本    !ip 选择当前段落，等待输入命令
```

范围可以复用，学会一个，就能搭配多个操作符：

```txt
 范围    含义                      组合示例
━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━
 w       到下一个单词开头          dw、yw
──────  ────────────────────────  ───────────────
 $       到行尾                    d$、c$、y$
──────  ────────────────────────  ───────────────
 iw      整个单词，不含周围空白    diw、ciw、yiw
──────  ────────────────────────  ───────────────
 aw      单词及相邻空白            daw、caw
──────  ────────────────────────  ───────────────
 i"      双引号内部                di"、ci"、yi"
──────  ────────────────────────  ───────────────
 a"      包括双引号                da"、ca"
──────  ────────────────────────  ───────────────
 ip      当前段落                  dip、yip、=ip
──────  ────────────────────────  ───────────────
 G       到文件末尾                dG、yG
```

很多操作符还有整行形式：dd 删除行、yy 复制行、cc 修改行、>> 缩进行、== 重新缩
进行。

例如，光标位于 "hello world" 内部：

- di"：变成 ""，留在普通模式。
- ci"：变成 ""，开始输入新内容。
- yi"：复制 hello world，不修改文本。


### which key 的 tirgger mode
这里的 trigger（触发）就是：which-key 在什么时机开始提示“接下来可以按哪些键”。它有两种入口。

1. 按下某个前缀键，触发提示

比如你定义了：

<leader>f → 搜索文件
<leader>b → 查看 buffer

which-key 可以给 <leader> 安装一个触发映射。你按下 <leader> 后停一会儿，它就显示 f、b 等后续选项。

这就是 trigger keymap。

2. 进入某个模式，触发提示

以普通模式下输入 diw（删除一个单词）为例：

按 d → 进入 operator-pending 模式，等待指定删除范围
按 i → 继续等待文本对象
按 w → 完成删除

按下 d 时，Neovim 会产生 ModeChanged 事件。which-key 监听到进入 operator-pending 模式，就可以提示后续动作。按 v 进
入 visual 模式也是类似的机制。

所以，按 d 后出现提示，不一定是给 d 安装了触发映射，也可能是检测到了模式变化。官方说明
(https://github.com/folke/which-key.nvim#-triggers)

<auto> 和 mode = "nixsotc" 怎么读？

{ "<auto>", mode = "nixsotc" }

• <auto>：特殊配置标记，让插件自动选择合适的映射前缀并安装触发器；不是一个实际按键。
• mode：在哪些模式启用，字符串里每个字母代表一种模式。

 字母    模式
━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 n       Normal，普通模式
──────  ────────────────────────────────
 i       Insert，插入模式
──────  ────────────────────────────────
 x       Visual，可视选择模式
──────  ────────────────────────────────
 s       Select，选择模式
──────  ────────────────────────────────
 o       Operator-pending，等待操作范围
──────  ────────────────────────────────
 t       Terminal，终端模式
──────  ────────────────────────────────
 c       Command-line，命令行模式

自动触发器会避开已有映射，包括 Neovim 内置的单键命令，因此并不意味着“按任何键都弹提示”。官方说明
(https://github.com/folke/which-key.nvim#-triggers)

defer 则控制：模式变化后，要不要再等一个键

例如对 d 返回 true：

defer = function(ctx)
  return ctx.operator == "d"
end

输入 diw 时：

按 d  → 已触发，但暂时隐藏提示
再按 i → 显示后续文本对象提示
再按 w → 完成删除

defer 是“再等一次按键”；弹窗等待多少毫秒由 delay 控制。配置说明
(https://github.com/folke/which-key.nvim#%EF%B8%8F-configuration)

你目前的配置 (/home/martins3/.dotfiles/nvim/lua/usr/which-key.lua:3)是：

```txt
triggers = {
  { "<auto>", mode = "nxso" },
  { "c", mode = "n" },
}
```

意思是：在 n/x/s/o 模式自动配置触发器，另外手动让普通模式的 c 成为触发前缀。因为 c 本身是内置的 change 命令，自动
配置会跳过它，所以这里显式补上。

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
