#!/usr/bin/env bash
set -E -e -u -o pipefail

# 更换版本时，需要同时更新下载文件的 SHA256 和对应的 Clang 主版本。
readonly VERSION='21.1'
readonly CLANG_MAJOR='21'
readonly SHA256='84814776ba39ac901c467ce93b2171db3ca563db9c4dd06836d79d45a3a23d8d'
readonly INSTALL_DIR="$HOME/.local/share/cppinsights/v_${VERSION}"

[[ "$(uname -sm)" == 'Linux x86_64' ]] || {
	echo '需要 Linux x86_64'
	exit 1
}
resource_dir="$(clang++ -print-resource-dir)"
[[ $resource_dir == */"$CLANG_MAJOR" && -f "$resource_dir/include/stddef.h" ]] || {
	echo "需要 Clang $CLANG_MAJOR 的头文件"
	exit 1
}

mkdir -p "$INSTALL_DIR/clang" "$HOME/.local/bin"

curl -fL --retry 3 \
	"https://github.com/andreasfertig/cppinsights/releases/download/v_${VERSION}/insights-ubuntu-amd64.tar.gz" \
	-o "$INSTALL_DIR/insights.tar.gz"
printf '%s  %s\n' "$SHA256" "$INSTALL_DIR/insights.tar.gz" | sha256sum -c -

tar -xOzf "$INSTALL_DIR/insights.tar.gz" insights >"$INSTALL_DIR/insights.out"

# 官方二进制内嵌了 Clang 解析器，但压缩包没有附带 stddef.h 等内置头文件。
# 从本机同主版本的 Clang 复制这些文件，避免解析源码时出现头文件找不到的错误。
# -L 将符号链接解析为实际文件，安装后不再依赖原头文件目录的位置。
# C/C++ 标准库头文件仍由本机已有的开发环境提供。
cp -RL "$resource_dir/include" "$INSTALL_DIR/clang/"

# 生成启动封装，自动传入头文件目录，使用时只需执行 insights。
# 路径由顶部常量生成；printf 的 %q 会正确转义路径中的空格等特殊字符。
{
	cat <<'EOF'
#!/usr/bin/env bash
set -E -e -u -o pipefail
# 自动指定随工具安装的 Clang 内置头文件，并原样转发用户参数。
EOF
	printf 'exec %q --extra-arg-before=%q "$@"\n' "$INSTALL_DIR/insights.out" "-resource-dir=$INSTALL_DIR/clang"
} >"$HOME/.local/bin/insights"

chmod +x "$INSTALL_DIR/insights.out" "$HOME/.local/bin/insights"
"$HOME/.local/bin/insights" --version
echo "安装完成：$HOME/.local/bin/insights"
