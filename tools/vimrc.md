https://www.reddit.com/r/vim/comments/fzpdpd/im_loving_the_combination_of_vimtex_goyo/ : 终于解决我的

https://github.com/ripxorip/aerojump.nvim : easymothion 的替代品 ?

https://www.reddit.com/r/vim/comments/g2dfkt/back_with_more_vimtex_content_this_time_with_pdf/  : 直接在终端中间预览pdf

https://github.com/voldikss/vim-floaterm#use-as-the-git-editor : 将原来的文件

https://github.com/wellle/context.vim : 有点意思

https://github.com/iamcco/coc-spell-checker : TODO 之前的拼写检查真的好丑呀!

https://github.com/HJLebbink/asm-dude : 有人做汇编语言的 lsp 吗 ?


[](http://lymslive.top/book/vimllearn/_book/) : 中文 vimscript 教程
https://github.com/wsdjeg/vim-plugin-dev-guide : 王世东的教程，非常简短的
https://github.com/wsdjeg/vim-galore-zh_cn : 使用的教程，值的认真看看

https://www.reddit.com/r/vim/comments/i50pce/how_to_show_commit_that_introduced_current_line/ : 太酷了，搞懂这一句应该会学会很多

# 使用 lua
https://gabrielpoca.com/2019-11-02-a-little-bit-of-lua-in-your-vim/

更多值的探索的插件。
https://www.reddit.com/r/vim/comments/fxal8p/ive_crawled_166_dotfiles_repos_and_have_generated/

https://github.com/tyru/notify-changed.vim : 和 wakatime 联合使用

wakatime 的哪一个东西，我想制作一个出来玩一下 !

https://github.com/klaussinani/taskbook/blob/master/docs/readme.ZH.md
https://github.com/kdheepak/lazygit.vim
如果利用这两个技术实现vim 中间处理todo 就不错了

## Pinao
本文依赖使用的环境: manjaro + neovim-0.3.4

# 启动
首先安装依赖
```
sudo pacman -S sdl2 sdl2_mixer
sudo pip3 install pysdl2
```

vim 中间安装插件
https://github.com/skywind3000/vim-keysound

添加配置:
```
    let g:keysound_py_version = 3 " 根据自己的使用python 版本
    let g:keysound_enable = 1
```

参考资料:
1. https://github.com/WarpPrism/AutoPiano

https://www.reddit.com/r/vim/comments/fzpdpd/im_loving_the_combination_of_vimtex_goyo/


## 垃圾堆

 ----------------- 垃圾堆，不用理会 -------------------------------------------
 https://github.com/high-moctane/asyncomplete-nextword.vim # 也许是解救我的写作的位置了
 https://github.com/jceb/vim-orgmode/blob/master/doc/orgguide.txt
 https://github.com/vivien/vim-linux-coding-style 内核 C 语言风格
 https://github.com/liuchengxu/vim-clap 什么时候将 leaderf 替换掉

 作者水平一般，暂时不可用
 [[custom_plugins]]
     name = 'kdheepak/lazygit.vim'
     rev = 'nvim-v0.4.3'

 实现括号颜色配对 和 vim-lsp-cxx-highlight 冲突，需要手动设置
 [[custom_plugins]]coc.nvim
     name =  'luochen1990/rainbow'

 编辑 hex 文件
 [[custom_plugins]]
     name = 'Shougo/vinarise.vim'

 键盘声音
 [[custom_plugins]]
   # name = 'skywind3000/vim-keysound'
 配套使用的UI系统
 [[custom_plugins]]
     name = 'waldson/vui'

 [[custom_plugins]]
     name = 'AndrewRadev/splitjoin.vim'

 当进入到 window 的时候自动调节其大小，有点bug
 [[custom_plugins]]
     name = 'camspiers/animate.vim'
 [[custom_plugins]]
     name = 'camspiers/lens.vim'

 用于预览 reStructuredText 文件，但是无法正常工作
 [[custom_plugins]]
     name = 'Rykka/riv.vim'
 [[custom_plugins]]
     name = 'Rykka/InstantRst'

 一个有意思的同步软件
 [[custom_plugins]]
   # name = 'kenn7/vim-arsync'
   # merged =  0

 two cppman tools, waiting for it complete 
 [[custom_plugins]]
     name = 'gauteh/vim-cppman'
 [[custom_plugins]]
     name = 'skywind3000/vim-cppman'

 Failed to install
 https://www.reddit.com/r/neovim/comments/ezs67g/neovim_plugin_that_provides_fzf_preset_with/
 [[custom_plugins]]
     name = 'yuki-ycino/fzf-preview.vim'

 vim game based on new features of Vim 8.2，it doesn't work on nvim 4.3
 [[custom_plugins]]
     name = 'vim/killersheep'

 [[custom_plugins]]
     name = 'vhda/verilog_systemverilog.vim'

 interesting plug
 https://github.com/janko/vim-test
 https://github.com/justinmk/vim-sneak seems better than easymotion
 https://github.com/prettier/vim-prettier a format for many languages we don't have 

 relearn vim about how to move and edit more quickly !
 https://github.com/tpope/vim-repeat
 https://github.com/tpope/vim-surround


 After instaling this plugin, it seems nothing changed !
 [[custom_plugins]]
   # name = 'sheerun/vim-polyglot'

 [[custom_plugins]]
 # name = 'sedm0784/vim-you-autocorrect'

 maybe useful
 tpope/vim-surround
 terryma/vim-multiple-cursors

 another debug framework
 https://github.com/puremourning/vimspector#faq

 debug for vim doesn't work well so far !
 https://github.com/idanarye/vim-vebugger
 [[layers]]
 # name = "debug"

 https://github.com/kristijanhusak/defx-icons
 maybe file icon is not so easy to support for defx-icons
 [[custom_plugins]]
     name = 'kristijanhusak/defx-icons'

 [[custom_plugins]]
     name =  "Shougo/vimproc.vim"
     build = "make"

 [[custom_plugins]]
     name = 'Shougo/neoinclude.vim'

 [[custom_plugins]]
     name = 'jsfaint/coc-neoinclude'

 几个语言的支持，其他的语言支持利用 coc.nvim 
 能不能直接利用 SpaceVim 中间 lsp 的支持
 [[layers]]
   name = "lang#vim"

 [[layers]]
   name = "lang#latex"

 看一下吧
 https://github.com/sakhnik/nvim-gdb


```
# 处理 adoc 格式的文件
[[custom_plugins]]
    name = 'habamax/vim-asciidoctor'

# 安装方法 "./install_gadget.py --enable-c"
# 然后将需要下载的内容粘贴到浏览器中间，下载好之后拷贝到指定的文件夹
# 这个插件是否好用，值的观察
# [[custom_plugins]]
#     name = 'puremourning/vimspector'

" TODO 可以深度开发一下，暂时很迷
" https://github.com/voldikss/coc-todolist
call coc#config("todolist.monitor", v:true)
call SpaceVim#custom#SPC('nnoremap', ['n', 'c'], 'CocCommand todolist.create', 'create a todolist', 1)
call SpaceVim#custom#SPC('nnoremap', ['n', 'l'], 'CocList todolist', 'list functions', 1)
```
