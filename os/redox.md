# Redox

[官方文档](https://doc.redox-os.org/book/ch01-01-welcome.html)

调试的方法:
https://gitlab.redox-os.org/redox-os/kernel/-/tree/master


## 问题
- [ ] 在 Linux 内核中间，是通过 acpi 进一步的去扫描 pci bus root 的，现在都是直接访问的啦
- [ ] pci 的 bar 就是直接从 pci 中读去出来的，这和 qemu 表现的并不一致。
  - [ ] qemu 中开始的时候根本不知道 bar 的空间，是后来通过截取 CONFIG_ADTA 和 CONFIG_ADDR 来实现读取 bar 从而配置的
- [ ] 这个 MSI 是怎么操作的，并不清楚


## 如何调用的 e1000 driver 的
如何进行这个:
https://gitlab.redox-os.org/redox-os/drivers

```c
cargo rustc --lib --target=/home/maritns3/core/ld/redox/kernel/targets/x86_64-unknown-none.json --release -Z build-std=core,alloc -- -C soft-float -C debuginfo=2 -C lto --emit link=../build/libkernel.a
```

这个代码是放到这里的:
/home/maritns3/core/ld/redox/cookbook/recipes/drivers/source/e1000d/src/main.rs

在 e1000 启动的时候，就会通过参数告知其最重要的三个参数:
1. pci bar 的开始位置
2. pci bar 的大小
3. irq

```rs
fn main() {
    let mut args = env::args().skip(1);

    let mut name = args.next().expect("e1000d: no name provided");
    name.push_str("_e1000");

    let bar_str = args.next().expect("e1000d: no address provided");
    let bar = usize::from_str_radix(&bar_str, 16).expect("e1000d: failed to parse address");

    let bar_size_str = args.next().expect("e1000d: no address size provided");
    let bar_size = usize::from_str_radix(&bar_size_str, 16).expect("e1000d: failed to parse address size");

    let irq_str = args.next().expect("e1000d: no irq provided");
    let irq = irq_str.parse::<u8>().expect("e1000d: failed to parse irq");

    println!("if need pci, call this + E1000 {} on: {:X} size: {:X} IRQ: {}", name, bar, bar_size, irq);
```

## AHCI 到底是个什么玩意儿

## redox/cookbook/recipes/nulld 的工作原理


## acpid 的工作原理
- [ ] 在内核的那一侧并没有找到这个
```rs
    let rxsdt_raw_data: Arc<[u8]> = std::fs::read("kernel/acpi:rxsdt")
        .expect("acpid: failed to read `kernel/acpi:rxsdt`")
        .into();

    let shutdown_pipe = File::open("kernel/acpi:kstop")
        .expect("acpid: failed to open `kernel/acpi:kstop`");
```

- [ ] trace 一下这些信息的来源吧
```c
Main kernel thread exited with status 9
kernel::arch::x86_64::stop:INFO -- Running kstop()
kernel::arch::x86_64::stop:INFO -- Notifying any potential ACPI driver
kernel::arch::x86_64::stop:INFO -- Waiting one second for ACPI driver to run the shutdown sequence.
2021-05-20T03-36-01..452++00:00 [@acpid:242 INFO] Received shutdown request from kernel.
2021-05-20T03-36-01..456++00:00 [@acpid:242 INFO] Received shutdown request from kernel.
2021-05-20T03-36-01..463++00:00 [@acpid::aml:126 INFO] Shutdown SLP_TYPa 0, SLP_TYPb 0
2021-05-20T03-36-01..463++00:00 [@acpid::aml:126 INFO] Shutdown SLP_TYPa 0, SLP_TYPb 0
user:~# 2021-05-20T03-36-01..463++00:00 [@acpid::aml:129 INFO] Shutdown with ACPI outw(0x604, 0x2000)
2021-05-20T03-36-01..463++00:00 [@acpid::aml:129 INFO] Shutdown with ACPI outw(0x604, 0x2000)
```

- [ ] 为什么 acpid 下面还是需要 aml 语言的解析啊

从现在看，acpi 提供了两件事情，提供设备的内存空间布局，支持各种 bios 的请求:

应该是通过 socket 的方式，将消息通知给 acpid:
/home/maritns3/core/ld/redox/cookbook/recipes/drivers/source/acpid/src/aml/mod.rs

- [ ] acpi 中的 namespace 是如何初始化的？
- [ ] `_S5` 为什么和关机有关?

## 检查一下在 acpid 如何读去 rdst
首先启动的时候，参数 already_supplied_rsdps 为 None


搜索的范围是规定好了的:
```rs
    pub fn get_rsdp_by_searching(active_table: &mut ActivePageTable) -> Option<RSDP> {
        let start_addr = 0xE_0000;
        let end_addr = 0xF_FFFF;
```
搜索的方法就是比对字符串而已

通过该地址可以获取 sdt
```rs
let rxsdt = get_sdt(rsdp.sdt_address(), active_table);

// kernel::acpi:INFO -- rxsdt Sdt { signature: [82, 83, 68, 84], length: 52, revision: 1, checksum: 139, oem_id: [66, 79, 67, 72, 83, 32], oem_table_id: [66, 88, 80, 67, }
```

- [x] 如何将 kernel/acpi:rxsdt 将数据传输过去的
  - 参考 redox/kernel/src/scheme/mod.rs:new_root 函数

现在进入到 redox/cookbook/recipes/drivers/source/acpid/src/main.rs 中间

```c
let acpi_context = self::acpi::AcpiContext::init(physaddrs_iter);
```

```
// 发送过去的原始数据
----------> acpi read
data [82, 83, 68, 84, 52, 0, 0, 0, 1, 139, 66, 79, 67, 72, 83, 32, 66, 88, 80, 67,
82, 83, 68, 84, 1, 0, 0, 0, 66, 88, 80, 67, 1, 0, 0, 0, 12, 32, 254, 127, 0, 33, 254]

// 读去的原始数据
ggda rxsdt_raw_data : [82, 83, 68, 84, 52, 0, 0, 0, 1, 139, 66, 79, 67, 72, 83, 32, 66, 88, 80, 67, 82, 83, 68, 84, 1, 0, 0, 0, 66, 88, 80, 67, 1, 0, 0, 0, 12, 32, 254]

// 解析出来的数据
ggda sdt : Sdt { header: SdtHeader { signature: [82, 83, 68, 84], length: 52, revision: 1, checksum: 139, oem_id: [66, 79, 67, 72, 83, 32], oem_table_id: [66, 88, 80, }

TABLE AT 7FFE200C
TABLE AT 7FFE2100
TABLE AT 7FFE2190
TABLE AT 7FFE21C8
```
- [x] 是怎么从 sdt 中解析出来 table 的

后面都是 table 解析之类的事情，如果想要快乐的分析这些代码，首先修改一下这个的编译系统吧，太他妈的痛苦了，编译一次。
