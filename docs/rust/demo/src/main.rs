use std::cell::Cell;
use std::cell::RefCell;
use std::collections::HashMap;
use std::env;
use std::io::{BufRead, BufReader};
use std::os::unix::net::{UnixListener, UnixStream};
use std::rc::Rc;
use std::thread;

mod backtrace_demo;
mod future_demo;
mod oop;
mod unit_test;

fn handle_client(stream: UnixStream) {
    let stream = BufReader::new(stream);
    for line in stream.lines() {
        println!("{}", line.unwrap());
    }
}

// 一个小小的 uds server
fn uds_server() {
    let listener = UnixListener::bind("/tmp/rust-uds.sock").unwrap();

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                thread::spawn(|| handle_client(stream));
            }
            Err(err) => {
                println!("Error: {}", err);
                break;
            }
        }
    }
}

fn unwrap() {
    let k = 10;
    assert_eq!(Some(4).unwrap_or_else(|| 2 * k), 4);
    assert_eq!(None.unwrap_or_else(|| 2 * k), 20);

    // None.unwrap_or_else(...)
    // None 匹配到 unwrap_or_else 的 None 分支
    // 调用闭包：|| 2 * k
}

fn test_closure() {
    // 1. 只读捕获 x
    let x = 5;
    let closure_fn = || println!("{}", x);
    fn call_fn<F: Fn()>(f: F) {
        f();
    }

    // 2. 修改捕获的变量
    let mut count = 0;
    let closure_mut = || {
        count += 1;
        println!("{}", count);
    };
    fn call_fn_mut<F: FnMut()>(mut f: F) {
        f();
    }

    // 3. closure_move 将会 move 掉这个函数的
    let s = String::from("hello");
    let closure_move = move || println!("{}", s);
    fn call_fn_once<F: FnOnce()>(f: F) {
        f();
    }
    // 实现 Fn ⇒ 自动实现 FnMut 和 FnOnce
    // 实现 FnMut ⇒ 自动实现 FnOnce
    //
    // Fn：用于只读回调，如 GUI 事件处理（不修改状态）。
    // FnMut：用于带状态的迭代器适配器（如 fold、scan）。
    // FnOnce：用于一次性初始化、线程启动（std::thread::spawn 要求 FnOnce + Send）。

    call_fn(closure_fn);
    // call_fn(closure_mut);
    // call_fn(closure_move);
    call_fn_mut(closure_fn);
    call_fn_mut(closure_mut);
    // call_fn_mut(closure_move);

    call_fn_once(closure_fn);
    // call_fn_once(closure_mut);
    call_fn_once(closure_move);
}

macro_rules! say_hello {
    () => {
        println!("Hello!");
    };
}

macro_rules! repeat {
    ($val:expr; $n:expr) => {
        for _ in 0..$n {
            println!("{}", $val);
        }
    };
}

fn test_macro() {
    say_hello!();
    repeat!("Hi"; 3);
}

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

// 1. 通过这个例子，说明一下如果在运行时出现了两个 mut ，那么就会 pani
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
fn test_refcell3() {
    let shared = Rc::new(RefCell::new(0));
    let a = shared.clone();
    let b = shared.clone();

    *a.borrow_mut() += 10;
    *b.borrow_mut() += 5;

    println!("shared = {}", shared.borrow()); // shared = 15
}

// Rust 的默认所有权模型是单一所有
fn test_rc1() {
    let a = String::from("hello");
    let b = a; // 所有权移动
               // println!("{}", a); // 编译错误
}

#[derive(Debug)]
struct User {
    name: String,
}

#[derive(Debug)]
struct Group {
    leader: Rc<User>,
    member: Rc<User>,
}

fn test_rc2() {
    let user = Rc::new(User {
        name: "Alice".to_string(),
    });

    let group = Group {
        leader: Rc::clone(&user),
        member: Rc::clone(&user),
    };

    println!("{:?}", group.leader);
    println!("{:?}", group.member);

    println!("strong count = {}", Rc::strong_count(&user));
}

// 这里是一个经典例子，如果两个 node 指向一个节点，那么就必须使用引用计数
#[derive(Debug)]
struct Node {
    value: i32,
    next: Option<Rc<Node>>,
}
fn test_rc3() {
    use std::rc::Rc;

    let tail = Rc::new(Node {
        value: 3,
        next: None,
    });

    let n1 = Rc::new(Node {
        value: 1,
        next: Some(Rc::clone(&tail)),
    });

    let n2 = Rc::new(Node {
        value: 2,
        next: Some(Rc::clone(&tail)),
    });

    println!("tail count = {}", Rc::strong_count(&tail));
}

fn test_while_let1() {
    // Make `optional` of type `Option<i32>`
    let mut optional = Some(0);

    // Repeatedly try this test.
    loop {
        match optional {
            // If `optional` destructures, evaluate the block.
            Some(i) => {
                if i > 9 {
                    println!("Greater than 9, quit!");
                    optional = None;
                } else {
                    println!("`i` is `{:?}`. Try again.", i);
                    optional = Some(i + 1);
                }
                // ^ Requires 3 indentations!
            }
            // Quit the loop when the destructure fails:
            _ => {
                break;
            } // ^ Why should this be required? There must be a better way!
        }
    }
}

fn test_while_let2() {
    // Make `optional` of type `Option<i32>`
    let mut optional = Some(0);

    // This reads: "while `let` destructures `optional` into
    // `Some(i)`, evaluate the block (`{}`). Else `break`.
    //
    //  在 while let 下，右侧是变量，左侧是模式
    while let Some(i) = optional {
        if i > 9 {
            println!("Greater than 9, quit!");
            optional = None;
        } else {
            println!("`i` is `{:?}`. Try again.", i);
            optional = Some(i + 1);
        }
        // ^ Less rightward drift and doesn't require
        // explicitly handling the failing case.
    }
    // ^ `if let` had additional optional `else`/`else if`
    // clauses. `while let` does not have these.
}

#[derive(Debug)]
struct Me {
    name: i32,
    age: i32,
}

fn test_container() {
    let mut contacts = HashMap::new();
    contacts.insert(1, Me { age: 1, name: 2 });

    // for (contact, &number) in contacts.iter() {
    // number 后面添加 & 意味着 number 将会出现borrow，需要显示地 copy Trait
    // move occurs because `number` has type `Me`, which does not implement the `Copy` trait
    // }

    for (contact, number) in contacts.iter() {
        println!("{:?} ", contact);
        println!("{:?} ", number);
    }
    // 2. 什么使用 &  &mut Copy 和 borrow ，是不是都可以实现 ?
}

fn test_unit_test_demo() {
    // 假装调用一下
    let result = unit_test::add(5, 3);
    println!("Example: add(5, 3) = {}", result);

    let calc = unit_test::Calculator::new();
    println!("Created calculator with initial result: {}", calc.result);
}

struct Foo<'a> {
    bar: String,
    baz: &'a str,
}

fn test_lifetime() {
    let s = String::from("hello");

    let f = Foo {
        bar: "x".to_string(),
        baz: &s,
    };
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 2 {
        println!("Usage: {} <option>", args[0]);
        println!("Options:");
        println!("  1 - Test UDS server");
        println!("  2 - unwrap");
        println!("  3 - Test function variance");
        println!("  4 - Test macros");
        println!("  5 - Test Cell and RefCell");
        println!("  6 - Test Rc");
        println!("  7 - Test OOP example");
        println!("  8 - Test while let examples");
        println!("  9 - Test container examples");
        println!("  10 - Unit test demo (shows how to write unit tests in Rust)");
        println!("  11 - Lifetime example");
        println!("  12 - Backtrace demo (std::backtrace)");
        println!("  13 - Future demo (std::future::Future)");
        return;
    }

    match args[1].as_str() {
        "1" => {
            uds_server();
        }
        "2" => {
            unwrap();
        }
        "3" => {
            test_closure();
        }
        "4" => {
            test_macro();
        }
        "5" => {
            test_cell();
            test_refcell();
            test_refcell2();
            test_refcell3();
        }
        "6" => {
            test_rc1();
            test_rc2();
            test_rc3();
        }
        "7" => {
            oop::oop();
        }
        "8" => {
            // 参考 https://doc.rust-lang.org/beta/rust-by-example/flow_control/while_let.html
            // 中的例子
            test_while_let1();
            test_while_let2();
        }
        "9" => {
            test_container();
        }
        "10" => {
            test_unit_test_demo();
        }
        "11" => {
            test_lifetime();
        }
        "12" => {
            // 演示 std::backtrace 的使用
            // 参考: https://doc.rust-lang.org/std/backtrace/index.html
            backtrace_demo::run_all();
        }
        "13" => {
            // 演示 std::future::Future、Poll、Waker 和 async/await 的关系
            future_demo::run_all();
        }
        _ => {
            println!("Invalid option: {}", args[1]);
        }
    }
}
