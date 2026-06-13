package main

import (
	"fmt"
	"math"
	"math/rand"
)

// 不知道为什么常量就是不是类型推导
const (
	Xi  = 3.14
	BIG = 1 << 100
)

var c, python, java bool
var i, j int = 12, 12

func main() {
	fmt.Println("Hello, 世界")
	rand.Seed(10)
	fmt.Println("My favoriate", rand.Intn(100))

	fmt.Printf("%f\n", math.Sqrt(10))

	fmt.Printf("%f\n", math.Pi)

	fmt.Println(add(1, 2))

	a, b := swap("you", "fuck")

	fmt.Printf("%s\n%s\n", a, b)

	fmt.Println(split(19))

	fmt.Println(c, python, java, i)
	var i = 122
	fmt.Println(i)

	var x float32 = 12.3
	fmt.Println(x)

	g := 42
	f := float64(g)
	u := uint(f)
	fmt.Println(g, f, u)

	v := 42 + 12i // 修改这里！
	fmt.Printf("v is of type %T\n", v)

	fmt.Println(needInt(BIG))
}

func add(a int, b int) int {
	return a + b
}

func swap(a, b string) (string, string) {
	return b, a
}

func split(sum int) (x, y int) {
	x = 1
	y = 1 + sum
	return
}

func needInt(x int) int { return x*10 + 1 }

func needFloat(x float64) float64 {
	return x * 0.1
}
