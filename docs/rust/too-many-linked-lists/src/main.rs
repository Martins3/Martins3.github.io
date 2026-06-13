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

#[allow(dead_code)]
fn second() {
    let mut x = second::List::new();
    x.push(12);
    x.push(12);
    x.push(12);
    x.push(12);
    x.push(12);
}

struct Name<'a> {
    x: &'a str,
    y: &'a str,
}

impl<'a> Name<'a> {
    fn longest(&self, y: &str) -> &str {
        if self.x.len() > y.len() {
            self.x
        } else {
            self.y
        }
    }
}
struct TextEditor {
    text: String,
}

impl TextEditor {
    //Other methods omitted ...

    pub fn get_text<'a>(&'a self) -> &'a String {
        return &self.text;
    }
}

fn main() {
    let a = Name {
        x: (&"a"),
        y: (&"b"),
    };

    let b = a.longest(&"fasdf");
    println!("{}", b);

    let a = TextEditor {
        text: "fasdfa".to_string(),
    };

    println!("{}", 0x20000000);
    println!("{}", 1 << 29);
}
