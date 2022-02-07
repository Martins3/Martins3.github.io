#!/bin/bash

function copyFiles {
  declare -n arr=$1
  x=12

  for i in "${arr[@]}"; do
    echo "$i"
  done
}

array=("one" "two" "three")
copyFiles array

echo $x
