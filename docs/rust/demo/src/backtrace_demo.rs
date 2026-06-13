use std::backtrace::Backtrace;

fn inner_function() {
    println!("捕获当前调用栈:");
    let bt = Backtrace::capture();
    println!("{}", bt);
}

fn middle_function() {
    inner_function();
}

fn outer_function() {
    middle_function();
}

/// 演示 Backtrace::capture() 的基本用法
pub fn basic_backtrace() {
    println!("=== 基本 Backtrace 演示 ===");
    outer_function();
}

/// 演示如何检查 backtrace 状态
pub fn backtrace_status() {
    println!("\n=== Backtrace 状态检查 ===");

    let bt = Backtrace::capture();

    // 检查这个 backtrace 实例的状态
    // BacktraceStatus 可以是 Captured, Disabled, 或 Unsupported
    println!("Backtrace 捕获状态: {:?}", bt.status());
}

/// 演示 Backtrace::force_capture() - 强制捕获，忽略环境变量设置
pub fn force_capture_demo() {
    println!("\n=== 强制捕获演示 ===");

    fn level_3() {
        let bt = Backtrace::force_capture();
        println!("强制捕获的 backtrace 状态: {:?}", bt.status());
        println!("强制捕获的调用栈 (前10行):");
        // 打印完整的 backtrace
        let bt_string = format!("{}", bt);
        for line in bt_string.lines().take(10) {
            println!("{}", line);
        }
    }

    fn level_2() {
        level_3();
    }

    fn level_1() {
        level_2();
    }

    level_1();
}

/// 演示 Backtrace 的 Display 特性
pub fn display_demo() {
    println!("\n=== Backtrace 显示演示 ===");

    let bt = Backtrace::capture();

    // Backtrace 实现了 Display，可以直接打印
    println!("Backtrace 内容:");
    println!("{}", bt);
}

pub fn run_all() {
    basic_backtrace();
    backtrace_status();
    force_capture_demo();
    display_demo();

    println!("\n=== 提示 ===");
    println!("要查看 backtrace，可以设置环境变量:");
    println!("  RUST_BACKTRACE=1 cargo run -- 12     (显示精简 backtrace)");
    println!("  RUST_BACKTRACE=full cargo run -- 12  (显示完整 backtrace)");
}
