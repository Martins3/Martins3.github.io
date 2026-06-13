#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
PROGDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
PERFBOOK_DIR="/home/martins3/data/perfbook"
PROMPT_FILE="prompt.txt"
DONE_DIR="$PROGDIR/chapters"

declare -a chapters=(
	"howto/howto.tex|01|How To Use This Book"
	"intro/intro.tex|02|Introduction"
	"cpu/cpu.tex|03|Hardware and its Habits"
	"toolsoftrade/toolsoftrade.tex|04|Tools of the Trade"
	"count/count.tex|05|Counting"
	"SMPdesign/SMPdesign.tex|06|Partitioning and Synchronization Design"
	"locking/locking.tex|07|Locking"
	"owned/owned.tex|08|Data Ownership"
	"defer/defer.tex|09|Deferred Processing"
	"datastruct/datastruct.tex|10|Data Structures"
	"debugging/debugging.tex|11|Validation"
	"formal/formal.tex|12|Formal Verification"
	"together/together.tex|13|Putting It All Together"
	"advsync/advsync.tex|14|Advanced Synchronization"
	"memorder/memorder.tex|15|Advanced Synchronization: Memory Ordering"
	"easy/easy.tex|16|Ease of Use"
	"future/future.tex|17|Conflicting Visions of the Future"
	"summary.tex|18|Looking Forward and Back"
	"appendix/appendix.tex|A|Appendix"
)

function usage() {
	echo "Usage: $0 [ask <chapter_num> [question]]"
	echo ""
	echo "Modes:"
	echo "  (no args)          Batch process chapters"
	echo "  ask <num> [q]      Ask question about a chapter (interactive if no question)"
	exit 1
}

function collect_tex_files() {
	local file="$1"
	local dir
	dir=$(dirname "$file")
	echo "$file"

	while IFS= read -r line; do
		if [[ $line =~ ^[[:space:]]*\\input\{([^}]+)\}[[:space:]]*$ ]]; then
			local subfile="${BASH_REMATCH[1]}"
			if [[ $subfile == *".fcv" ]]; then
				continue
			fi
			if [[ -f "$dir/$subfile" ]]; then
				collect_tex_files "$dir/$subfile"
			elif [[ -f "$dir/$subfile.tex" ]]; then
				collect_tex_files "$dir/$subfile.tex"
			elif [[ -f "$PERFBOOK_DIR/$subfile" ]]; then
				collect_tex_files "$PERFBOOK_DIR/$subfile"
			elif [[ -f "$PERFBOOK_DIR/$subfile.tex" ]]; then
				collect_tex_files "$PERFBOOK_DIR/$subfile.tex"
			fi
		fi
	done <"$file"
}

function collect_unique_tex_files() {
	local file="$1"
	declare -A seen=()
	local path

	while IFS= read -r path; do
		if [[ -n ${seen["$path"]+x} ]]; then
			continue
		fi
		seen["$path"]=1
		echo "$path"
	done < <(collect_tex_files "$file")
}

function find_chapter_info() {
	local target="$1"
	local entry
	for entry in "${chapters[@]}"; do
		IFS='|' read -r chapter_path chapter_num chapter_name <<<"$entry"
		if [[ $chapter_num == "$target" ]]; then
			echo "$entry"
			return 0
		fi
	done
	return 1
}

function slugify() {
	local text="$1"

	text=$(echo "$text" | tr '[:upper:]' '[:lower:]')
	text=$(echo "$text" | sed -E 's/[^a-z0-9]+/-/g; s/^-+//; s/-+$//; s/-+/-/g')

	if [[ -z $text ]]; then
		text="chapter"
	fi

	echo "$text"
}

function get_chapter_dir() {
	local chapter_num="$1"
	local chapter_name="$2"
	local slug

	slug=$(slugify "$chapter_name")
	echo "$DONE_DIR/${chapter_num}-${slug}"
}

function migrate_chapter_dir_if_needed() {
	local chapter_num="$1"
	local chapter_name="$2"
	local target_dir legacy_dir

	target_dir=$(get_chapter_dir "$chapter_num" "$chapter_name")
	legacy_dir="$DONE_DIR/$chapter_num"

	if [[ -d $legacy_dir && ! -e $target_dir ]]; then
		mv "$legacy_dir" "$target_dir"
	fi

	echo "$target_dir"
}

function ask_chapter() {
	local target_num="$2"
	shift 2
	local question="${*:-}"

	if [[ -z $target_num ]]; then
		echo "[FAIL] Chapter number is required"
		usage
	fi

	# Normalize chapter number: 7 -> 07, but keep A as A
	if [[ $target_num =~ ^[0-9]+$ && ${#target_num} -eq 1 ]]; then
		target_num=$(printf '%02d' "$target_num")
	fi

	local entry
	entry=$(find_chapter_info "$target_num") || {
		echo "[FAIL] Chapter '$target_num' not found"
		exit 1
	}

	IFS='|' read -r chapter_path chapter_num chapter_name <<<"$entry"
	local full_path="$PERFBOOK_DIR/$chapter_path"
	local chapter_dir

	chapter_dir=$(migrate_chapter_dir_if_needed "$chapter_num" "$chapter_name")

	if [[ ! -f $full_path ]]; then
		echo "[FAIL] $full_path not found"
		exit 1
	fi

	if [[ -z $question ]]; then
		read -r -p "Question for chapter $chapter_num ($chapter_name): " question
	fi

	if [[ -z $question ]]; then
		echo "[FAIL] Empty question, aborting"
		exit 1
	fi

	local tex_files
	tex_files=$(collect_unique_tex_files "$full_path")

	local gen_files=""
	if [[ -d $chapter_dir ]]; then
		local f
		while IFS= read -r f; do
			gen_files="$gen_files$f
"
		done < <(find "$chapter_dir" -maxdepth 1 -type f \( -name '*.md' -o -name '*.txt' -o -name '*.c' -o -name '*.h' -o -name 'Makefile' \) ! -name 'done' | sort)
	fi

	echo "========================================"
	echo "=> Ask Chapter $chapter_num: $chapter_name"
	echo "=> Question: $question"
	echo "========================================"

	local full_prompt="用户问题: $question

请基于以下材料回答用户问题。回答时必须引用 perfbook 原文的具体内容，不能泛泛而谈。请依次读取所有列出的文件。

该章节已生成的分析笔记文件：
$gen_files
perfbook 第 $chapter_num 章 ($chapter_name) 的原始 tex 源码文件：
$tex_files

要求：
1. 回答前先阅读原始 tex 源码，找到与问题相关的原文段落。
2. 结合已生成的分析笔记和原始 tex 源码给出回答。
3. 引用原文时请标注出处（章节/文件名）。
4. 如果原始 tex 源码中有代码示例，可以结合说明。"

	kimi --yolo -p "$full_prompt" --work-dir "$chapter_dir"
}

function process_chapters() {
	if [[ ! -f $PROMPT_FILE ]]; then
		echo "[FAIL] $PROMPT_FILE not found in current directory"
		exit 1
	fi

	mkdir -p "$DONE_DIR"
	local PROMPT
	PROMPT=$(cat "$PROMPT_FILE")

	local entry chapter_path chapter_num chapter_name full_path chapter_dir done_file legacy_done_file legacy_done_file2
	for entry in "${chapters[@]}"; do
		IFS='|' read -r chapter_path chapter_num chapter_name <<<"$entry"
		full_path="$PERFBOOK_DIR/$chapter_path"
		chapter_dir=$(migrate_chapter_dir_if_needed "$chapter_num" "$chapter_name")
		done_file="$chapter_dir/done"
		legacy_done_file="$DONE_DIR/$chapter_num.done"
		if [[ $chapter_num =~ ^0([1-9])$ ]]; then
			legacy_done_file2="$DONE_DIR/${BASH_REMATCH[1]}.done"
		else
			legacy_done_file2=""
		fi

		if [[ ! -f $full_path ]]; then
			echo "[WARN] $full_path not found, skipping"
			continue
		fi

		if [[ -f $legacy_done_file && ! -f $done_file ]]; then
			mkdir -p "$chapter_dir"
			mv "$legacy_done_file" "$done_file"
		fi
		if [[ -n $legacy_done_file2 && -f $legacy_done_file2 && ! -f $done_file ]]; then
			mkdir -p "$chapter_dir"
			mv "$legacy_done_file2" "$done_file"
		fi

		if [[ -f $done_file ]]; then
			echo "[OK] Skip chapter $chapter_num: $chapter_name"
			continue
		fi

		mkdir -p "$chapter_dir"

		local file_list file_count file_paths f
		file_list=$(collect_unique_tex_files "$full_path" | sed "s|^$PERFBOOK_DIR/||")
		file_count=$(echo "$file_list" | wc -l)

		echo "========================================"
		echo "=> Chapter $chapter_num: $chapter_name"
		echo "=> Files: $file_count"
		echo "========================================"

		file_paths=""
		while IFS= read -r f; do
			file_paths="$file_paths$PERFBOOK_DIR/$f
"
		done <<<"$file_list"

		local full_prompt="$PROMPT
请分析第 $chapter_num 章: $chapter_name
本章包含以下文件，请依次读取并进行完整分析:

将结果和代码记录到 $chapter_dir 中
$file_paths"

		echo "$full_prompt"

		kimi --yolo -p "$full_prompt" --work-dir "$chapter_dir" --max-ralph-iterations 0

		date '+%F %T' >"$done_file"
		echo ""
		echo "[OK] Finished: Chapter $chapter_num - $chapter_name"
		echo "[OK] Marked done: $done_file"
		exit 0
	done

	echo "[OK] All chapters processed"
}

case "${1:-}" in
	ask)
		ask_chapter "$@"
		;;
	-h | --help | help)
		usage
		;;
	"")
		process_chapters
		;;
	*)
		echo "[FAIL] Unknown command: $1"
		usage
		;;
esac
