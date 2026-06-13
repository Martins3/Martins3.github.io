#!/usr/bin/env bash
set -e
WORKDIR=/home/maritns3/core/vn/tmp/flame
flamegraph_pl=${WORKDIR}/flamegraph.pl
stackcount_data=${WORKDIR}/stackcount.out
img=${WORKDIR}/stackcount.svg

if [[ ! -d ${WORKDIR} ]]; then
  wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl -O ${flamegraph_pl}
fi

stackcount-bpfcc -f -P -D 3 do_sys_open > ${stackcount_data}
perl ${flamegraph_pl} --title "martins3" --hash --bgcolors=grey < ${stackcount_data} > ${img}
