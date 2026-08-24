use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

#[allow(dead_code)]
type Link = Option<Rc<RefCell<Node>>>;

#[derive(Debug)]
struct Node {
    field: i32,
    next: Link,
    prev: Link,
}

#[derive(Debug)]
struct List {
    head: Link,
    tail: Link,
}

// 双向链表 + HashMap 实现 LRU
struct LRUCache {
    capacity: i32,
    map: HashMap<i32, Link>,
    list: List,
    size: i32,
}

/**
 * `&self` means the method takes an immutable reference.
 * If you need a mutable reference, change it to `&mut self` instead.
 */
#[allow(dead_code)]
impl LRUCache {
    fn new(capacity: i32) -> Self {
        LRUCache {
            capacity,
            map: HashMap::new(),
            list: List {
                head: None,
                tail: None,
            },
            size: 0,
        }
    }

    fn get(&self, key: i32) -> i32 {
        // *self.map.get(&key).unwrap_or(&-1)
        match self.map.get(&key) {
            None => -1,
            Some(Some(node)) => return node.borrow().field,
            Some(_) => -1,
        }
    }

    fn put(&mut self, _key: i32, _value: i32) {
        match self.map.get(&_key) {
            // 没有找到
            None => {
                if self.capacity > self.size {
                    let m = Some(Rc::new(RefCell::new(Node {
                        field: _value,
                        next: None,
                        prev: None,
                    })));
                    if self.list.head.is_none() {
                        // 第一个节点：head 和 tail 都指向它
                        self.list.head = m.clone();
                        self.list.tail = m.clone();
                    } else {
                        // 不是第一个，插到头部：
                        // 旧头的 prev 指向 m，m 的 next 指向旧头，head 更新为 m
                        let old_head = self.list.head.take().unwrap();
                        old_head.borrow_mut().prev = m.clone();
                        if let Some(node) = &m {
                            node.borrow_mut().next = Some(old_head);
                        }
                        self.list.head = m.clone();
                    }
                } else {
                    // 满了，删掉最后一个，然后添加第一个
                    // let head = self.list.clone();
                    // 移除 head 的
                }
            }
            Some(m) => {
                // m 是什么啊
            }
        }
    }
}

/**
 * Your LRUCache object will be instantiated and called as such:
 * let obj = LRUCache::new(capacity);
 * let ret_1: i32 = obj.get(key);
 * obj.put(key, value);
 */
#[cfg_attr(test, test)]
pub fn run() {
    let mut obj = LRUCache::new(10);
    let ret_1: i32 = obj.get(1);
    println!("get(1) = {}", ret_1);
    obj.put(1, 1);
}
