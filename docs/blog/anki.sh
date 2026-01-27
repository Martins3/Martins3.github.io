#!/usr/bin/env bash
set -E -e -u -o pipefail

MAIN_REPO=$HOME/data/vn

anki=/home/martins3/data/vn/docs/blog/fsrs/target/debug/anki-fsrs
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
	# 创建缓存目录和文件
	cache_dir="/tmp/anki_cache"
	cache_file="$cache_dir/uuid_mapping.cache"
	rm -rf "$cache_dir"
	mkdir -p "$cache_dir"


	# 基本思路：
	# 1. 使用 rg 查找所有包含 UUID 格式的注释行
	# 2. 对每个匹配的文件进行处理
	# 3. 直接将 UUID -> 文件:行号 的映射写入缓存
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

			# 将 UUID -> 文件:行号 的映射写入缓存
			echo "${uuid}:${file}:${start_line}" >>"$cache_file"
		done
	done

}

function get_original_info_from_cache() {
	local uuid=$1
	local cache_file="/tmp/anki_cache/uuid_mapping.cache"
	local line

	# 在缓存文件中查找 UUID 对应的信息
	if ! grep "^${uuid}:" "$cache_file" &> /dev/null ; then
		$anki --delete "$uuid"
		echo ""
	fi
	line=$(grep "^${uuid}:" "$cache_file" | head -n 1)
	if [[ -n $line ]]; then
		local file_part
		local line_part
		file_part=$(echo "$line" | cut -d: -f2)
		line_part=$(echo "$line" | cut -d: -f3)
		echo "$file_part:$line_part"
		return 0
	fi

	return 1
}

function extract_title_from_file() {
	local file=$1
	local line_num=$2
	title=$(sed -n "${line_num}p" "$file" | sed 's/^[[:space:]#]*//')
}

function play2() {
	readarray -t items < <($anki --due)
	for uuid in "${items[@]}"; do
		# 从缓存获取原始文件和行号
		info=$(get_original_info_from_cache "$uuid")
		if [[ -z $info ]]; then
			echo "UUID $uuid not found in cache, skip"
			# $anki --delete "$uuid"
			continue
		fi

		original_file=$(echo "$info" | cut -d: -f1)
		original_line=$(echo "$info" | cut -d: -f2)

		# 提取标题
		extract_title_from_file "$original_file" "$original_line"

		if ! gum style --foreground 212 \
			--border-foreground 212 \
			--border double \
			--margin "1 1" \
			--padding "1 1" \
			"$title"; then
			echo "$title"
		fi

		read_user_input
		echo "$user_input"
		$anki --insert "${uuid}" "$user_input"
		pushd "$MAIN_REPO"
		set -x
		nvim "+$original_line" "$original_file"
		set +x
	done
}

function import() {
	# 从缓存文件中读取所有 UUID 并导入
	local cache_file="/tmp/anki_cache/uuid_mapping.cache"
	if [[ -f $cache_file ]]; then
		local uuid file line
		while IFS=':' read -r uuid file line || [[ -n $uuid ]]; do
			if [[ -n $uuid ]]; then
				if ! $anki --check "$uuid"; then
					$anki --insert "$uuid" 4
				fi
			fi
		done <"$cache_file"
	fi
}

while getopts "rsc" opt; do
	case $opt in
		r)
			refinement
			import
			exit 0
			;;
		s)
			$anki --stats
			exit 0
			;;
		*)
			echo "anki"
			echo "anki -r (refresh cache and import)"
			echo "anki -c (clear cache)"
			exit 1
			;;
	esac
done
check_neovim_env
play2

# 给这个自动带上 word 来处理
# code/misc/word.sh
