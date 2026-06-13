#!/usr/bin/env bash
set -E -e -u -o pipefail

# shellcheck disable=SC2034
readonly ANKI_BIN="$HOME/data/vn/docs/blog/fsrs/target/debug/anki-fsrs"
readonly CACHE_DIR="/tmp/anki_cache"
readonly CACHE_FILE="$CACHE_DIR/uuid_mapping.cache"
readonly UUID_PATTERN='[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}'

readonly -a ANKI_PROJECT_ROOTS=(
	"$HOME/data/vn"
	"$HOME/data/anki"
)

function check_neovim_env() {
	if env | grep neovim >/dev/null; then
		echo "Don't run nvim in nvim"
		exit 1
	fi
}

function scan_card_entries_with_title() {
	local -a roots=()
	local root

	for root in "${ANKI_PROJECT_ROOTS[@]}"; do
		[[ -d $root ]] || continue
		roots+=("$root")
	done

	[[ ${#roots[@]} -gt 0 ]] || return 0

	# shellcheck disable=SC2016
	{
		rg -l -0 -u --no-messages --color=never -g '!*.svg' "$UUID_PATTERN" "${roots[@]}" 2>/dev/null || true
	} \
		| xargs -0 -r awk '
			FNR == 1 {
				prev = ""
				prev_nr = 0
			}

			{
				if (match($0, /<!--[[:space:]]*[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}[[:space:]]*-->/)) {
					uuid = substr($0, RSTART, RLENGTH)
					sub(/^<!--[[:space:]]*/, "", uuid)
					sub(/[[:space:]]*-->$/, "", uuid)
					comment_col = RSTART

					if (prev_nr > 0 && match(prev, /#/)) {
						header_col = RSTART
						if (header_col == comment_col) {
							title = prev
							sub(/^.*#[[:space:]]*/, "", title)
							printf "%s\t%s\t%d\t%s\n", uuid, FILENAME, prev_nr, title
						}
					}
				}

				prev = $0
				prev_nr = FNR
			}
		'
}

function scan_card_entries() {
	scan_card_entries_with_title | cut -f1-3
}

function rebuild_cache() {
	rm -rf "$CACHE_DIR"
	mkdir -p "$CACHE_DIR"

	scan_card_entries >"$CACHE_FILE"
}

function ensure_cache_exists() {
	if [[ ! -f $CACHE_FILE ]]; then
		rebuild_cache
	fi
}
