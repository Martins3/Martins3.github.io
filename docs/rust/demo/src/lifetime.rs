struct Foo<'a> {
    bar: String,
    baz: &'a str,
}
#[cfg_attr(test, test)]
fn test_lifetime() {
    let s = String::from("hello");

    let f = Foo {
        bar: "x".to_string(),
        baz: &s,
    };
    println!("bar = {}, baz = {}", f.bar, f.baz);
}

// ============================================================
// 没有 lifetime 标注，下面这些场景编译器根本无法处理：
//
// 1. 返回值可能是多个输入参数之一（longest）
// 2. 结构体字段里存引用
// 3. 表达"引用必须比它指向的数据活得更短"，杜绝悬垂引用
// ============================================================

// ---- 场景 1：返回值可能是 x 或 y 之一 ----
// 下面的代码无法编译：
//
//   fn longest(x: &str, y: &str) -> &str {
//       if x.len() > y.len() { x } else { y }
//   }
//
// 错误：missing lifetime specifier
// 原因：省略规则（elision）只覆盖"只有一个输入引用"或
//       "返回值来自 &self"等简单情形。这里返回值既可能借用 x
//       也可能借用 y，编译器不知道返回的引用到底和谁绑定，
//       也就无法为它生成任何安全保证。

// 加上 lifetime 后，'a 同时约束 x、y 和返回值：
// 返回值的生命周期取 x 和 y 中较短的那一个
fn longest<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() {
        x
    } else {
        y
    }
}

#[cfg_attr(test, test)]
fn test_longest() {
    let string1 = String::from("abcd");
    let string2 = "xyz";

    let result = longest(string1.as_str(), string2);
    println!("The longest string is {}", result);

    // 下面这段会编译失败，正是 lifetime 在起作用：
    // 返回值被声明为 &'a str，'a 必须同时覆盖 x 和 y。
    // string2 在块内就被销毁，result 在块外使用时可能
    // 指向已释放的内存，编译器直接拒绝。
    //
    // let result;
    // {
    //     let string2 = String::from("xyz");
    //     result = longest(string1.as_str(), string2.as_str());
    // }
    // println!("The longest string is {}", result);
    // 错误：`string2` does not live long enough
}

// ---- 场景 2：结构体字段存引用 ----
// 下面的代码无法编译：
//
//   struct RefHolder {
//       s: &str,
//   }
//
// 错误：missing lifetime specifier
// 原因：结构体实例可能被到处传递，编译器必须知道字段里的
//       引用何时失效，否则无法保证不出现悬垂引用。
//       必须用泛型 lifetime 参数把结构体和引用绑定在一起。

struct RefHolder<'a> {
    s: &'a str,
}

impl<'a> RefHolder<'a> {
    // 只有一个 &self 输入，省略规则够用，可以不用显式写
    fn get(&self) -> &str {
        self.s
    }
}

#[cfg_attr(test, test)]
fn test_ref_holder() {
    let s = String::from("hello");
    let holder = RefHolder { s: &s };
    println!("holder.s = {}", holder.get());
}

// ---- 场景 3：悬垂引用在编译期就不可能写出来 ----
// 没有 lifetime 概念，语言就无法表达"引用必须比数据活得短"，
// 也就无法在编译期消灭悬垂指针这一类内存安全 bug。

#[cfg_attr(test, test)]
fn test_dangling() {
    // fn dangling() -> &str {
    //     let s = String::from("hello");
    //     &s
    // }
    // 错误：returns a reference to data owned by the current function
    // 在 C/C++ 里这是最经典的悬垂指针 bug 来源，Rust 直接拒绝编译。
    println!("返回局部变量引用的代码在编译期就被拒绝");
}

pub fn run_all() {
    test_lifetime();
    test_longest();
    test_ref_holder();
    test_dangling();
}
