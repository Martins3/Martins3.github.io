package main

import (
	"fmt"
	"time"
)

func say(s string) {
	for i := 0; i < 5; i++ {
		time.Sleep(1000 * time.Millisecond)
		fmt.Print(s)
	}
}

func main() {
	go say("world ")
	say("hello ")
	say(" !\n")
}
