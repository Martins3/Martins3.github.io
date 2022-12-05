#!/usr/bin/env bash

function usage() {
  echo "Usage :   [options] [--]

    Options:
    -h|help       Display this message"
}

cmd=""
while getopts "hc:" opt; do
  case $opt in
  c) cmd=${OPTARG} ;;
  h)
    usage
    exit 0
    ;;
  *)
    echo -e "\n  Option does not exist : OPTARG\n"
    usage
    exit 1
    ;;
  esac # --- end of case ---
done
shift $((OPTIND - 1))

case "$cmd" in
1)
  echo "统计内核中以 attach 开头的执行次数"
  funccount-bpfcc 'attach*'
  ;;
2)
  echo "统计函数中以 hello 开头的执行次数"
  funccount-bpfcc './program.out:hello*'
  ;;
3)
  echo "跟踪函数和其参数"
  trace-bpfcc './program.out:add "%d %d", arg1, arg2'
  ;;
4)
  echo "跟踪函数和其返回值"
  # TMP_TODO 我们发现在 return 位置上增加一个 arg1 的时候
  # trace-bpfcc 'r:./program.out:add "%d %d", retval'
  trace-bpfcc 'r:./program.out:add "%d %d %d", arg1, arg2, retval'
  ;;
  # TMP_TODO BFP performance Tool P12.2.6 最后的举例 argdist, stackcount 和 profile 没看懂
esac
