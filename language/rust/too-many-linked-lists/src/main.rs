mod first;
mod first2;
mod second;

#[allow(dead_code)]
fn first() {
    let list: first::List<i32> = first::List::Cons(
        1,
        Box::new(first::List::Cons(2, Box::new(first::List::Nil))),
    );
    println!("{:?}", list);
}

#[allow(dead_code)]
fn first2() {
    let mut list: first2::List = first2::List::new();
    list.push(12);
    list.push(2);
    list.push(2);
    list.push(2);
    list.push(2);
    list.print();
}

fn second() {
    let mut x = second::List::new();
    x.push(12);
    x.push(12);
    x.push(12);
    x.push(12);
    x.push(12);
}
fn main() {
    second()
}
