use std::mem;

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

        // 如果没有这一行，那么 Rust 会立刻消失
        // self.head = Some(new_node);
    }

    pub fn pop(&mut self) -> Option<i32> {
        match mem::replace(&mut self.head, None) {
            None => None,
            Some(node) => {
                self.head = node.next;
                Some(node.elem)
            }
        }
    }
}

impl Drop for List {
    fn drop(&mut self) {
        let mut cur_link = mem::replace(&mut self.head, None);
        while let Some(mut boxed_node) = cur_link {
            cur_link = mem::replace(&mut boxed_node.next, None);
        }
    }
}

impl Drop for Node{
    fn drop(&mut self) {
        println!("fuck")
    }
}

#[cfg(test)]
mod test{
    use super::List;
    #[test]
    fn fuck(){
        let mut x = List::new();
        x.push(12);
        x.push(12);
        x.push(12);
        x.push(12);
        x.push(12);
    }
}

