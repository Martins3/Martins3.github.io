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
