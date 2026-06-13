package main

import "fmt"

func rotate(nums []int, k int) []int {
	if k < 0 || len(nums) == 0 {
		return nums
	}

	fmt.Printf("nums %p array %p len %d cap %d slice %v\n", &nums, &nums[0], len(nums), cap(nums), nums)

	r := len(nums) - k%len(nums)
	nums = append(nums[r:], nums[:r]...)

	fmt.Printf("nums %p array %p len %d cap %d slice %v\n", &nums, &nums[0], len(nums), cap(nums), nums)

	return nums
}

func main() {
	nums := []int{1, 2, 3, 4, 5, 6, 7}

	fmt.Printf("nums %p array %p len %d cap %d slice %v\n", &nums, &nums[0], len(nums), cap(nums), nums)

	nums = rotate(nums, 3)

	fmt.Printf("nums %p array %p len %d cap %d slice %v\n", &nums, &nums[0], len(nums), cap(nums), nums)
}
