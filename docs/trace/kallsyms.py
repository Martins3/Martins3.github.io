#!/usr/bin/env python3

system_map_entries = set()
kallsyms_entries = set()

# 提前准备
# sudo cat /proc/kallsyms | sudo tee /tmp/kallsyms
def init():
    # Read kallsyms (requires root)
    try:
        with open('/tmp/kallsyms', 'r') as f:
            kallsyms = [line.strip() for line in f.readlines()]
    except PermissionError:
        print("Error: Need root privileges to read /proc/kallsyms")
        return

    # Read System.map
    try:
        with open('/boot/System.map', 'r') as f:
            system_map = [line.strip() for line in f.readlines()]
    except FileNotFoundError:
        print("Error: /boot/System.map not found")
        return

    # Build a set of (address, name) tuples from System.map for faster lookup
    for line in system_map:
        parts = line.split()
        if len(parts) >= 3:  # Ensure we have address, type, and name
            system_map_entries.add((parts[0], parts[2]))

    for line in kallsyms:
        parts = line.split()
        if len(parts) < 3:
            continue

        if len(parts) == 4:
            # print(parts[3])
            # 过滤掉 modules 的内容
            continue

        addr = parts[0]
        name = parts[2]
        kallsyms_entries.add((addr, name))

def unique_in_system_map():
    for i in system_map_entries:
        if i not in kallsyms_entries:
            print(f"{i[0]} : {i[1]}")
        break

def unique_in_kallsyms():
    for i in kallsyms_entries:
        if i not in system_map_entries:
            print(f"{i[0]} : {i[1]}")

if __name__ == "__main__":
    init()
    # unique_in_kallsyms()
    unique_in_system_map()
