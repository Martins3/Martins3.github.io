use std::mem;

pub struct List<T> {
    head: Link<T>,
}

// yay type aliases!
type Link<T> = Option<Box<Node<T>>>;

struct Node<T> {
    elem: T,
    next: Link<T>,
}

impl<T> List<T> {
    pub fn new() -> Self {
        List { head: None }
    }

    pub fn push(&mut self, elem: T) {
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
    pub fn pop(&mut self) -> Option<T> {
        self.head.take().map(|node| {
            self.head = node.next;
            node.elem
        })
    }

    pub fn peek(&self) -> Option<&T> {
        // 通过 as_ref 将 Option<T> 装换为 Option<&T>
        self.head.as_ref().map(|node| &node.elem)
    }

    pub fn peek_mut(&mut self) -> Option<&mut T> {
        self.head.as_mut().map(|node| &mut node.elem)
    }
}

impl<T> Drop for List<T> {
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

pub struct IntoIter<T>(List<T>);

impl<T> List<T> {
    pub fn into_iter(self) -> IntoIter<T> {
        println!("into inter");
        IntoIter(self)
    }

    pub fn raw_iter(self) -> List<T> {
        self
    }
}

impl<T> Iterator for IntoIter<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> {
        // access fields of a tuple struct numerically
        self.0.pop()
    }
}

impl<T> Iterator for List<T> {
    type Item = T;
    fn next(&mut self) -> Option<Self::Item> {
        // access fields of a tuple struct numerically
        self.pop()
    }
}

pub struct Iter<'a, T> {
    next: Option<&'a Node<T>>,
}

impl<T> List<T> {
    pub fn iter(&self) -> Iter<T> {
        Iter {
            // 1. self.head.map(|node| &node) : 因为 head 是
            //    Option<Box<Node<T>>>，所以将其中的 &Box<Node<T>> 了
            // 2. next: self.head.map(|node| &*node) : box 可以直接来 * 和 &,
            //    那么就可以直接获取到
            //
            //    从 Option<Box<Node<T>>> 到 Option<&Node<T>> 的转换
            next: self.head.as_deref(),
        }
    }
}

impl<'a, T> Iterator for Iter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item> {
        self.next.map(|node| {
            // self.next = node.next.as_deref();
            // 或者使用这种写法
            // Option<Box<Node<T>>> ==> Option<&Box<Node<T>>>
            self.next = node.next.as_ref().map::<&Node<T>, _>(|node| &node);
            &node.elem
        })
    }
}

// 使用常量 iter 可以遍历对象，对于 option 是拷贝，next 得到的并不能进行写
// 当是非常量的 iter 的时候，进行遍历的时候，list 就会变短啊
pub struct IterMut<'a, T> {
    next: Option<&'a mut Node<T>>,
}

impl<T> List<T> {
    pub fn iter_mut(&mut self) -> IterMut<'_, T> {
        IterMut { next: self.head.as_deref_mut() }
    }
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
    fn peek() {
        let mut list = List::new();
        assert_eq!(list.peek(), None);
        assert_eq!(list.peek_mut(), None);
        list.push(1);
        list.push(2);
        list.push(3);

        assert_eq!(list.peek(), Some(&3));
        assert_eq!(list.peek_mut(), Some(&mut 3));

        list.peek_mut().map(|value| *value = 42);

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

        let mut list = List::new();
        list.push(1);
        list.push(2);
        let mut x = list.raw_iter();
        assert_eq!(x.next(), Some(2));
        assert_eq!(x.next(), Some(1));
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
}
