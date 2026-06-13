#!/usr/bin/env bash
set -E -e -u -o pipefail

file="${1:?need markdown file}"

[[ -f $file ]]
[[ $file == *.md ]]

dir="$(cd -- "$(dirname -- "$file")" && pwd)"
file="$dir/$(basename -- "$file")"
output="${file%.md}.codex.md"

prompt="我之前在 $file 中记录了，我希望保持原意的情况下，
整理一下。

要求：
- 如果确实标题，提供一个标题
- 直接替换掉原来的文件
- 去掉其中杂乱的，口语化的表达
- 不要使用过多的子标题
- 不要使用引用，例如 [^1]: ，直接在使用的地方 hyperlink
"

function use_codex() {
	export https_proxy=http://127.0.0.1:7890
	export http_proxy=http://127.0.0.1:7890
	export HTTPS_PROXY=http://127.0.0.1:7890
	export HTTP_PROXY=http://127.0.0.1:7890
	export ftp_proxy=http://127.0.0.1:7890
	export FTP_PROXY=http://127.0.0.1:7890
	exec codex --sandbox danger-full-access "$prompt"
}
exec kimi -p "$prompt"
# use_codex
