#!/usr/bin/env python3
"""检查模拟指令 trace 中，是否所有 INSN/MMIO/PIO 记录都落在同一线程的
CALL (fentry kvm:x86_emulate_instruction) 与 RETURN (fexit kvm:x86_emulate_instruction) 之间。

输入为 emulate-insn.bt 采集得到的 bpftrace 原始输出（制表符分隔，一行一条记录）：

    CALL    <tid> <vcpu> <emulation_type> <exit_reason> <cr2_or_gpa>
    INSN    <tid> <csbase> <rip> <flags> <len> <failed> <bytes>
    MMIO    <tid> <type> <len> <gpa> <val>
    PIO     <tid> <rw> <port> <size> <count> <val>
    RETURN  <tid> <retval>

也兼容被追加了时间戳的变体（例如 emulate-insn.raw.tsv）：
    CALL    <ts> <tid> <vcpu> <emulation_type> <exit_reason> <cr2_or_gpa>
    ...（依此类推，<ts> 是一段 >=11 位数字的纳秒时间戳）

“越界”指某条 INSN/MMIO/PIO 出现时，该 tid 上没有尚未配对的 CALL（即不在
call..return 窗口内）。同时报告未配对的 RETURN、嵌套 CALL 以及到 EOF 仍未
关闭的 CALL，便于端到端核对。

注意：bpftrace 退出时会把 @map 打印到输出末尾（以 “@” 开头的行），
这些行会被跳过，不参与配对。

用法：
    ./check_call_return.py [输入文件] [输出文件]
默认输入为仓库根目录下的 a（即 /home/martins3/data/vn/a），
默认输出为脚本同目录下的 a-check.txt。
历史输入 a 已移除；重新采集后可显式传入新文件。
没有 CALL 记录时无法验证配对，不会报告检查成功。
"""

from __future__ import annotations

import sys
from collections import Counter, defaultdict
from pathlib import Path

CALL_TYPES = {"INSN", "MMIO", "PIO"}


def parse_args(argv: list[str]) -> tuple[Path, Path]:
    script_dir = Path(__file__).resolve().parent
    default_in = script_dir.parents[2] / "a"
    default_out = script_dir / "a-check.txt"

    if len(argv) > 1:
        default_in = Path(argv[1])
    if len(argv) > 2:
        default_out = Path(argv[2])
    return default_in, default_out


def detect_tid_index(in_path: Path) -> int:
    """根据第一条 CALL 行的字段形态判断 tid 在第几列（0 起）。

    无时间戳的原始 bpftrace 输出：tid 在第 1 列；
    追加了纳秒时间戳的变体：时间戳（>=11 位）在第 1 列，tid 在第 2 列。
    """
    with in_path.open(encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.rstrip("\n")
            if not line or line.startswith("@"):
                continue
            parts = line.split("\t")
            if parts[0] != "CALL" or len(parts) < 3:
                continue
            return 2 if parts[1].isdigit() and len(parts[1]) >= 11 else 1
    return 1


def main() -> int:
    in_path, out_path = parse_args(sys.argv)
    try:
        tid_idx = detect_tid_index(in_path)
    except OSError as error:
        sys.stderr.write(f"无法读取输入文件 {in_path}: {error.strerror}\n")
        return 2

    # tid -> 当前未配对的 CALL 数量
    depth: Counter[str] = Counter()
    # 记录每次 CALL 的起始行号，便于报告未关闭的 CALL 从哪里开始
    call_line: dict[str, list[int]] = defaultdict(list)

    outside: list[tuple[int, str]] = []  # (行号, 原始行) 越界的 INSN/MMIO/PIO
    unmatched_return: list[tuple[int, str]] = []
    nested_calls: list[tuple[int, str]] = []
    unclosed: list[tuple[str, int]] = []  # (tid, 起始行号) EOF 时仍未关闭的 CALL

    counts: Counter[str] = Counter()
    outside_by_type: Counter[str] = Counter()
    skipped_map_lines = 0

    with in_path.open(encoding="utf-8", errors="replace") as fh:
        for lineno, raw in enumerate(fh, start=1):
            line = raw.rstrip("\n")
            if not line:
                continue
            if line.startswith("@"):
                # bpftrace 退出时打印的 @map 汇总行
                skipped_map_lines += 1
                continue
            parts = line.split("\t")
            kind = parts[0]
            counts[kind] += 1

            if kind not in CALL_TYPES | {"CALL", "RETURN"}:
                # READY / END 等标记行，不参与配对
                continue

            tid = parts[tid_idx] if len(parts) > tid_idx else "?"

            if kind == "CALL":
                if depth[tid] > 0:
                    nested_calls.append((lineno, line))
                depth[tid] += 1
                call_line[tid].append(lineno)
            elif kind == "RETURN":
                if depth[tid] == 0:
                    unmatched_return.append((lineno, line))
                else:
                    depth[tid] -= 1
                    if call_line[tid]:
                        call_line[tid].pop()
            else:  # INSN / MMIO / PIO
                if depth[tid] == 0:
                    outside.append((lineno, line))
                    outside_by_type[kind] += 1

    for tid, starts in call_line.items():
        for start in starts:
            unclosed.append((tid, start))

    # ---------------- 报告 ----------------
    lines_out: list[str] = []
    add = lines_out.append

    add(f"输入文件: {in_path}")
    add(f"tid 列位置: 第 {tid_idx + 1} 列")
    add(f"各类型记录数: {dict(sorted(counts.items()))}")
    add(f"跳过的 @map 汇总行: {skipped_map_lines}")
    add("")

    total_call = counts["CALL"]
    total_return = counts["RETURN"]
    add(f"CALL 总数:   {total_call}")
    add(f"RETURN 总数: {total_return}")
    add("")

    add(f"越界 (不在 call..return 之间) 的 INSN/MMIO/PIO: {len(outside)}")
    if outside_by_type:
        add(f"  其中按类型: {dict(sorted(outside_by_type.items()))}")
    add(f"未配对的 RETURN: {len(unmatched_return)}")
    add(f"嵌套 CALL: {len(nested_calls)}")
    add(f"EOF 时仍未关闭的 CALL: {len(unclosed)}")
    add("")

    ok = (
        total_call > 0
        and not outside
        and not unmatched_return
        and not nested_calls
        and not unclosed
    )
    if total_call == 0:
        add("结论: 没有采集到 CALL 记录，无法验证模拟器调用配对")
    else:
        add(
            "结论: "
            + (
                "所有 INSN/MMIO/PIO 都位于 CALL 与 RETURN 之间"
                if ok
                else "存在不满足 call..return 配对约束的记录，详见下方"
            )
        )

    def dump(title: str, items: list[tuple[int, str]], limit: int = 50) -> None:
        add("")
        add(f"--- {title} (共 {len(items)} 条) ---")
        for lineno, raw in items[:limit]:
            add(f"行 {lineno}: {raw}")
        if len(items) > limit:
            add(f"... 其余 {len(items) - limit} 条省略")

    if outside:
        dump("越界记录", outside)
    if unmatched_return:
        dump("未配对的 RETURN", unmatched_return)
    if nested_calls:
        dump("嵌套 CALL", nested_calls)
    if unclosed:
        add("")
        add(f"--- EOF 未关闭的 CALL (共 {len(unclosed)} 条) ---")
        for tid, start in unclosed[:50]:
            add(f"tid {tid} 起始于行 {start}")

    report = "\n".join(lines_out) + "\n"
    out_path.write_text(report, encoding="utf-8")
    sys.stdout.write(report)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
