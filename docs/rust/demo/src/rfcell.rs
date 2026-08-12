use std::cell::{Cell, RefCell};
use std::rc::Rc;

#[cfg_attr(test, test)]
fn test_cell() {
    let x = Cell::new(5);
    println!("x = {}", x.get()); // x = 5

    // 即使 x 是不可变绑定，也可以修改内部
    x.set(10);
    println!("x = {}", x.get()); // x = 10

    let z = &x;

    let y = &x; // 不可变引用
    y.set(20); // 依然可以修改！

    println!("z = {}", z.get());
    z.set(30);
    println!("y = {}", y.get());
}

// 1. 通过这个例子，说明一下如果在运行时出现了两个 mut ，那么就会 panic
#[cfg_attr(test, test)]
fn test_refcell() {
    let s = RefCell::new(String::from("hello"));
    println!("s = {}", s.borrow());

    // 修改内容
    s.borrow_mut().push_str(" world");
    println!("s = {}", s.borrow());

    // 运行时借用冲突示例（会 panic）：
    let r1 = s.borrow(); // 不可变借用
    let r2 = s.borrow(); // 不可变借用
    println!("{}, {}", r1, r2);

    // let r3 = s.borrow_mut(); // 可变借用 —— 在运行时 panic！
    // println!("{}", r3);
}

// 2. 但是如果运行时没有两个 mut ，那么就可以正常运行
#[cfg_attr(test, test)]
fn test_refcell2() {
    let rc = RefCell::new(vec![1, 2, 3]);

    {
        let mut writer = rc.borrow_mut();
        writer.push(4);
    } // writer 被释放

    {
        let reader = rc.borrow();
        println!("{:?}", *reader); // [1, 2, 3, 4]
    }
}

// 3. 通过 RefCell，我们把“借用检查”从编译时推迟到了运行时。
// 而 Rc 允许多个所有者共享同一个 RefCell
// 于是我们实现了同时存在多个 mut 的情况
#[cfg_attr(test, test)]
fn test_refcell3() {
    let shared = Rc::new(RefCell::new(0));
    let a = shared.clone();
    let b = shared.clone();

    *a.borrow_mut() += 10;
    *b.borrow_mut() += 5;

    println!("shared = {}", shared.borrow()); // shared = 15
}

pub fn run_all() {
    test_cell();
    test_refcell();
    test_refcell2();
    test_refcell3();
}
