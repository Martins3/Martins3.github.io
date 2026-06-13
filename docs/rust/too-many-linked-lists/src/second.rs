use std::mem;

use std::rc::Rc;

pub struct List {
    head: Link,
}

// yay type aliases!
type Link = Option<Box<Node>>;

struct Node {
    elem: i32,
    next: Link,
}

impl List {
    pub fn new() -> Self {
        List { head: None }
    }

    pub fn push(&mut self, elem: i32) {
        let new_node = Box::new(Node {
            elem,
            next: mem::replace(&mut self.head, None),
        });

        // 没有这一行会导致 Rust 会释放 Node
        // 否则就是 move 到 self.head 中间了
        self.head = Some(new_node);
    }

    // pub fn pop(&mut self) -> Option<i32> {
        // match mem::replace(&mut self.head, None) {
            // None => None,
            // Some(node) => {
                // self.head = node.next;
                // Some(node.elem)
            // }
        // }
    // }

    // replace 使用情况的简化，加上 map option 的使用
    // 原来 map 是这么使用的，秒啊!
    pub fn pop(&mut self) -> Option<i32> {
        self.head.take().map(|node| {
            self.head = node.next;
            node.elem
        })
    }
}


impl Drop for List {
    fn drop(&mut self) {
        // self.head 持有的 Box 的 move 到f cur_link 中间
        //
        // 为什么不能使用赋值语句 ?
        // 因为 move 之后就不可以使用了

        let mut cur_link = mem::replace(&mut self.head, None);
        while let Some(mut boxed_node) = cur_link {
            cur_link = mem::replace(&mut boxed_node.next, None);
        }
    }
}

#[cfg(test)]
mod test {
    use super::List;
    #[test]
    fn fuck() {
        let mut x = List::new();
        x.push(12);
        x.push(12);
        x.push(12);
        x.push(12);
        x.push(12);
    }
}
