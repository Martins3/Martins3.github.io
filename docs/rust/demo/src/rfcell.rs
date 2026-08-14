use std::cell::{Cell, Ref, RefCell, RefMut};
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

/// 演示 Cell "整个换掉" 的语义：get 是复制，set/replace 是整体替换，永远拿不到内部引用。
#[cfg_attr(test, test)]
pub fn test_cell2() {
    println!("=== Cell: get 拿到的是副本 ===");
    let c = Cell::new(5);
    let a = c.get(); // a 是从 Cell 内部【复制】出来的 5，和内部再无关联
    c.set(99); // 整个换掉内部的值
    println!("a = {a}, c = {}", c.get()); // a 还是 5，不受影响

    println!("\n=== Cell 装 String（非 Copy）：get() 根本不存在 ===");
    let s = Cell::new(String::from("hello"));
    // s.get();              // 编译错误：String 不是 Copy trait，无法复制出来
    let old = s.replace(String::from("world")); // replace：旧的拿出来，新的塞进去
    println!("换出来的旧值 = {old}, 现在 = {:?}", s.take()); // take：把里面的整个 move 走，留下默认值 ""

    println!("\n=== Cell 做不到的事：就地改一个字段 ===");
    let v = Cell::new(vec![1, 2, 3]);
    // 想 push 一个元素？拿不到 &mut Vec，只能整个换掉：
    let mut tmp = v.take(); // 1. 先把 Vec 整个 move 出来（Cell 里留下空 Vec）
    let ptr_before = tmp.as_ptr(); // 元素堆内存的地址
    tmp.push(4); // 2. 在外面改
    v.set(tmp); // 3. 再整个塞回去
    let back = v.take();
    println!("Cell 里的 Vec = {:?}", back);
    // move 的只是 Vec 的 (ptr, len, cap) 三个字段，堆里的元素一个都没复制：
    println!("元素地址变化了吗？ {:p} -> {:p}", ptr_before, back.as_ptr());

    println!("\n=== RefCell：借引用进去就地改，一步到位 ===");
    let v = RefCell::new(vec![1, 2, 3]);
    v.borrow_mut().push(4); // 借 &mut Vec 进去，直接改
    println!("RefCell 里的 Vec = {:?}", v.borrow());
}

// 1. 通过这个例子，说明一下如果在运行时出现了两个 mut ，那么就会 panic
#[cfg_attr(test, test)]
fn test_refcell2() {
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

// 2. 用 random() 在运行时决定是否同时存在两个 mut 借用：
//    两个 mut 同时存活 -> RefCell 在运行时 panic
//    两个 mut 不重叠 -> 正常运行
#[cfg_attr(test, test)]
fn test_refcell3() {
    let rc = RefCell::new(vec![1, 2, 3]);

    let _writer = if rand::random() {
        // 运气不好: 把 writer 保存到 Some 中，使它继续持有可变借用
        let mut writer = rc.borrow_mut();
        writer.push(4);
        Some(writer)
    } else {
        // 运气好: writer 在块内释放，两个 mut 借用不重叠 -> 正常运行
        {
            let mut writer = rc.borrow_mut();
            writer.push(4);
        } // writer 被释放
        None
    };

    let reader = rc.borrow_mut(); // _writer 为 Some 时，两个 mut 同时存活 -> 运行时 panic!
    println!("{:?}", *reader); // _writer 为 None 时输出 [1, 2, 3, 4]
}

// 3. 通过 RefCell，我们把“借用检查”从编译时推迟到了运行时。
//  而 Rc 允许多个所有者共享同一个 RefCell
//  于是我们实现了同时存在多个 mut 的情况
#[cfg_attr(test, test)]
fn test_refcell_rc() {
    let shared = Rc::new(RefCell::new(0));
    let a = shared.clone();
    let b = shared.clone();

    *a.borrow_mut() += 10;
    *b.borrow_mut() += 5;

    println!("shared = {}", shared.borrow()); // shared = 15
}

// 4. borrow() -> Ref<T>（只读守卫），borrow_mut() -> RefMut<T>（可写守卫）。
//    守卫活着 = 借用被占用；守卫 drop = 借用释放，规则由运行时计数器保证。
#[cfg_attr(test, test)]
fn test_refcell_guards() {
    let data = RefCell::new(String::from("hello"));

    // borrow() 返回 Ref<'_, String>，实现了 Deref，用起来就像 &String
    let r: Ref<'_, String> = data.borrow();
    println!("Ref 读到的内容 = {}, len = {}", *r, r.len());

    // r 活着期间，data 一直占着读锁：此时要可写借用会违规。
    // 用 try_borrow_mut 演示（返回 Err 而不是直接 panic）：
    println!("Ref 活着时 try_borrow_mut 失败？ {}", data.try_borrow_mut().is_err());
    // 读锁可以有多个：
    let r2 = data.borrow();
    println!("再来一个只读借用没问题 = {}", *r2);
    drop(r);
    drop(r2); // 守卫析构，读锁全部释放

    // borrow_mut() 返回 RefMut<'_, String>，实现了 DerefMut，用起来像 &mut String
    let mut w: RefMut<'_, String> = data.borrow_mut();
    w.push_str(" world"); // 就地修改内部
    println!("RefMut 改完 = {}", *w);
    // w 活着期间，连只读借用都借不出来：
    println!("RefMut 活着时 try_borrow 失败？ {}", data.try_borrow().is_err());
    drop(w); // 写锁释放

    println!("最终 = {}", data.borrow());
}

// 5. Ref<'b, T> 的生命周期：守卫不许比它借的 RefCell 活得更久。
//    可以把 Ref<'b, T> 理解成 & 的运行时版本，生命周期规则一模一样。
#[cfg_attr(test, test)]
fn test_refcell_lifetime() {
    // --- 情形 1：Ref 想比 RefCell 活得久 -> 编译错误 ---
    //
    // let r;
    // {
    //     let data = RefCell::new(String::from("hi"));
    //     r = data.borrow();   // 编译错误：data does not live long enough
    // }                        // data 在这里析构
    // println!("{}", *r);      // r 会变成悬垂守卫，编译器直接拦下

    // --- 情形 2：正确的顺序：Ref 先死，RefCell 后死 ---
    let data = RefCell::new(String::from("hello"));
    {
        let r = data.borrow();
        println!("块内借用: {}", *r);
    } // r 在这里 drop，读锁释放
    println!("Ref 死后还能再借: {}", data.borrow());

    // --- 情形 3：函数签名里显式写生命周期 ---
    // 返回值 Ref<'a, String> 里的 'a 和参数 &'a RefCell<String> 是同一个：
    // 意思是"这个守卫借用了传入的 RefCell，调用方必须保证 RefCell 活得比守卫久"。
    fn first_char<'a>(cell: &'a RefCell<String>) -> Ref<'a, str> {
        // Ref::map 把 Ref<'a, String> 收窄成只看第一个字符的 Ref<'a, str>
        Ref::map(cell.borrow(), |s| &s[..1])
    }

    let cell = RefCell::new(String::from("world"));
    let c = first_char(&cell); // c: Ref<'_, str>，借用期依附于 cell
    println!("第一个字符: {}", c);
    // c 活着期间 cell 一直被占着读锁，这正由 'a 表达
    drop(c);

    // --- 情形 4：反过来，把本地 RefCell 的 Ref 返回出去 -> 编译错误 ---
    //
    // fn bad<'a>() -> Ref<'a, String> {
    //     let local = RefCell::new(String::from("x"));
    //     local.borrow()   // 编译错误：local 是局部变量，函数返回时就析构了
    // }                     // Ref 不可能比它的 RefCell 活得久
    //
    // 这也是为什么链表里节点要用 Rc 包着：让节点的"主人"不止一个，
    // Ref 的存活才有意义（rfcell-linked-list.rs 的 peek_front 正是靠 Rc 才能返回 Ref）。
}

pub fn run_all() {
    test_cell();
    test_refcell2();
    test_refcell3();
    test_refcell_rc();
    test_refcell_guards();
    test_refcell_lifetime();
}
