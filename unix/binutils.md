## [ndisasm](https://www.nasm.us/doc/nasmdoca.html)
a companion program to the Netwide Assembler, NASM
```
ndisasm -b {16|32|64} filename
```
## [binutils](https://www.gnu.org/software/binutils/)
Mainly
1. ld - the GNU linker.
1. as - the GNU assembler.

But they also include:
1. addr2line - Converts addresses into filenames and line numbers.
1. ar - A utility for creating, modifying and extracting from archives.
1. c++filt - Filter to demangle encoded C++ symbols.
1. dlltool - Creates files for building and using DLLs.
1. gold - A new, faster, ELF only linker, still in beta test.
1. gprof - Displays profiling information.
1. nlmconv - Converts object code into an NLM.
1. nm - Lists symbols from object files.
1. objcopy - Copies and translates object files.
1. objdump - Displays information from object files.
1. ranlib - Generates an index to the contents of an archive.
1. readelf - Displays information from any ELF format object file.
1. size - Lists the section sizes of an object or archive file.
1. strings - Lists printable strings from files.
1. strip - Discards symbols.
1. windmc - A Windows compatible message compiler.
1. windres - A compiler for Windows resource files.

# [binutils](https://www.gnu.org/software/binutils/)
> 将其他的几个 merge 到此处吧!
https://interrupt.memfault.com/blog/gnu-binutils#readelf

https://github.com/keystone-engine/keystone : as 的替代产品吧
    https://github.com/unicorn-engine/unicorn　
    https://github.com/aquynh/capstone
    // Reversing Trilogy: Capstone (capstone-engine.org), Unicorn (unicorn-engine.org) & Keystone (keystone-engine.org)

### ld
--verbose --gc-sections
