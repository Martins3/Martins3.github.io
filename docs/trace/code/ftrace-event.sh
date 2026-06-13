#!/usr/bin/env bash

set -E -e -u -o pipefail
cd /sys/kernel/debug/tracing
cd events
ls
