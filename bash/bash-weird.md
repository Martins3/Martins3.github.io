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
