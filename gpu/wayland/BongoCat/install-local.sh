#!/usr/bin/env bash
set -E -e -u -o pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
cargo build --release --locked
if [[ -e "$HOME/.local/bin/bongocat" && ! -e "$HOME/.local/bin/bongocat.before-rust" ]]; then
	cp -p -- "$HOME/.local/bin/bongocat" "$HOME/.local/bin/bongocat.before-rust"
fi
install -Dm755 target/release/bongocat "$HOME/.local/bin/bongocat"
install -Dm644 extension/extension.js "$HOME/.local/share/gnome-shell/extensions/bongocat-core@local/extension.js"
install -Dm644 extension/metadata.json "$HOME/.local/share/gnome-shell/extensions/bongocat-core@local/metadata.json"
echo 'Installed. If GNOME does not yet list the extension, log out and back in.'
echo 'Enable with: gnome-extensions enable bongocat-core@local'
