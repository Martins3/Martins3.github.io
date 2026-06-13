

同一个环境构建和运行，就有这个错误，这个是预期的吗?
```txt
libbpf: loading object 'readahead_bpf' from buffer
libbpf: elf: section(3) kprobe/page_cache_ra_unbounded, size 232, link 0, flags 6, type=1
libbpf: sec 'kprobe/page_cache_ra_unbounded': found program 'kprobe_page_cache_ra_unbounded' at insn offset 0 (0 bytes), code size 29 insns (232 bytes)
libbpf: elf: section(4) .relkprobe/page_cache_ra_unbounded, size 48, link 26, flags 40, type=9
libbpf: elf: section(5) license, size 4, link 0, flags 3, type=1
libbpf: license of readahead_bpf is GPL
libbpf: elf: section(6) .maps, size 32, link 0, flags 3, type=1
libbpf: elf: section(16) .BTF, size 1693, link 0, flags 0, type=1
libbpf: elf: section(18) .BTF.ext, size 268, link 0, flags 0, type=1
libbpf: elf: section(26) .symtab, size 336, link 1, flags 0, type=2
libbpf: looking for externs among 14 symbols...
libbpf: collected 0 externs total
libbpf: map 'nr_to_read_count': at sec_idx 6, offset 0.
libbpf: map 'nr_to_read_count': found type = 1.
libbpf: map 'nr_to_read_count': found key [8], sz = 8.
libbpf: map 'nr_to_read_count': found value [8], sz = 8.
libbpf: map 'nr_to_read_count': found max_entries = 2048.
libbpf: sec '.relkprobe/page_cache_ra_unbounded': collecting relocation for section(3) 'kprobe/page_cache_ra_unbounded'
libbpf: sec '.relkprobe/page_cache_ra_unbounded': relo #0: insn #7 against 'nr_to_read_count'
libbpf: prog 'kprobe_page_cache_ra_unbounded': found map 0 (nr_to_read_count, sec 6, off 0) for insn #7
libbpf: sec '.relkprobe/page_cache_ra_unbounded': relo #1: insn #15 against 'nr_to_read_count'
libbpf: prog 'kprobe_page_cache_ra_unbounded': found map 0 (nr_to_read_count, sec 6, off 0) for insn #15
libbpf: sec '.relkprobe/page_cache_ra_unbounded': relo #2: insn #20 against 'nr_to_read_count'
libbpf: prog 'kprobe_page_cache_ra_unbounded': found map 0 (nr_to_read_count, sec 6, off 0) for insn #20
libbpf: object 'readahead_bpf': failed (-95) to create BPF token from '/sys/fs/bpf', skipping optional step...
libbpf: loaded kernel BTF from '/sys/kernel/btf/vmlinux'
libbpf: sec 'kprobe/page_cache_ra_unbounded': found 1 CO-RE relocations
libbpf: CO-RE relocating [15] struct user_pt_regs: found target candidate [271] struct user_pt_regs in [vmlinux]
libbpf: prog 'kprobe_page_cache_ra_unbounded': relo #0: <byte_off> [15] struct user_pt_regs.regs[1] (0:0:1 @ offset 8)
libbpf: prog 'kprobe_page_cache_ra_unbounded': relo #0: matching candidate #0 <byte_off> [271] struct user_pt_regs.regs[1] (0:0:1 @ offset 8)
libbpf: prog 'kprobe_page_cache_ra_unbounded': relo #0: patched insn #0 (LDX/ST/STX) off 8 -> 8
libbpf: map 'nr_to_read_count': created successfully, fd=3
Tracing page_cache_ra_unbounded() nr_to_read... Hit Ctrl-C to end.
.....
```

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
