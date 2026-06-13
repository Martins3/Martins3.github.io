#!/usr/bin/env bash

entry=kprobe:bcachefs_exit
if [[ $entry =~ kprobe:.* ]]; then
	echo "hi"
fi

entry=${entry##*:}
echo "$entry"
