# [Learning Rust With Entirely Too Many Linked Lists](https://rust-unofficial.github.io/too-many-lists/)

## perface
In doing so, you should learn:

- The following pointer types: &, &mut, Box, Rc, Arc, `*const`, `*mut`
- Ownership, borrowing, inherited mutability, interior mutability, Copy
- All The Keywords: struct, enum, fn, pub, impl, use, ...
- Pattern matching, generics, destructors
- Testing
- Basic Unsafe Rust


- [ ] 作者论述了，我们不应该自己实现链表，而且链表的使用范围非常有限
  - [ ] 可以好好看看，试着反驳

## A bad stack
Linked lists are something procedural programmers shouldn't touch with a 10-foot pole, and what functional programmers use for everything.
> why functional programmers have use linked list

It also still suffers from non-uniformly allocating our elements.

> 为了防止出现private type leak，申明成为 struct 就可以，enum 不行
```
...
9 |     More(Box<Node>),
  |          ^^^^^^^^^ can't leak private type
```

Some of you might be thinking "this is clearly tail recursive, and any decent language would ensure that such code wouldn't blow the stack". This is, in fact, incorrect! To see why, let's try to write what the compiler has to do, by manually implementing Drop for our List as the compiler would:
> linked list 的默认实现的drop 并不是 tail recursive　的，为此需要手动帮助其实现
> 因为不是 tail recursive 的，所以在释放的容易 stackoverflow
> 我感觉，除非有特殊的原因，list 的释放显然不可能是自动分配的，为什么要从这种角度论证

## An Ok Singly-Linked Stack
- 测试 2 中间使用提出一个改进，使用 option 来代替 enum 
- 但是，如果仔细思考一下 push 的时候，怎么可以保证一个 List 被释放的时候，其中的所有的节点都可以释放

- 实现内存关系的基本是，一个 object 从 stack 上消失的时候，自动让其持有的资源也消失

- [x] 但是，如果 push Node 的时候，根本不将 Node 连接起来，如何 ?
  - 新创建的 Node 马上消失
  - 思考一下，为什么需要 Box ?
  - 没有 Box 分配 heap 上的内存 ?

- [ ] https://phaazon.net/blog/rust-no-drop
  - 解释了一个实现了 drop 之后，就不可以将自己成员 move 出去
  - 后半段没看懂


- https://hashrust.com/blog/references-in-rust/
  - rust references 可以自动 dereference
- https://hashrust.com/blog/arrays-vectors-and-slices-in-rust/
  - rust 有 array 和 vector，他们都可以支持 slice

We need to add lifetimes only in function and type signatures:

## A Persistent Singly-Linked Stack
In order to get thread safety, we have to use Arc. Arc is completely identical to Rc except for the fact that reference counts are modified atomically. 

The reason this is the case is because Rust models thread-safety in a first-class way with two traits: `Send` and `Sync`.

Interior mutability types violate this: they let you mutate through a shared reference. There are two major classes of interior mutability: cells, which only work in a single-threaded context; and locks, which work in a multi-threaded context.

## A Bad but Safe Doubly-Linked Deque
- [ ] 所以，靠 Rc 只能构建单向链表 ?


## A Bad Save Deque

**WHEN COMMING BACK**

从 section 5.4 开始吧
