# 学习一下 regex

- [ ] sed vim 中的替换和一种格式的 regex 吗?
- 在 vim 中 You can remove trailing spaces with the command :%s/\s\+$/.
- [ ] https://wangdoc.com/bash/condition.html#%E6%AD%A3%E5%88%99%E5%88%A4%E6%96%AD
  - 到底什么是 glob 和 regex 啊

1. [tutorial](https://github.com/ziishaned/learn-regex/blob/master/README-cn.md) from github.
2. [another](https://www.elastic.co/guide/en/beats/heartbeat/current/regexp-support.html) good tutorial.
https://refrf.shreyasminocha.me/

- [ ] 使用 ^-?[0-9]+$ 来匹配所有的数字，结果发现 glob regex 的区别
  - [ ] 而且判断语句 =~ and =

|  Category  | Description |
|:----------:|:-----------:|
| ECMAScript |             |
|    basic   |             |
|  expanded  |             |
|     awk    |             |
|    grep    |             |
|    egrep   |             |

现在根本看不懂 regex 的语法了:
https://github.com/dylanaraps/pure-bash-bible#use-regex-on-a-string
```sh
regex() {
    # Usage: regex "string" "regex"
    [[ $1 =~ $2 ]] && printf '%s\n' "${BASH_REMATCH[1]}"
}
```

regex "      hello" '^\s*(.*)'
## quesion
1. About different version, how many version ? The difference between them ?
