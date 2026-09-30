#!/usr/bin/env bash

set -E -e -u -o pipefail

export LANG=C.UTF-8
export LC_ALL=C.UTF-8
runtime_dir="/run/user/$(id -u)"
export XDG_RUNTIME_DIR="$runtime_dir"
export DBUS_SESSION_BUS_ADDRESS="unix:path=$runtime_dir/bus"

exec /usr/bin/gdbus call \
	--session \
	--dest org.freedesktop.Notifications \
	--object-path /org/freedesktop/Notifications \
	--method org.freedesktop.Notifications.Notify \
	"企业微信" \
	0 \
	"dialog-information" \
	"企业微信" \
	"有新消息" \
	'[]' \
	'{}' \
	5000
