//! 经典面试题：翻转单链表
//!
//! Rust 里链表节点用 `Box` 独占所有权，避免 Rc/RefCell 的运行时开销。
//!
//! 基础实现来自 Learning Rust With Entirely Too Many Lists 的 second.rs
//! (An Ok Singly-Linked Stack)，并在 `List<T>` 上加入从切片构建、转换为
//! `Vec<T>` 和翻转链表的接口。

type Link<T> = Option<Box<Node<T>>>;

#[derive(Debug)]
struct Node<T> {
    elem: T,
    next: Link<T>,
}

impl<T> Node<T> {
    fn new(elem: T, next: Link<T>) -> Link<T> {
        Some(Box::new(Node { elem, next }))
    }
}

/// 演示链表的翻转和栈操作。
#[cfg_attr(test, test)]
pub fn run_all() {
    println!("=== 翻转单链表 ===");

    let list = List::from_slice(&[1, 2, 3, 4, 5]);
    println!("原链表:     {:?}", list.into_vec());

    let list = List::from_slice(&[1, 2, 3, 4, 5]);
    println!("迭代翻转后: {:?}", list.reverse().into_vec());
    println!("空链表:     {:?}", List::<i32>::new().reverse().into_vec());

    println!("=== Ok Singly-Linked Stack (too-many-lists second) ===");
    let mut list = List::new();
    list.push(1);
    list.push(2);
    list.push(3);
    println!("push 1,2,3 后 peek: {:?}", list.peek());
    println!("iter 输出:        {:?}", list.iter().collect::<Vec<_>>());

    if let Some(value) = list.peek_mut() {
        *value = 42;
    }
    println!("peek_mut 改 42 后: {:?}", list.peek());

    for v in list.iter_mut() {
        *v += 10;
    }
    println!("iter_mut 每个+10:  {:?}", list.iter().collect::<Vec<_>>());

    println!(
        "into_iter 输出:    {:?}",
        list.into_iter().collect::<Vec<_>>()
    );

    let mut list2 = List::new();
    list2.push(10);
    list2.push(20);
    println!("pop2 演示:         {:?} {:?}", list2.pop2(), list2.pop2());
}

// =====================================================================
// Ok Singly-Linked Stack
// 来源: Learning Rust With Entirely Too Many Lists 的 second.rs
// =====================================================================

pub struct List<T> {
    head: Link<T>,
}

impl<T> List<T> {
    pub fn new() -> Self {
        List { head: None }
    }

    /// 从切片构建链表，保持元素顺序。
    pub fn from_slice(values: &[T]) -> Self
    where
        T: Clone,
    {
        let mut head = None;
        for elem in values.iter().rev() {
            head = Node::new(elem.clone(), head);
        }
        List { head }
    }

    /// 消耗链表，并按照链表顺序返回所有元素。
    pub fn into_vec(mut self) -> Vec<T> {
        let mut out = Vec::new();
        while let Some(elem) = self.pop() {
            out.push(elem);
        }
        out
    }

    /// 迭代翻转链表：逐个摘下头节点，再插到新链表头部。
    pub fn reverse(mut self) -> Self {
        let mut head = self.head.take();
        let mut next;
        let mut list = None;

        while let Some(mut node) = head {
            // next = node.next;
            // 一般来说，倾向于使用 take ，而不是直接 move
            // 是的，这里的一个简单赋值就是 move 的意思

            next = node.next.take(); // 1. 摘下旧后继
            node.next = list; // 2. 当前节点指向新链表头
            list = Some(node); // 3. 当前节点成为新链表头
            head = next; // 4. 继续处理原链表剩余部分
        }

        List { head: list }
    }

    pub fn push(&mut self, elem: T) {
        self.head = Node::new(elem, self.head.take());
    }

    pub fn pop(&mut self) -> Option<T> {
        self.head.take().map(|node| {
            self.head = node.next;
            node.elem
        })
    }

    // 演示一个 map 的场景
    pub fn pop2(&mut self) -> Option<T> {
        match self.head.take() {
            Some(node) => {
                self.head = node.next;
                Some(node.elem)
            }
            None => None,
        }
    }

    pub fn peek(&self) -> Option<&T> {
        self.head.as_ref().map(|node| &node.elem)
    }

    pub fn peek_mut(&mut self) -> Option<&mut T> {
        self.head.as_mut().map(|node| &mut node.elem)
    }

    pub fn into_iter(self) -> IntoIter<T> {
        IntoIter(self)
    }

    pub fn iter(&self) -> Iter<'_, T> {
        Iter {
            next: self.head.as_deref(),
        }
    }

    pub fn iter_mut(&mut self) -> IterMut<'_, T> {
        IterMut {
            next: self.head.as_deref_mut(),
        }
    }
}

// 那么当生命周期结束，就会自动的消失，如果实现了 drop ，就可以观察到这个结果:
// #0  demo::linked_list::{impl#2}::drop<i32> (self=0x7fffffff5b68) at src/linked_list.rs:168
// #1  0x000055555556ee53 in core::ptr::drop_in_place<demo::linked_list::List<i32>> () at /rustc/59807616e1fa2540724bfbac14d7976d7e4a3860/library/core/src/ptr/mod.rs:805
// #2  0x0000555555564357 in demo::linked_list::List<i32>::into_vec<i32> (self=...) at src/linked_list.rs:96
// #3  0x0000555555568030 in demo::linked_list::run_all () at src/linked_list.rs:28
// #4  0x0000555555574ed3 in demo::main () at src/main.rs:414

impl<T> Drop for List<T> {
    fn drop(&mut self) {
        let mut cur_link = self.head.take();
        while let Some(mut boxed_node) = cur_link {
            cur_link = boxed_node.next.take();
        }
    }
}

pub struct IntoIter<T>(List<T>);

impl<T> Iterator for IntoIter<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> {
        self.0.pop()
    }
}

pub struct Iter<'a, T> {
    next: Option<&'a Node<T>>,
}

impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;
    fn next(&mut self) -> Option<Self::Item> {
        self.next.map(|node| {
            self.next = node.next.as_deref();
            &node.elem
        })
    }
}

pub struct IterMut<'a, T: 'a> {
    next: Option<&'a mut Node<T>>,
}

impl<'a, T> Iterator for IterMut<'a, T> {
    type Item = &'a mut T;

    fn next(&mut self) -> Option<Self::Item> {
        self.next.take().map(|node| {
            self.next = node.next.as_deref_mut();
            &mut node.elem
        })
    }
}

#[cfg(test)]
mod test {
    use super::List;

    #[test]
    fn reverse_list() {
        assert_eq!(
            List::from_slice(&[1, 2, 3, 4, 5]).reverse().into_vec(),
            [5, 4, 3, 2, 1]
        );

        assert!(List::<i32>::new().reverse().into_vec().is_empty());
    }

    #[test]
    fn basics() {
        let mut list = List::new();

        // Check empty list behaves right
        assert_eq!(list.pop(), None);

        // Populate list
        list.push(1);
        list.push(2);
        list.push(3);

        // Check normal removal
        assert_eq!(list.pop(), Some(3));
        assert_eq!(list.pop(), Some(2));

        // Push some more just to make sure nothing's corrupted
        list.push(4);
        list.push(5);

        // Check normal removal
        assert_eq!(list.pop(), Some(5));
        assert_eq!(list.pop(), Some(4));

        // Check exhaustion
        assert_eq!(list.pop(), Some(1));
        assert_eq!(list.pop(), None);
    }

    #[test]
    fn peek() {
        let mut list = List::new();
        assert_eq!(list.peek(), None);
        assert_eq!(list.peek_mut(), None);
        list.push(1);
        list.push(2);
        list.push(3);

        assert_eq!(list.peek(), Some(&3));
        assert_eq!(list.peek_mut(), Some(&mut 3));

        if let Some(value) = list.peek_mut() {
            *value = 42;
        }

        assert_eq!(list.peek(), Some(&42));
        assert_eq!(list.pop(), Some(42));
    }

    #[test]
    fn into_iter() {
        let mut list = List::new();
        list.push(1);
        list.push(2);
        list.push(3);

        let mut iter = list.into_iter();
        assert_eq!(iter.next(), Some(3));
        assert_eq!(iter.next(), Some(2));
        assert_eq!(iter.next(), Some(1));
        assert_eq!(iter.next(), None);
    }

    #[test]
    fn iter() {
        let mut list = List::new();
        list.push(1);
        list.push(2);
        list.push(3);

        let mut iter = list.iter();
        assert_eq!(iter.next(), Some(&3));
        assert_eq!(iter.next(), Some(&2));
        assert_eq!(iter.next(), Some(&1));
    }

    #[test]
    fn iter_mut() {
        let mut list = List::new();
        list.push(1);
        list.push(2);
        list.push(3);

        let mut iter = list.iter_mut();
        assert_eq!(iter.next(), Some(&mut 3));
        assert_eq!(iter.next(), Some(&mut 2));
        assert_eq!(iter.next(), Some(&mut 1));
    }
}
