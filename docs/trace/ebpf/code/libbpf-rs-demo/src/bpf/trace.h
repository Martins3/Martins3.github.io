#ifndef __LIBBPF_RS_DEMO_TRACE_H
#define __LIBBPF_RS_DEMO_TRACE_H

#define TASK_COMM_LEN 16

struct event {
    unsigned int pid;
    int fd;
    unsigned long long count;
    char comm[TASK_COMM_LEN];
};

#endif
