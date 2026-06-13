#!/usr/bin/env bash
set -E -e -u -o pipefail

source "$HOME/data/vn/docs/blog/anki-common.sh"

function check_all() {
	local selection
	local uuid
	local file
	local line
	local title

	readarray -t array < <(
		while IFS=$'\t' read -r uuid file line title; do
			printf '%s\t%s\t%s\t%s\n' "$title" "$file" "$line" "$uuid"
		done < <(scan_card_entries_with_title)
	)

	selection=$(printf "%s\n" "${array[@]}" | fzf --delimiter=$'\t' --with-nth=1)

	if [[ -z $selection ]]; then
		return
	fi

	file=$(echo "$selection" | cut -f2)
	line=$(echo "$selection" | cut -f3)
	nvim "+${line}" "${file}"
}

check_neovim_env
check_all
