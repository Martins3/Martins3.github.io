#!/bin/bash
FILES=./*

NAME=
TITLE=

for f in $FILES
do
  echo "Processing $f file..."
  mv "$f" "${f%.*}_${NAME}_${TITLE}.${f##*.}"
done
exit 0
