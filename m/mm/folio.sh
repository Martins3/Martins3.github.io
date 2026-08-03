#!/usr/bin/env bash
set -E -e -u -o pipefail
set -x

cd "$(dirname "$0")"
gcc share-user.c -o share-user.out
../mod.sh -r
../mod.sh share 1
sudo ./share-user.out &
sleep 1
../mod.sh share 1
# sudo pkill share-user.out
