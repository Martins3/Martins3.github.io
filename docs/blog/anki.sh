#!/usr/bin/env bash
set -E -e -u -o pipefail

MAIN_REPO=$HOME/data/vn

anki=/home/martins3/data/vn/docs/blog/fsrs/target/debug/anki-fsrs
output=/tmp/output/
pushd "$MAIN_REPO"

function check_neovim_env() {
	if env | grep neovim >/dev/null; then
		echo "Don't Run nvim in nvim 😸"
		exit 1
	fi
}

function error_format() {
	echo "$1"
	echo "error : $2"
	echo ""
	echo "# title"
	echo "<!-- uuid -->"
	exit 1
}

user_input=""
function read_user_input() {
	while true; do
		echo "Choose one key:"
		echo "  [A] Again  [H] Hard  [G] Good  [E] Easy"
		read -r -n 1 -p "> " choice
		echo

		case "$choice" in
			A | a)
				echo "You chose: Again"
				break
				;;
			H | h)
				echo "You chose: Hard"
				break
				;;
			G | g)
				echo "You chose: Good"
				break
				;;
			E | e)
				echo "You chose: Easy"
				break
				;;
			*)
				echo "Invalid key. Use A/H/G/E."
				echo
				;;
		esac
	done
	user_input="$choice"
}

function extract() {
	local file=$1
	title=$(sed -n '2p' "$file" | sed 's/^[[:space:]#]*//')
}

function refinement() {
	rm -rf $output
	mkdir -p $output

	# 基本思路：
	# 1. 使用 rg 查找所有包含 UUID 格式的注释行
	# 2. 对每个匹配的文件进行处理
	# 3. 提取从注释行到下一个 # 开头行的内容
	pattern="<!-- [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} -->"
	uuid_pattern="[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"
	readarray -t array < <(rg -l --type md "$pattern")
	for file in "${array[@]}"; do
		echo "$file"
		# 提取所有 UUID 和 line number
		readarray -t uuids < <(rg -n "$pattern" "$file")

		for u in "${uuids[@]}"; do
			start_line=$(echo "$u" | cut -d: -f1)
			uuid=$(echo "$u" | rg -o "$uuid_pattern")
			# echo "u : $u"
			# echo "uuid : $uuid"
			# echo "start line : $start_line"
			#
			# 文件的行是从 1 开始计算的
			start_line=$((start_line - 1))
			if [[ $start_line -le 0 ]]; then
				nvim +$start_line "$file"
				error_format "$uuid" "no tilte found"
			fi

			if sed -n "${start_line}p" "$file" | grep -q '^#'; then
				:
			else
				nvim +$start_line "$file"
				error_format "$uuid" "no title found"
			fi

			local o="$output/${uuid}.md"
			local end_line
			echo "$file:$start_line" >"$o"
			# 一直匹配到 # 之前的行号
			if end_line=$(tail -n +"$((start_line + 1))" "$file" | rg -n -e "^#" -m 1 | cut -d: -f1); then
				actual_end=$((start_line + end_line - 1))
				sed -n "${start_line},${actual_end}p" "$file" >>"$o"
			else
				sed -n "${start_line},\$p" "$file" >>"$o"
			fi
		done
	done

}

function play2() {
	readarray -t items < <($anki --due)
	for uuid in "${items[@]}"; do
		file=$output/${uuid}.md
		if [[ ! -f $file ]]; then
			echo "$(basename "$file") not found, skip"
			continue
		fi
		extract "$file"
		original_file=$(head -n 1 "$file" | cut -d: -f1)
		original_line=$(head -n 1 "$file" | cut -d: -f2)

		gum style --foreground 212 \
			--border-foreground 212 \
			--border double \
			--margin "1 1" \
			--padding "1 1" \
			"$title"

		read_user_input
		echo "$user_input"
		$anki --insert "${uuid}" "$user_input"
		if [[ $user_input == "Good" ]]; then
			bat "$file"
		else
			pushd "$MAIN_REPO"
			set -x
			nvim "+$original_line" "$original_file"
			set +x
		fi
	done
}

function import() {
	for file in "$output"/*.md; do
		m=$(basename "$file")
		m=${m%%.md}
		if ! $anki --check "$m"; then
			$anki --insert "$m" 4
		fi
	done
}

while getopts "r" opt; do
	case $opt in
		r)
			refinement
			import
			exit 0
			;;
		*)
			echo "anki"
			echo "anki -r"
			exit 1
			;;
	esac
done
check_neovim_env
play2

# 给这个自动带上 word 来处理
# code/misc/word.sh
