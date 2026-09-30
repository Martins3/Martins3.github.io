> Notification:
> This article are some useful snippets for OnlineJudge, especially for Leetcode.

1. sort int slice

```go
	sort.Slice(nums, func(i, j int) bool {
		return nums[i] < nums[j]
	})
```

2. sort struct

```go
	sort.Slice(people, func(i, j int) bool {
		return people[i].age < people[j].age
	})
```

3. int heap
```go
type IntHeap []int

// h.Len()
func (h IntHeap) Len() int           { return len(h) }
func (h IntHeap) Less(i, j int) bool { return h[i] < h[j] }
func (h IntHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }
func (h *IntHeap) Peek() int         { return (*h)[0] }


// heap.Push(h, val)
func (h *IntHeap) Push(x interface{}) {
	*h = append(*h, x.(int))
}

func (h *IntHeap) Pop() interface{} {
	old := *h
	n := len(old)
	x := old[n-1]
	*h = old[0 : n-1]
	return x
}
```



## String
[doc](https://golang.org/pkg/strings/?m=all)

1. reverse
```go
func Reverse(s string) string {
	r := []rune(s)
	for i, j := 0, len(r)-1; i < len(r)/2; i, j = i+1, j-1 {
		r[i], r[j] = r[j], r[i]
	}
	return string(r)
}
```
2. swap string
```
func swap(str1, str2 *string) {
    *str1, *str2 = *str2, *str1
}
func main() {
    a := "salut"
    b := "les gens"
    swap(&a, &b)
    fmt.Printf("a=%s\nb=%s\n", a, b)
}
```

3. empty string
```go
var s string
fmt.Println(s=="") // prints "true"
```

## slice
1. Add a element at the first
```go
data := []string{"A", "B", "C", "D"}
data = append([]string{"Prepend Item"}, data...)
fmt.Println(data)
// [Prepend Item A B C D]
```
2. push back

## stack
```go
// A unsafe thread !

type stack []int

func (s stack) Push(v int) stack {
    return append(s, v)
}

func (s stack) Pop() (stack, int) {
    l := len(s)
    return  s[:l-1], s[l-1]
}

func main(){
    s := make(stack,0)
    s = s.Push(1)
    s = s.Push(2)
    s = s.Push(3)

    s, p := s.Pop()
    fmt.Println(p)
}
```

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
