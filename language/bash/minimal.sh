#!/bin/sh


# https://stackoverflow.com/questions/8579399/why-does-true-false-evaluate-to-1-in-bash


exit 0
OVERLAY_BUNDLES="a b c"
BUNDLES_LIST="$(echo $OVERLAY_BUNDLES | tr ',' ' ')"

for i in $BUNDLES_LIST; do
  echo $i
done


for i in $OVERLAY_BUNDLES; do
  echo $i
done

JJ=abc

# 试试证明，比较不需要那么多的蛇皮内容 !
if [ "$JJ" = "abc" ]; then 
  echo "jj"
fi


FILE=/home/shen/Core/Vn/hack/lab/env.md
sed -i "s/fasdfasdf/jjjjjj/" $FILE


if [ 1 ]; then
  echo "fuck true"
fi

if [ ! 0 ]; then
  echo "fuck false"
fi

if [ 1 ] && [ ! 0 ]; then
  echo "fuck both"
fi

if [ 1 -a  0 ]; then
  echo "fuck both"
fi


DOWNLOAD_URL=http://kernel.org/pub/linux/kernel/v5.x/linux-5.4.3.tar.xz

# TODO 好吧，可以背下来，但是实际需要更加深刻的理解
# https://wiki.bash-hackers.org/syntax/pe

# Grab everything after the last '/' character.
ARCHIVE_FILE=${DOWNLOAD_URL#*/}
echo $ARCHIVE_FILE
