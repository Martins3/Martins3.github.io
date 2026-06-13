## 基本使用
### sudo bpftool prog show

虚拟机:
```txt
➜  ~ bpftool prog show

18: cgroup_device  tag 3650d9673c54ce30  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 504B  jited 317B  memlock 4096B
        pids systemd(1)
19: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
20: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
21: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
22: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
23: cgroup_device  tag c2c0d3eb960f5beb  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 560B  jited 359B  memlock 4096B
        pids systemd(1)
24: cgroup_device  tag fe70cb98d44810f6  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 696B  jited 425B  memlock 4096B
        pids systemd(1)
25: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
26: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
27: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
28: cgroup_skb  tag 6deef7357e7b4530  gpl
        loaded_at 2024-07-06T12:24:00+0800  uid 0
        xlated 64B  jited 62B  memlock 4096B
        pids systemd(1)
68: tracepoint  name handle_tp  tag 6a5dcef153b1001e  gpl
        loaded_at 2024-07-06T21:53:39+0800  uid 0
        xlated 104B  jited 73B  memlock 4096B  map_ids 11,12
        btf_id 29
        pids minimal(4183)
```

物理机:
```txt
🤒  sudo bpftool prog show

[sudo] password for martins3:
2: tracing  name hid_tail_call  tag 7cc47bbf07148bfe  gpl
        loaded_at 2024-06-15T11:27:54+0800  uid 0
        xlated 56B  jited 129B  memlock 4096B  map_ids 2
        btf_id 2
23: lsm  name restrict_filesystems  tag 713a545fe0530ce7  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 560B  jited 305B  memlock 4096B  map_ids 13
        btf_id 55
26: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 17
27: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 16
28: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 19
29: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 18
32: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 23
33: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 22
34: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 25
35: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 24
36: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 27
37: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 26
40: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 31
41: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 30
42: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 33
43: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 32
44: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 35
45: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 34
46: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 37
47: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 36
48: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 39
49: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 38
50: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 41
51: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 40
52: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 43
53: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 42
54: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 45
55: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 44
58: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 49
59: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 48
60: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 51
61: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 50
64: cgroup_device  name sd_devices  tag 3a32ccd9e03ea87a  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 504B  jited 318B  memlock 4096B
65: cgroup_skb  name sd_fw_egress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 55
66: cgroup_skb  name sd_fw_ingress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 54
67: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 57
68: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 56
69: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 59
70: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 58
71: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 61
72: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 60
73: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 15
74: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 14
75: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 21
76: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 20
77: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 63
78: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 62
79: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 65
80: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 64
81: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 67
82: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 66
83: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 69
84: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 68
85: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 71
86: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 70
87: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 73
88: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 72
89: cgroup_skb  name sd_fw_egress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 75
90: cgroup_skb  name sd_fw_ingress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:56+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 74
91: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 77
92: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 76
93: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 79
94: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 78
95: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 81
96: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 80
97: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 83
98: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 82
99: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 47
100: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 46
101: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 53
102: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 52
103: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 85
104: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:57+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 84
105: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 87
106: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 86
107: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 89
108: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 88
109: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 91
110: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 90
111: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 93
112: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 92
113: cgroup_device  name sd_devices  tag 63878b01a3de7bae  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 464B  jited 297B  memlock 4096B
114: cgroup_skb  name sd_fw_egress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 95
115: cgroup_skb  name sd_fw_ingress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 94
116: cgroup_device  name sd_devices  tag ee0e253c78993a24  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 416B  jited 264B  memlock 4096B
117: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 97
118: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 96
119: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 99
120: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 98
121: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 101
122: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 100
123: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 103
124: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 102
125: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 105
126: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 104
127: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 107
128: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 106
129: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 109
130: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 108
131: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 111
132: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 110
135: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 115
136: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 114
137: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 117
138: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 116
139: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 119
140: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 118
141: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 121
142: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 120
143: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 123
144: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 122
145: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 125
146: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 124
147: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 127
148: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 126
149: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 129
150: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 128
151: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 131
152: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 130
153: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 133
154: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 132
157: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 137
158: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 136
159: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 139
160: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 138
161: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 141
162: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 140
163: cgroup_skb  name sd_fw_egress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 143
164: cgroup_skb  name sd_fw_ingress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 142
165: cgroup_device  name sd_devices  tag ee0e253c78993a24  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 416B  jited 264B  memlock 4096B
166: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 145
167: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 144
168: cgroup_device  name sd_devices  tag 03e2cf74d47166f5  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 744B  jited 456B  memlock 4096B
169: cgroup_skb  name sd_fw_egress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 147
170: cgroup_skb  name sd_fw_ingress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 146
171: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 149
172: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 148
173: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 151
174: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 150
178: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 155
179: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 154
180: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 157
181: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 156
182: cgroup_device  name sd_devices  tag dbf0b2c646a97b6f  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 184B  jited 110B  memlock 4096B
183: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 159
184: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 158
185: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 161
186: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:58+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 160
187: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 163
188: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 162
189: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 165
190: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 164
191: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 167
192: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 166
193: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 169
194: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 168
195: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 171
196: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 170
197: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 173
198: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 172
199: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 175
200: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 174
201: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 177
202: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 176
203: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 179
204: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 178
205: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 181
206: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 180
207: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 183
208: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 182
209: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 29
210: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 28
217: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 191
218: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:27:59+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 190
219: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:00+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 193
220: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:00+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 192
224: cgroup_skb  name sd_fw_egress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 197
225: cgroup_skb  name sd_fw_ingress  tag 815d6551cd4d7b0b  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 64B  jited 63B  memlock 4096B  map_ids 196
226: cgroup_device  name sd_devices  tag 63878b01a3de7bae  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 464B  jited 297B  memlock 4096B
227: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 199
228: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 198
229: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 201
230: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 200
231: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 203
232: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 202
233: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 205
234: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 204
237: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 209
238: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 208
241: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 213
242: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 212
243: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 215
244: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 214
245: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 217
246: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:05+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 216
249: cgroup_device  name sd_devices  tag 81dfab3bd7eb416a  gpl
        loaded_at 2024-06-15T11:28:07+0800  uid 0
        xlated 472B  jited 303B  memlock 4096B
250: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:07+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 219
251: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:07+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 218
254: cgroup_device  tag ab4bc4523b7fe6b4
        loaded_at 2024-06-15T11:28:07+0800  uid 0
        xlated 552B  jited 365B  memlock 4096B
255: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 222
256: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 221
257: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 224
258: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 223
259: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 226
260: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 225
261: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 228
262: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:24+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 227
269: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:25+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 230
270: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:25+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 229
271: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:33+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 135
272: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:33+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 134
273: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:33+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 211
274: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:33+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 210
279: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:37+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 185
280: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T11:28:37+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 184
461: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T20:35:22+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 280
462: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-06-15T20:35:22+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 279
10061: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T11:46:49+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 234
10062: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T11:46:49+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 233
10231: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T17:10:45+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 611
10232: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T17:10:45+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 610
10256: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T19:38:26+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 113
10257: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T19:38:26+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 112
10262: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T21:23:52+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 1258
10263: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T21:23:52+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 1257
10279: cgroup_skb  name sd_fw_egress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T22:00:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 236
10280: cgroup_skb  name sd_fw_ingress  tag 7dc8126e8768ea37  gpl
        loaded_at 2024-07-06T22:00:01+0800  uid 0
        xlated 312B  jited 201B  memlock 4096B  map_ids 235
```


### sudo bpftrace --info

```txt
[sudo] password for martins3:
System
  OS: Linux 6.9.3 #1-NixOS SMP PREEMPT_DYNAMIC Thu May 30 07:45:04 UTC 2024
  Arch: x86_64

Build
  version: v0.20.4
  LLVM: 17.0.6
  unsafe probe: no
  bfd: yes
  libdw (DWARF support): yes

Kernel helpers
  probe_read: yes
  probe_read_str: yes
  probe_read_user: yes
  probe_read_user_str: yes
  probe_read_kernel: yes
  probe_read_kernel_str: yes
  get_current_cgroup_id: yes
  send_signal: yes
  override_return: no
  get_boot_ns: yes
  dpath: yes
  skboutput: yes
  get_tai_ns: yes
  get_func_ip: yes
  jiffies64: yes

Kernel features
  Instruction limit: 1000000
  Loop support: yes
  btf: yes
  module btf: yes
  map batch: yes
  uprobe refcount (depends on Build:bcc bpf_attach_uprobe refcount): yes

Map types
  hash: yes
  percpu hash: yes
  array: yes
  percpu array: yes
  stack_trace: yes
  perf_event_array: yes
  ringbuf: yes

Probe types
  kprobe: yes
  tracepoint: yes
  perf_event: yes
  kfunc: yes
  kprobe_multi: no
  uprobe_multi: yes
  raw_tp_special: yes
  iter: yes
```

#### 问题

Probe types : iter 是 ?

### bpftool feature

./bpftool.feature

## bpftool 的使用
bpftool prog help

## TODO 看完这个操作吧
https://gist.github.com/navarrothiago/e1f7228610a0bd07aea4928f6381e61d

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
