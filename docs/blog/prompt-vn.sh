#!/usr/bin/env bash
set -E -e -u -o pipefail

file="${1:?need markdown file}"

[[ -f $file ]]
[[ $file == *.md ]]

dir="$(cd -- "$(dirname -- "$file")" && pwd)"
file="$dir/$(basename -- "$file")"
output="${file%.md}.codex.md"

prompt="我之前在 $file 中记录了很多杂乱的笔记。

请重新阅读这些笔记，并结合相关源码或可验证材料，重新梳理其中的内容，然后写入 $output。

要求：
- 先按照自己的理解，对于这个主题做整体的讲解，不要一开始就分析细节
- 然后整理回答原始文档中的问题
- 请先阅读相关源码，再解释机制、数据结构、调用路径和关键细节。
- 输出使用中文，内容要完整，不要只写摘要。
- 只读取 $file，不要修改它。
- 将整理后的完整内容写入 $output。
- 写入完成后留在当前 Codex 交互会话中，等待我继续追问。"

export https_proxy=http://127.0.0.1:7890
export http_proxy=http://127.0.0.1:7890
export HTTPS_PROXY=http://127.0.0.1:7890
export HTTP_PROXY=http://127.0.0.1:7890
export ftp_proxy=http://127.0.0.1:7890
export FTP_PROXY=http://127.0.0.1:7890
exec codex --sandbox danger-full-access "$prompt"
# kimi --yolo -p "$prompt"
