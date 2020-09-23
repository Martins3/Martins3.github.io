#!/usr/bin/env bash

F=/home/shen/Core/Vn/t.md

while IFS= read -r line ;do
  array=( $line ) # do not use quotes in order to allow word expansion
  jj=${array[0]}

  if [ "$jj" = "C"   ];then
    echo $line
  fi

  if [ "$jj" = 'C/C++' ];then
    echo $line
  fi


done < "$F"




