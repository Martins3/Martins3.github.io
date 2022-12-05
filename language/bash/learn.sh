#!/usr/bin/env bash

echo "one_two_three
three_two
four_gg
this_k" | awk -F'_' '{print $1}'

echo "one_two_three" | awk -F_ '{print NR " " $(NF - 1) " " $(NF)}'
echo "one_two_three" | awk -F_ '{print NR " " $(NF - 1) " " NF}'
