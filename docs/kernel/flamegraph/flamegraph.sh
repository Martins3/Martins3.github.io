#!/usr/bin/env bash

# 开始之前
# echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid
# echo 0 | sudo tee /proc/sys/kernel/kptr_restrict
# echo 10000 | sudo tee /proc/sys/kernel/perf_event_max_sample_rate

WORKDIR=/home/martins3/flame

set -ex
function usage() {
  echo "Usage :   [options] [--]

    Options:
    -h|help       Display this message
    -g|grep       Only display what you care about
    -c|cmd        The command to perf
    -b|bind       The cpu to bind

Examples:
    f -c \"sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio\"
"
}

target=""
cmd=""
cpu=""
while getopts "hg:c:b:" opt; do
  case $opt in
  c) cmd=${OPTARG} ;;
  b) cpu="${OPTARG}" ;;
  h)
    usage
    exit 0
    ;;
  g) target=${OPTARG} ;;
  *)
    echo -e "\n  Option does not exist : OPTARG\n"
    usage
    exit 1
    ;;
  esac # --- end of case ---
done
shift $((OPTIND - 1))

echo "cmd=$cmd"
echo "grep=$target"

stackcollapse_pl=${WORKDIR}/stackcollapse-perf.pl
flamegraph_pl=${WORKDIR}/flamegraph.pl
img=${WORKDIR}/flamegraph_dd.svg
perf_data=${WORKDIR}/perf_data.out

if [[ ! -d ${WORKDIR} ]]; then
  mkdir -p ${WORKDIR}
  wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/stackcollapse-perf.pl -O ${stackcollapse_pl}
  wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl -O ${flamegraph_pl}
fi

if [[ -z $cpu ]]; then
  cpu_bind="-a"
else
  cpu_bind="--cpu ${cpu} "
  cmd="sudo taskset -c $cpu ${cmd}"
fi

perf record ${cpu_bind} -g -- $cmd
perf script | perl ${stackcollapse_pl} >${perf_data}

if [[ -z ${target} ]]; then
  perl ${flamegraph_pl} --title "martins3" >${img} <${perf_data}
else
  grep "${target}" ${perf_data} | perl ${flamegraph_pl} --title "trace" >${img}
fi
microsoft-edge-dev "${img}"

# taskset -c 5 fio 4k-read.fio
