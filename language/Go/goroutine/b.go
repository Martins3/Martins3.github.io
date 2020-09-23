package main

import (
	"fmt"
	"time"
)

func hello() {
	for i := 0; i < 10; i++ {
		fmt.Println("Hello world goroutine")
	}
}

func main() {
	go hello()
	time.Sleep(1 * time.Second)
	hello()
	fmt.Println("main function")
}
