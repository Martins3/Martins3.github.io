#!/usr/bin/env bash
set -E -e -u -o pipefail

lsmod | grep -q cpuid
