#!/usr/bin/env bash
set -E -e -u -o pipefail

set -x
echo 0 | sudo tee /sys/kernel/hacking/genetlink
# shellcheck disable=2046
gcc genetlink.user.c $(pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0) -o genetlink.out
./genetlink.out
