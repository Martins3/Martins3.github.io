#!/usr/bin/env bash
set -E -e -u -o pipefail

function check_all() {
	pattern="<!-- [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} -->"

	readarray -t array < <(rg --type md "$pattern" -B 1 --no-filename | rg "##")
	word=$(printf "%s\n" "${array[@]}" | fzf)

	if [[ -z $word ]]; then
		return
	fi

	echo "$word"

	results=$(rg --type md -n "$word")
	count=$(echo "$results" | wc -l)

	if [[ $count -eq 0 ]]; then
		echo "ERROR: No matches found." >&2
		exit 1
	elif [[ $count -gt 1 ]]; then
		echo "ERROR: Multiple matches found:" >&2
		echo "$results" >&2
		exit 1
	else
		# 得到单一匹配的 "file:line:content"
		file=$(echo "$results" | cut -d: -f1)
		line=$(echo "$results" | cut -d: -f2)
		nvim "+${line}" "${file}"
	fi

}


if env | grep neovim >/dev/null; then
	echo "Don't Run nvim in nvim 😸"
	exit 1
fi

cd "$HOME"/data/vn
check_all
