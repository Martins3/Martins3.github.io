#!/usr/bin/env bash
set -E -e -u -o pipefail

readonly DAIRY_DIR="$HOME/data/anki/dairy"

function main() {
	local filename
	local filepath
	local timestamp

	filename=$(date '+%Y-%m-%d')
	filepath="${DAIRY_DIR}/${filename}"
	timestamp=$(date '+%Y-%m-%d %H:%M:%S %z')

	mkdir -p "$DAIRY_DIR"

	if [[ -e $filepath ]]; then
		echo "already exists: $filepath"
	else
		printf '%s\n' "$timestamp" >"$filepath"
		echo "$filepath"
	fi

	cd "$(dirname "$filepath")"
	nvim "$filepath"
}

main "$@"
