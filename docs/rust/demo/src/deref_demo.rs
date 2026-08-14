use std::cell::{Ref, RefCell};

/// 演示 as_deref 的适用场景：它是 Option/Result 的方法，不是 Ref 的方法。
/// 配合调试器单步看每个变量的类型和值。
#[cfg_attr(test, test)]
pub fn run_all() {
    println!("=== 1. Ref 本身没有 as_deref ===");
    let data = RefCell::new(String::from("hello"));
    let r: Ref<'_, String> = data.borrow();
    // r.as_deref();  // 编译错误：Ref<'_, String> 没有这个方法
    println!("直接用 Deref 读内容: {}", *r);
    drop(r);

    println!("\n=== 2. Option 包着 Ref 时，as_deref 是 Option 的方法 ===");
    let o: Option<Ref<'_, String>> = Some(data.borrow());
    println!("as_deref 后: {:?}", o); // 两个打印是一样的
    let d: Option<&String> = o.as_deref(); // Option<Ref<String>> -> Option<&String>
    println!("as_deref 后: {:?}", d);
    drop(o);

    println!("\n=== 3. 经典场景：Option<String> -> Option<&str> ===");
    let name: Option<String> = Some(String::from("alice"));

    // as_ref:  Option<String> -> Option<&String>
    let by_ref: Option<&String> = name.as_ref();
    println!("as_ref:    {:?}", by_ref);

    // as_deref: Option<String> -> Option<&str>（多走一层 Deref）
    let by_deref: Option<&str> = name.as_deref();
    println!("as_deref:  {:?}", by_deref);

    // name 没被 move，还能继续用
    println!("name 还在: {:?}", name);

    println!("\n=== 4. 为什么传参常用 as_deref ===");
    fn greet(n: Option<&str>) {
        match n {
            Some(s) => println!("hello, {s}"),
            None => println!("hello, stranger"),
        }
    }
    greet(name.as_deref()); // 不用 move name，也不用写 name.as_ref().map(|s| s.as_str())
    greet(None);
}
