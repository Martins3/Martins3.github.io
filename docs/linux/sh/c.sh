#!/bin/bash

var=aaaaaaaaaaaaabbbbbbbbbbb
sub_string=aabb

# sub_string=aaaaaaaaaaaaabbbbbbbbbbb
# var=aabb

# 通过 ** 匹配
if [[ "${var}" == *"${sub_string}"* ]]; then
    printf '%s\n' "$sub_string is in $var."
fi

# 通过 bash 内置的 =~ 判断
if [[ "${var}" =~ ${sub_string} ]]; then
    printf '%s\n' "$sub_string is in $var."
fi
