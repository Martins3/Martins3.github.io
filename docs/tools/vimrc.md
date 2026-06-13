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
