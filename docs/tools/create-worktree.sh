#!/usr/bin/env bash
# 通用的 git worktree 创建工具

set -E -e -u -o pipefail

# 默认配置
WORKTREE_BASE=""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

function show_help() {
	cat <<'EOF'
创建 git worktree

用法:
  create-worktree.sh [选项] <worktree-name> [branch-name]

参数:
  worktree-name  - worktree 名称 (如: feature-x)
  branch-name    - 分支名称 (可选，默认为 worktree-name)

选项:
  -h, --help         显示帮助信息
  -b, --base-dir     worktree 基础目录 (默认: .worktrees)

示例:
  # 仅创建 worktree
  create-worktree.sh feature-x

  # 指定 worktree 存放目录
  create-worktree.sh -b ./worktrees feature-x my-branch
EOF
}

function log_info() {
	echo -e "${BLUE}[INFO]${NC} $*"
}

function log_success() {
	echo -e "${GREEN}[OK]${NC} $*"
}

function log_error() {
	echo -e "${RED}[ERROR]${NC} $*"
}

function log_warn() {
	echo -e "${YELLOW}[WARN]${NC} $*"
}

# 查找 git 仓库根目录
function find_git_root() {
	local dir="${1:-$(pwd)}"
	while [[ $dir != "/" ]]; do
		if [[ -d "$dir/.git" ]]; then
			echo "$dir"
			return 0
		fi
		dir=$(dirname "$dir")
	done
	return 1
}

function create_worktree() {
	local worktree_name="$1"
	local branch_name="${2:-$1}"

	# 检查参数
	if [[ -z $worktree_name ]]; then
		log_error "请提供 worktree 名称"
		show_help
		exit 1
	fi

	# 查找 git 根目录
	local git_root
	if ! git_root=$(find_git_root); then
		log_error "当前目录不在 git 仓库中"
		exit 1
	fi

	log_info "检测到 git 仓库: $git_root"

	# 确定 worktree 基础目录
	local worktree_base
	if [[ -n $WORKTREE_BASE ]]; then
		worktree_base="$WORKTREE_BASE"
	else
		worktree_base="${git_root}/.worktrees"
	fi

	local worktree_path="${worktree_base}/${worktree_name}"

	# 检查主仓库
	if [[ ! -d "$git_root/.git" ]]; then
		log_error "主仓库不存在: $git_root"
		exit 1
	fi

	cd "$git_root"

	# 确保基础目录存在
	if [[ ! -d $worktree_base ]]; then
		log_info "创建 worktree 基础目录: $worktree_base"
		mkdir -p "$worktree_base"
	fi

	# 检查分支是否存在，不存在则创建
	if git rev-parse --verify "$branch_name" &>/dev/null; then
		log_info "使用已有分支: $branch_name"
	else
		log_info "创建新分支: $branch_name"
		git branch "$branch_name"
	fi

	# 创建 worktree
	if [[ ! -d $worktree_path ]]; then
		log_info "创建 worktree: $worktree_name"
		git worktree add "$worktree_path" "$branch_name"
		log_success "worktree 创建完成: $worktree_path"
	else
		log_warn "worktree 已存在: $worktree_path"
	fi

	log_success "完成! worktree 路径: $worktree_path"
}

# 解析参数
WORKTREE_NAME=""
BRANCH_NAME=""

while [[ $# -gt 0 ]]; do
	case "$1" in
		-h | --help)
			show_help
			exit 0
			;;
		-b | --base-dir)
			WORKTREE_BASE="$2"
			shift 2
			;;
		-*)
			log_error "未知选项: $1"
			show_help
			exit 1
			;;
		*)
			if [[ -z $WORKTREE_NAME ]]; then
				WORKTREE_NAME="$1"
			elif [[ -z $BRANCH_NAME ]]; then
				BRANCH_NAME="$1"
			else
				log_error "太多参数"
				show_help
				exit 1
			fi
			shift
			;;
	esac
done

# 检查必需参数
if [[ -z $WORKTREE_NAME ]]; then
	show_help
	exit 1
fi

# 执行
create_worktree "$WORKTREE_NAME" "${BRANCH_NAME:-$WORKTREE_NAME}"
