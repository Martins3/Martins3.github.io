type Link = Option<Box<Node>>;

#[derive(Debug)]
struct List {
    head: Link,
}

#[derive(Clone)] // Clone 可以和 Drop 共存，Copy 不行
struct SimpleList {
    head: i32,
}

#[derive(Debug)]
struct Node {
    next: List,
    val: i32,
}

impl Drop for List {
    // 这里实现了 drop ，其中可以访问 self.head ，所以要求
    // List::head 不可以被 move 走
    fn drop(&mut self) {
        println!("drop it {:?}", self.head);
    }
}

impl List {
    fn add(& mut self) {
        // 因此 Rust 选择了一条简单且安全的规则：
        //   没有自定义 Drop：
        //       允许部分 move
        //       编译器分别销毁剩余字段
        //
        //   存在自定义 Drop：
        //       必须先调用 Drop::drop(&mut self)
        //       因而 self 必须始终完整
        //       禁止移出非 Copy 字段
        //
        // 如果这里使用 let head = self.head;
        // 那么 self.head 就转移到了 head 中，如果对于 List 来 drop
        // drop 函数中又访问了 self.head ，就是问题了
        let head = self.head.take();
        println!("{:?}", head);
    }
}

impl Drop for SimpleList {
    fn drop(&mut self) {
        println!("drop it");
    }
}

impl SimpleList {
    fn add(self) {
        let head = self.head;
        println!("{:?}", head);
    }
}

#[cfg_attr(test, test)]
pub fn test_unwrap() {
    let sl = SimpleList { head: 12 };
    let m = sl.clone(); // clone 出第二份独立的值，l 仍然可用
    sl.add(); // l 被 move 进 add，add 结束 drop 一次
    println!("main 继续");
    // main 结束，m 离开作用域，再 drop 一次

    // Node 的字段顺序是 next, val，顺序不影响 struct literal，但语法必须正确
    // Box 没有 Box<Node{...}> 这种字面量写法，必须调用 Box::new(...)
    // Node.next 的类型是 List（不是 Option），所以内层是 List { head: None }
    let mut l = List { head: Some(Box::new(Node { val: 12, next: List { head: None } })) };

    // List 有自定义 Drop，禁止 partial move（不能直接把字段 move 出来），
    // 所以先 take() 把 Option 换成 None，再 unwrap() 取出 Box<Node>
    let node = l.head.take().unwrap();
    println!("node.val = {}", node.val);

    // take() 之后 head 已经变成 None，此时 unwrap() 会 panic；
    // 有自定义 Drop 也不能再 move 字段，改用 as_ref() 借用 + unwrap_or 兜底
    let fallback = Box::new(Node { val: 0, next: List { head: None } });
    let node2 = l.head.as_ref().unwrap_or(&fallback);
    println!("{:?}", node2);
    l.add();
}
