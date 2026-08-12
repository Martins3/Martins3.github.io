use std::cell::{Ref, RefCell, RefMut};
use std::fmt;
use std::rc::Rc;

pub struct List<T> {
    head: Link<T>,
    tail: Link<T>,
}

type Link<T> = Option<Rc<RefCell<Node<T>>>>;

struct Node<T> {
    elem: T,
    next: Link<T>,
    prev: Link<T>,
}

impl<T> Node<T> {
    fn new(elem: T) -> Rc<RefCell<Self>> {
        Rc::new(RefCell::new(Node {
            elem,
            prev: None,
            next: None,
        }))
    }
}

impl<T> List<T> {
    pub fn new() -> Self {
        List {
            head: None,
            tail: None,
        }
    }

    pub fn push_front(&mut self, elem: T) {
        let new_head = Node::new(elem);
        match self.head.take() {
            Some(old_head) => {
                old_head.borrow_mut().prev = Some(new_head.clone());
                new_head.borrow_mut().next = Some(old_head);
                self.head = Some(new_head);
            }
            None => {
                self.tail = Some(new_head.clone());
                self.head = Some(new_head);
            }
        }
    }

    pub fn push_back(&mut self, elem: T) {
        let new_tail = Node::new(elem);
        match self.tail.take() {
            Some(old_tail) => {
                old_tail.borrow_mut().next = Some(new_tail.clone());
                new_tail.borrow_mut().prev = Some(old_tail);
                self.tail = Some(new_tail);
            }
            None => {
                self.head = Some(new_tail.clone());
                self.tail = Some(new_tail);
            }
        }
    }

    pub fn pop_back(&mut self) -> Option<T> {
        self.tail.take().map(|old_tail| {
            match old_tail.borrow_mut().prev.take() {
                Some(new_tail) => {
                    new_tail.borrow_mut().next.take();
                    self.tail = Some(new_tail);
                }
                None => {
                    self.head.take();
                }
            }
            Rc::try_unwrap(old_tail).ok().unwrap().into_inner().elem
        })
    }

    pub fn pop_front(&mut self) -> Option<T> {
        self.head.take().map(|old_head| {
            match old_head.borrow_mut().next.take() {
                Some(new_head) => {
                    new_head.borrow_mut().prev.take();
                    self.head = Some(new_head);
                }
                None => {
                    self.tail.take();
                }
            }
            Rc::try_unwrap(old_head).ok().unwrap().into_inner().elem
        })
    }

    pub fn peek_front(&self) -> Option<Ref<'_, T>> {
        self.head
            .as_ref()
            .map(|node| Ref::map(node.borrow(), |node| &node.elem))
    }

    pub fn peek_back(&self) -> Option<Ref<'_, T>> {
        self.tail
            .as_ref()
            .map(|node| Ref::map(node.borrow(), |node| &node.elem))
    }

    pub fn peek_back_mut(&mut self) -> Option<RefMut<'_, T>> {
        self.tail
            .as_ref()
            .map(|node| RefMut::map(node.borrow_mut(), |node| &mut node.elem))
    }

    pub fn peek_front_mut(&mut self) -> Option<RefMut<'_, T>> {
        self.head
            .as_ref()
            .map(|node| RefMut::map(node.borrow_mut(), |node| &mut node.elem))
    }

    pub fn into_iter(self) -> IntoIter<T> {
        IntoIter(self)
    }
}

impl<T> Drop for List<T> {
    fn drop(&mut self) {
        while self.pop_front().is_some() {}
    }
}

// 为什么不实现 iter()？
// 节点包在 Rc<RefCell<_>> 里：iter() 想返回 Ref<'a, T>，但 Ref 无法在持有 'a 生命周期
// 的情况下从当前节点穿到下一个节点（每次 borrow 都要穿过一层 RefCell），
// 而存 Rc 快照又会引入迭代器失效问题（列表 pop 时返回的引用可能悬垂）。
// 《Learn Rust With Entirely Too Many Linked Lists》第四章也是因此放弃 Iter。
// 所以这里直接实现 Debug，让 {:?} 就能打印整个链表。
impl<T: fmt::Debug> fmt::Debug for List<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut list = f.debug_list();
        let mut cur = self.head.clone();
        while let Some(node) = cur {
            let node_ref = node.borrow();
            list.entry(&node_ref.elem);
            cur = node_ref.next.clone();
        }
        list.finish()
    }
}

pub struct IntoIter<T>(List<T>);

impl<T> Iterator for IntoIter<T> {
    type Item = T;

    fn next(&mut self) -> Option<T> {
        self.0.pop_front()
    }
}

impl<T> DoubleEndedIterator for IntoIter<T> {
    fn next_back(&mut self) -> Option<T> {
        self.0.pop_back()
    }
}

/// 演示 `Rc<RefCell<_>>` 双向链表的双端操作和内部可变性。
#[cfg_attr(test, test)]
pub fn run_all() {
    println!("=== Rc<RefCell<_>> 双向链表 ===");

    let mut list = List::new();
    list.push_back(2);
    list.push_front(1);
    list.push_back(3);
    println!("{:?}", list);
    println!("push 后 front: {:?}", list.peek_front().as_deref());
    println!("push 后 back:  {:?}", list.peek_back().as_deref());

    if let Some(mut front) = list.peek_front_mut() {
        *front = 10;
    }
    if let Some(mut back) = list.peek_back_mut() {
        *back = 30;
    }
    println!("修改后 front: {:?}", list.peek_front().as_deref());
    println!("修改后 back:  {:?}", list.peek_back().as_deref());

    println!("pop_front: {:?}", list.pop_front());
    println!("pop_back:  {:?}", list.pop_back());
    println!("剩余元素:   {:?}", list.pop_front());

    println!("=== DoubleEndedIterator ===");
    let list = List::<i32>::new();
    let mut iter = list.into_iter();
    println!("空迭代器 next:      {:?}", iter.next());
    println!("空迭代器 next_back: {:?}", iter.next_back());

    let mut list = List::new();
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);
    let mut iter = list.into_iter();
    println!("next:      {:?}", iter.next());
    println!("next_back: {:?}", iter.next_back());
    println!("next:      {:?}", iter.next());
    println!("next_back: {:?}", iter.next_back());
}
