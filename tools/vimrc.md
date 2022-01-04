## minimal's coc
1. 
```
loongson ➜  .vim cat coc-settings.json
{
  "languageserver": {
    "ccls": {
      "command": "ccls",
      "filetypes": [
        "c",
        "cc",
        "cpp",
        "c++",
        "objc",
        "objcpp"
      ],
      "rootPatterns": [
        ".ccls",
        "compile_commands.json",
        ".git/",
        ".hg/"
      ],
      "initializationOptions": {
        "cache": {
          "directory": ".ccls"
        }
      }
    }
  }
}
loongson ➜  .vim
```
2. 然后将标准配置放到 mini.vim 中间
3. vim -u mini.vim a.c

## how to debug neovim
- print(vim.inspect(the_table_you_want_to_show))
  - https://github.com/glepnir/nvim-lua-guide-zh
- print(debug.backtrace())
  - https://stackoverflow.com/questions/10838961/lua-find-out-calling-function
