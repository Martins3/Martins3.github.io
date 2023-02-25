# 双引号

在对于含有空格的可以采用双引号包围，但是
$var 的时候自动去掉双引号
```sh
set -x
entries=(
  "cpu family"
)

for i in "${entries[@]}"; do
  cat /proc/cpuinfo | grep "$i" | tee a
done
```
## glob, spliting and empty remove

ust like in any argument to any command, variable expansions must be quoted to prevent split+glob and empty removal


## [ 和 [[ 的关系

## 这个为什么失败？
printf "-- %s ---" $a

## 数值系统太模糊，各种装换
仅仅支持整数，不然就是 awk 或者 bc
$((1 + 2))

## 整理清楚各种 regex 吧

## 如何命令成功，这个是不是和 [[ [ 之类描述是反过来的
https://unix.stackexchange.com/questions/22726/how-to-conditionally-do-something-if-a-command-succeeded-or-failed
