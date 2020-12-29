## 6.824

- https://github.com/ivanallen/thor : **mit : 首先观看的内容 其他的到时候再说**
- https://github.com/wlgq2/MIT-6.824-2018 : 6.824 的某一个人的作业
- https://fantastickb.com/posts/6.824_0_introduction/
  - https://blog.microdba.com/archive/?tag=6.824 : 对于课程的记录
https://www.zhihu.com/question/29597104/answer/128443409 : pingcap 的作者谈如何学习 MIT 6.218

## [Notes in course](https://pdos.csail.mit.edu/6.824/schedule.html)


### Lecture 2
- [ ] https://gobyexample.com/waitgroups
- [ ] Go's select

#### FAQ
**At a high level, a chan is a struct holding a buffer and a lock.**
Sending on a channel involves acquiring the lock, waiting (perhaps
releasing the CPU) until some thread is receiving, and handing off the
message. Receiving involves acquiring the lock and waiting for a
sender. You could implement your own channels with Go sync.Mutex and
sync.Cond.


WaitGroup is fairly special-purpose; it's only useful when waiting
for a bunch of activities to complete. Channels are more
general-purpose; for example, you can communicate values over
channels. You can wait for multiple goroutines using channels, though it
takes a few more lines of code than with WaitGroup.

**A slice is an object that contains a pointer to an array and a start and
end index into that array.** This arrangement allows multiple slices to
share an underlying array, with each slice perhaps exposing a different
range of array elements.

Here's a more extended discussion: **https://blog.golang.org/go-slices-usage-and-internals**


