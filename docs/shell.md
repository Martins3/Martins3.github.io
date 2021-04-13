# shell 的一些经验之谈 

![](https://preview.redd.it/8a7tpszpdgj41.png?width=640&height=360&crop=smart&auto=webp&s=04e05726a9bb67ff47a8599101931409953859a0)

`shell` 和 `gnu` `make`, `cmake` 等各种工具类似，一学就会，学完就忘。究其原因，是因为使用频率太低了。
所以，shell 我不建议非常认真系统的学习，因为学完之后发现根本用不上。难道你每天都需要使用正则表达式删除文件吗?

## shell 资源推荐
1. https://devhints.io/bash  : 语法清单
2. https://explainshell.com/ : 给出一个 shell 命令，对于其进行解释
3. https://linuxjourney.com/ : 一个简明的教程

## 选择好用的 shell
zsh 和 bash 之前语法上基本是兼容的，但是由于[oh my zsh](https://github.com/ohmyzsh/ohmyzsh)，我强烈推荐使用zsh

## 终端模拟器
我现在的选在是 tmux + alacritty，等到我彻底解决了 alacritty 和 fcitx 的问题，我会提供对应教程。

- [Deepin](https://github.com/linuxdeepin/deepin-terminal)
- [tilix](https://gnunn1.github.io/tilix-web/)
- [kitty](https://sw.kovidgoyal.net/kitty/)
- [hyper](https://hyper.is/)
- [Alacritty](https://github.com/alacritty/alacritty)

## 常用命令行工具的替代
使用Linux有个非常窒息的事情在于，默认的工具使用体验一般，下面介绍一些体验更加的工具。
[这里](https://css.csail.mit.edu/jitk/) 总结的工具非常不错，下面是我自己的补充。这些工具都是基本是从 github awesome[^1][^2][^3] 和 hacker news[^4] 中间找到:

| 😞   | 😃                                                                                                                   |
|------|----------------------------------------------------------------------------------------------------------------------|
| cd   | [autojump](https://github.com/wting/autojump) <br> [z.lua](https://github.com/skywind3000/z.lua)                     |
| ls   | [lsd](https://github.com/Peltoche/lsd)                                                                               |
| du   | [ncdu](https://dev.yorhel.nl/ncdu)                                                                             |
| gdb  | [gdb dashboard](https://github.com/cyrus-and/gdb-dashboard)                                                 |
| git  | [diff-so-fancy](https://github.com/so-fancy/diff-so-fancy) <br> [lazy git](https://github.com/jesseduffield/lazygit) <br> [bit](https://github.com/chriswalz/bit)|
| man  | [cheat](https://github.com/chubin/cheat.sh)                                                                          |
| find | [fd](https://github.com/chinanf-boy/fd-zh)                                                                           |
| ssh | [sshfs](https://github.com/libfuse/sshfs)                                                                           |

## 一些小技巧
- [alias](https://thorsten-hans.com/5-types-of-zsh-aliases)
- [/dev/null](https://www.putorius.net/introduction-to-dev-null.html)
- [bash 使用方向键匹配历史记录](https://askubuntu.com/questions/59846/bash-history-search-partial-up-arrow)

## reference
[^1]: https://github.com/agarrharr/awesome-cli-apps
[^2]: https://github.com/alebcay/awesome-shell
[^3]: https://github.com/unixorn/awesome-zsh-plugins
[^4]: https://news.ycombinator.com/ 
