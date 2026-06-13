#!/usr/bin/env bash

set -E -e -u -o pipefail
PROGNAME=$(basename "")
PROGDIR=$(readlink -m "$(dirname "")")
for i in "$@"; do
  echo "$i"
done
cd "$(dirname "$0")"

entries=(
  # "processor"
  # "vendor_id"
  # "cpu family"
  # "model"
  # "model name"
  # "stepping"
  # "microcode"
  # "cpu MHz"
  # "cache size"
  # "physical id"
  # "siblings"
  # "core id"
  # "cpu cores"
  "^apicid"
  # "initial apicid"
  # "fpu"
  # "fpu_exception"
  # "cpuid level"
  # "wp"
  # "flags"
  # "vmx flags"
  # "bugs"
  # "bogomips"
  # "clflush size"
  # "cache_alignment"
  # "address sizes"
  # "power management"
)

for i in "${entries[@]}"; do
  echo "$i"
done

# apicid		: 72
# initial apicid	: 72

for i in "${entries[@]}"; do
  # cat /proc/cpuinfo | grep "$i" | tee -a a
  cat /proc/cpuinfo | grep "$i"
done
