use std::borrow::Borrow;
use std::rc::Rc;

pub struct List<T> {
    head: Link<T>,
}

type Link<T> = Option<Rc<Node<T>>>;

struct Node<T> {
    elem: T,
    next: Link<T>,
}

impl<T> Borrow<T> for Node<T> {
    fn borrow(&self) -> &T {
        &self.elem
    }
}

impl<T> List<T> {
    pub fn new() -> Self {
        List { head: None }
    }

    pub fn prepend(&self, elem: T) -> List<T> {
        List {
            head: Some(Rc::new(Node {
                elem,
                // 实现 Rc 的关键
                next: self.head.clone(),
            })),
        }
    }

    // 获取到下一个 node 才有的 thread
    pub fn tail(&self) -> List<T> {
        List {
            head: self.head.as_ref().and_then(|node| node.next.clone()),
        }
    }

    // 演示使用 borrow 的方法
    pub fn head2(&self) -> Option<&T> {
        self.head.as_deref().map(Borrow::borrow)
    }

    pub fn head(&self) -> Option<&T> {
        self.head.as_ref().map(|node| &node.elem)
    }

    /// 返回头节点的强引用数；空链表没有头节点，因此返回 0。
    pub fn strong_count(&self) -> usize {
        self.head.as_ref().map_or(0, Rc::strong_count)
    }

    pub fn iter(&self) -> Iter<'_, T> {
        Iter {
            next: self.head.as_deref(),
        }
    }
}

impl<T> Drop for List<T> {
    fn drop(&mut self) {
        let mut head = self.head.take();
        while let Some(node) = head {
            if let Ok(mut node) = Rc::try_unwrap(node) {
                head = node.next.take();
            } else {
                break;
            }
        }
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

/// 演示持久化链表、共享尾部以及 `Rc` 强引用计数。
#[cfg_attr(test, test)]
pub fn run_all() {
    println!("=== & 与 Borrow::borrow() ===");

    let node = Node {
        elem: 100,
        next: None,
    };
    let node_ref: &Node<i32> = &node;
    let elem_ref: &i32 = node.borrow();
    println!("&node 得到 &Node，节点元素为: {}", node_ref.elem);
    println!("node.borrow() 根据 Borrow<T> 得到 &T: {elem_ref}");

    println!("=== Rc 持久化链表 ===");

    let empty = List::new();
    let one = empty.prepend(1);
    let two = one.prepend(2);
    let three = two.prepend(3);

    println!("empty: {:?}", empty.iter().collect::<Vec<_>>());
    println!("one:   {:?}", one.iter().collect::<Vec<_>>());
    println!("two:   {:?}", two.iter().collect::<Vec<_>>());
    println!("three: {:?}", three.iter().collect::<Vec<_>>());
    println!("three 的头元素: {:?}", three.head());
    println!("three 的头元素: {:?}", three.head2());

    println!("=== tail() 共享已有节点 ===");

    let tail = three.tail();
    let tail2 = tail.tail();
    let tail3 = tail2.tail();
    println!("three: {:?}", three.iter().collect::<Vec<_>>());
    println!("tail:  {:?}", tail.iter().collect::<Vec<_>>());
    println!("tail2:  {:?}", tail2.iter().collect::<Vec<_>>());
    println!("tail3:  {:?}", tail3.iter().collect::<Vec<_>>());
    println!("节点 2 的强引用数: {}", two.strong_count());
    println!("tail 的强引用数: {}", tail.strong_count());
    println!("tail2 的强引用数: {}", tail2.strong_count());
    println!("tail3 的强引用数: {}", tail3.strong_count());

    println!("=== 两个分支共享同一尾部 ===");

    let shared = List::new().prepend(10).prepend(20);
    println!("创建分支前，节点 20 的强引用数: {}", shared.strong_count());

    let left = shared.prepend(30);
    let right = shared.prepend(40);
    println!("left:   {:?}", left.iter().collect::<Vec<_>>());
    println!("right:  {:?}", right.iter().collect::<Vec<_>>());
    println!("shared: {:?}", shared.iter().collect::<Vec<_>>());
    println!(
        "创建两个分支后，节点 20 的强引用数: {}",
        shared.strong_count()
    );

    drop(left);
    println!(
        "释放 left 后，节点 20 的强引用数: {}",
        shared.strong_count()
    );

    drop(right);
    println!(
        "释放 right 后，节点 20 的强引用数: {}",
        shared.strong_count()
    );
}
