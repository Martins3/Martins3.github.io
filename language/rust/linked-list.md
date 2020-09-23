# notes about linked list in rust

## https://rust-unofficial.github.io/too-many-lists/

### perface
> 作者论述了，我们不应该自己实现链表，而且链表的使用范围非常有限


### A bad stack
Linked lists are something procedural programmers shouldn't touch with a 10-foot pole, and what functional programmers use for everything.
> why functional programmers have use linked list



It also still suffers from non-uniformly allocating our elements.

> 最后一次升级中间，为了防止出现private type leak，申明成为 struct 就可以，enum 不行
```
...
9 |     More(Box<Node>),
  |          ^^^^^^^^^ can't leak private type
```

Some of you might be thinking "this is clearly tail recursive, and any decent language would ensure that such code wouldn't blow the stack". This is, in fact, incorrect! To see why, let's try to write what the compiler has to do, by manually implementing Drop for our List as the compiler would:
> linked list 的默认实现的drop 并不是 tail recursive　的，为此需要手动帮助其实现

