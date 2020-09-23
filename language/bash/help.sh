#!/bin/bash
###
### my-script — does one thing well
###
### Usage:
###   my-script <input> <output>
###
### Options:
###   <input>   Input file to read.
###   <output>  Output file to write. Use '-' for stdout.
###   -h        Show this message.
 ### 开头可以不是是^
### 开头必须是^
###?可以没有空格
###                ?仅仅表示0或者1个空格



# TODO 如何仅仅检查是否存在某个文件
help() {
    sed -rn 's/^### ?/ggggggggggg/;t;p' "$0"
}

if [[ $# == 0 ]] || [[ "$1" == "-h" ]]; then
    help
    exit 1
fi
