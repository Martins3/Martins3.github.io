- [How to create a Rust struct with string members?](https://stackoverflow.com/questions/25754863/how-to-create-a-rust-struct-with-string-members)
```rust
struct Foo {
    bar: String,
    baz: &'static str,
}

fn main() {
    let foo = Foo {
        bar: "bar".to_string(),
        baz: "baz",
    };
    println!("{}, {}", foo.bar, foo.baz);
}
```
