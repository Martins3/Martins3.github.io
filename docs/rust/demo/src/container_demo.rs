use std::collections::HashMap;
use std::collections::{BTreeSet, HashSet};

#[derive(Debug)]
struct Student {
    name: i32,
    age: i32,
}

#[cfg_attr(test, test)]
fn test_hashmap() {
    let mut contacts = HashMap::new();
    contacts.insert(1, Student { age: 1, name: 2 });
    contacts.insert(2, Student { age: 1, name: 3 });

    // number 后面添加 & 意味着 number 将会出现borrow，需要显示地 copy Trait
    // for (contact, &number) in contacts.iter() {
    //     println!("{:?} {:?}", contact, number);
    // }

    for (contact, number) in contacts.iter() {
        println!("{:?} {:?}", contact, number);
    }

    for (_, number) in contacts.iter_mut() {
        number.age = 12;
        number.age = 1234;
    }

    for (contact, number) in contacts.iter() {
        println!("{:?} {:?}", contact, number);
    }
    // 2. 什么使用 &  &mut Copy 和 borrow ，是不是都可以实现 ?
}

#[cfg_attr(test, test)]
fn test_vector() {
    let mut contacts = vec![Student { age: 3, name: 2 }];
    contacts.push(Student { age: 1, name: 200 });
    contacts.push(Student { age: 2, name: 100 });
    contacts.push(Student { age: 2, name: 300 });

    for number in contacts.iter() {
        println!("{:?}", number);
    }

    contacts.sort_by_key(|s| s.age + s.name);
    for number in contacts.iter() {
        println!("{:?}", number);
    }

    for number in contacts.iter_mut() {
        number.name = 0;
    }

    for number in contacts.iter() {
        println!("{:?}", number);
    }
}

#[cfg_attr(test, test)]
fn test_set() {
    // HashSet: 基于哈希表, 元素无序, 要求 T: Hash + Eq
    let mut contacts = HashSet::new();
    contacts.insert(1);
    contacts.insert(2);
    contacts.insert(2); // 重复插入被忽略
    contacts.insert(3);
    println!(
        "len = {}, contains 2 = {}",
        contacts.len(),
        contacts.contains(&2)
    );

    contacts.remove(&3);
    println!("after remove 3: {:?}", contacts);

    // 集合运算: 并集 / 交集 / 差集 / 对称差
    let a: HashSet<i32> = [1, 2, 3].into_iter().collect();
    let b: HashSet<i32> = [3, 4, 5].into_iter().collect();
    println!("union:        {:?}", a.union(&b).collect::<Vec<_>>());
    println!("intersection: {:?}", a.intersection(&b).collect::<Vec<_>>());
    println!("difference:   {:?}", a.difference(&b).collect::<Vec<_>>());
    println!(
        "sym diff:     {:?}",
        a.symmetric_difference(&b).collect::<Vec<_>>()
    );

    // BTreeSet: 基于平衡树, 迭代时按键升序, 要求 T: Ord
    let mut tree = BTreeSet::new();
    tree.insert(3);
    tree.insert(1);
    tree.insert(2);
    println!("BTreeSet 升序迭代: {:?}", tree.iter().collect::<Vec<_>>());
}

pub fn run_all() {
    test_hashmap();
    test_vector();
    test_set();
}
