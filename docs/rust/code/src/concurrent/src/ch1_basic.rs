use std::thread;

fn f() {
    println!("Hello from another thread!");

    let id = thread::current().id();
    println!("This is my thread id: {id:?}");
}

fn test1() {
    let t1 = thread::spawn(f);
    let t2 = thread::spawn(f);

    println!("Hello from the main thread.");
    t1.join().unwrap();
    t2.join().unwrap();
}

fn test2() {
    let numbers = vec![1, 2, 3];

    thread::spawn(move || {
        for n in &numbers {
            println!("{n}");
        }
    })
    .join()
    .unwrap();
}

fn test3() {
    let numbers = vec![1, 2, 3];

    thread::scope(|s| {
        s.spawn(|| {
            println!("length: {}", numbers.len());
        });
        s.spawn(|| {
            for n in &numbers {
                println!("{n}");
            }
        });
    });
}

fn test4() {
    let x: &'static [i32; 3] = Box::leak(Box::new([1, 2, 3]));

    thread::spawn(move || dbg!(x));
    thread::spawn(move || dbg!(x));
}


