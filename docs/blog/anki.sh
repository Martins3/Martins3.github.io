#!/usr/bin/env bash
set -E -e -u -o pipefail

source "$HOME/data/vn/docs/blog/anki-common.sh"

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

function get_original_info_from_cache() {
	local uuid=$1
	local line

	# 在缓存文件中查找 UUID 对应的信息
	if ! grep "^${uuid}"$'\t' "$CACHE_FILE" &>/dev/null; then
		"$ANKI_BIN" --delete "$uuid"
		echo "delete $uuid"
	fi
	line=$(grep "^${uuid}"$'\t' "$CACHE_FILE" | head -n 1)
	if [[ -n $line ]]; then
		local file_part
		local line_part
		file_part=$(echo "$line" | cut -f2)
		line_part=$(echo "$line" | cut -f3)
		printf '%s\t%s\n' "$file_part" "$line_part"
		return 0
	fi

	return 1
}

function extract_title_from_file() {
	local file=$1
	local line_num=$2
	title=$(sed -n "${line_num}p" "$file" | sed 's/^.*#[[:space:]]*//')
}

function play2() {
	ensure_cache_exists
	readarray -t items < <("$ANKI_BIN" --due)
	for uuid in "${items[@]}"; do
		# 从缓存获取原始文件和行号
		info=$(get_original_info_from_cache "$uuid")
		if [[ -z $info ]]; then
			echo "UUID $uuid not found in cache, skip"
			# $anki --delete "$uuid"
			continue
		fi

		original_file=$(echo "$info" | cut -f1)
		original_line=$(echo "$info" | cut -f2)

		# 提取标题
		extract_title_from_file "$original_file" "$original_line"

		"$ANKI_BIN" --inspect "$uuid"
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
		"$ANKI_BIN" --insert "${uuid}" "$user_input"
		set -x
		nvim "+${original_line}" "$original_file"
		set +x
	done
}

function import() {
	# 从缓存文件中读取所有 UUID 并导入
	if [[ -f $CACHE_FILE ]]; then
		local uuid file line
		while IFS=$'\t' read -r uuid file line || [[ -n $uuid ]]; do
			if [[ -n $uuid ]]; then
				if ! "$ANKI_BIN" --check "$uuid"; then
					"$ANKI_BIN" --insert "$uuid" 4
				fi
			fi
		done <"$CACHE_FILE"
	fi
}

while getopts "rsc" opt; do
	case $opt in
		r)
			rebuild_cache
			import
			exit 0
			;;
		s)
			"$ANKI_BIN" --stats
			exit 0
			;;
		*)
			echo "anki"
			echo "anki -r (refresh cache and import)"
			echo "anki -s (show stats)"
			exit 1
			;;
	esac
done

check_neovim_env
play2

# 给这个自动带上 word 来处理
# code/misc/word.sh
