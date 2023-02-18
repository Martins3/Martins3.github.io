# 记录下 bash 中让人抓狂内容

## 1
```sh
set -e
function b() {
  cat g
  cat g
  cat g
  cat g
  cat g
}

function a() {
  cat g || true

  b # 如果直接调用 b ，那么 b 中第一个就失败
  a=$(b) # 但是如果是这种调用方法，b 中失败可以继续
}

a
```

## 2

```sh
A=$@
```

调用的时候，A 将会是一个 string 而不是 array 了。
