https://www.zhihu.com/question/399923003/answer/1272134898 : 有一个学习路线

资源:
https://github.com/geektutu/7days-golang : 如果真的想搞分布式，那么这就是最佳的入手
https://github.com/inancgumus/learngo : 通过例子学习 golang

https://github.com/shomali11/go-interview : go 语言基础
https://github.com/go-vgo/robotgo : 应该很有意思的东西，用于实现桌面自动化

https://github.com/golang-standards/project-layout : go 项目安排，c++ 有类似的教学 repo

https://github.com/Alikhll/golang-developer-roadmap : golang 的 roadmap，看来都是搞后端的

**https://github.com/halfrost/LeetCode-Go** : finished understand of go and c++ simultaneously.

https://github.com/google/crfs : fuse, 3000 lines,  Go is taking over the world

https://github.com/ofabry/go-callvis : The purpose of this tool is to provide developers with a visual overview of a Go program using data from call graph and its relations with packages and types. This is especially useful in larger projects where the complexity of the code much higher or when you are just simply trying to understand code of somebody else.

https://geektutu.com/post/high-performance-go.html

https://eli.thegreenplace.net/2021/rest-servers-in-go-part-1-standard-library/ : tutorial go for RESTFUL

[Go 语言设计与实现](https://draveness.me/golang/) 德莱文写的

# Go
The reason to learn go is that I don't want to miss a good community!
As for how to learn it, I think the best !

:= a shothand for create a variable and assign a value to it !

## Interface
It seems that go is going back to the C. No class, only struct.

Interface heavily rely on **type**, which means T * and T are totally different !

Interface just work with method receiver ?

Interfaces are implemented implicitly
A type implements an interface by implementing its methods. There is no explicit declaration of intent, no "implements" keyword.
Implicit interfaces decouple the definition of an interface from its implementation, which could then appear in any package without prearrangement.




## tips
1. [slice](https://stackoverflow.com/questions/39984957/is-it-possible-to-initialize-golang-slice-with-specific-values)
```go
onesSlice := make([]int, 5)
for i := range onesSlice {
    onesSlice[i] = 1
}
```
2. [Init array of structs in Go](https://stackoverflow.com/questions/26159416/init-array-of-structs-in-go)
```go
	kva := []mr.KeyValue{}
```


## Question
1. what's make, is a **new** in c++
2. is there generic
3. does go has copy-constructor
4. code as below
```
type IntHeap []int

func (h IntHeap) Len() int           { return len(h) }
func (h IntHeap) Less(i, j int) bool { return h[i] < h[j] }
func (h IntHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }

func (h *IntHeap) Push(x interface{}) {
    // Push and Pop use pointer receivers because they modify the slice's length,
    // not just its contents.
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
why return type is `interface{}`

## External Links
1. https://tour.go-zh.org/basics/7
- https://github.com/go101/go101
  - https://github.com/go101/go101/issues/132
