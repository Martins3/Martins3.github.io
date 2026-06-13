# Reading

Theseus enables all software written in safe Rust, including
applications and libraries, to coexist alongside the core OS
components in a single address space (SAS) and execute at a
single privilege level (SPL), building upon language-provided
type and memory safety to realize isolation instead of hardware protection. Everything presented herein is written in
Rust and runs in the SAS/SPL environment.

Theseus follows three design principles:
P1. Require runtime-persistent bounds for all cells.
P2. Maximize the power of the language and compiler.
P3. Minimize state spill between cells.

很容易搭建。

查看了一下 e1000 的网卡，感觉还是相当的简单的，bar 地址直接从 device 中间读去就可以了。

## 为什么在这里 acpi 的模拟是使用 cpuid 进行的?
/home/maritns3/core/ld/Theseus/kernel/device_manager/src/lib.rs


## acpi 如何工作的
```
+---------+    +-------+    +--------+    +------------------------+
|  RSDP   | +->| XSDT  | +->|  FADT  |    |  +-------------------+ |
+---------+ |  +-------+ |  +--------+  +-|->|       DSDT        | |
| Pointer | |  | Entry |-+  | ...... |  | |  +-------------------+ |
+---------+ |  +-------+    | X_DSDT |--+ |  | Definition Blocks | |
| Pointer |-+  | ..... |    | ...... |    |  +-------------------+ |
+---------+    +-------+    +--------+    |  +-------------------+ |
               | Entry |------------------|->|       SSDT        | |
               +- - - -+                  |  +-------------------| |
               | Entry | - - - - - - - -+ |  | Definition Blocks | |
               +- - - -+                | |  +-------------------+ |
                                        | |  +- - - - - - - - - -+ |
                                        +-|->|       SSDT        | |
                                          |  +-------------------+ |
                                          |  | Definition Blocks | |
                                          |  +- - - - - - - - - -+ |
                                          +------------------------+
                                                       |
                                          OSPM Loading |
                                                      \|/
                                                +----------------+
                                                | ACPI Namespace |
                                                +----------------+

               Figure 1. ACPI Definition Blocks

```


```rs
    /// Returns a reference to the dynamically-sized part at the end of the table that matches the specified ACPI `signature`,
    /// if it exists.
    /// For example, this returns the array of SDT physical addresses at the end of the [`RSDT`](../) table.
    pub fn table_slice<S: FromBytes>(&self, signature: &AcpiSignature) -> Result<&[S], &'static str> {
        let loc = self.tables.get(signature).ok_or("couldn't find ACPI table with matching signature")?;
        let (offset, len) = loc.slice_offset_and_length.ok_or("specified ACPI table has no dynamically-sized part")?;
        self.mapped_pages.as_slice(offset, len)
    }
```

应该是 RSDP 指向 RSDT，而 RSDT 后面会跟上一堆地址，这些地址会进一步指向一堆表格。
