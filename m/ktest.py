#!/usr/bin/env python3
import re
import sys

def convert_line(line):
    match = re.match(r'DEFINE_TESTER(_RESOURCE)?\((\w+)\)', line.strip())
    if match:
        name = match.group(2)
        config_macro = f"CONFIG_TEST_{name.upper()}"
        print("????")
        return f"#ifdef {config_macro}\n{line.rstrip()}\n#endif\n"

    match = re.match(r'\s*&(\w+)_attribute\.attr,', line.strip())
    if match:
        name = match.group(1)
        config_macro = f"CONFIG_TEST_{name.upper()}"
        print("----")
        return f"#ifdef {config_macro}\n{line.rstrip()}\n#endif\n"

    return line


def process_file(input_file, output_file=None):
    with open(input_file, 'r') as f:
        lines = f.readlines()

    processed_lines = []
    for line in lines:
        processed_lines.append(convert_line(line))

    output = ''.join(processed_lines)

    if output_file:
       with open(output_file, 'w') as f:
            f.write(output)
    else:
        print(output)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python convert_tester.py <input_file> [output_file]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    process_file(input_file, output_file)
