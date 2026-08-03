#!/usr/bin/env bash
set -E -e -u -o pipefail

insmod ./snull.ko
ifconfig sn0 local0
ifconfig sn1 local1
