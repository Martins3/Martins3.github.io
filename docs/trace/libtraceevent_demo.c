/* te_demo.c - libtraceevent 使用示例
 *
 * # 进入包含开发库的 nix-shell
 * nix-shell -p libtraceevent libtracefs pkg-config
 * 
 * # 编译
 * gcc -o te_demo te_demo.c \
 *     $(pkg-config --cflags --libs libtraceevent libtracefs)
 * 
 * # 运行（普通用户可运行，但加载系统事件需要 root 权限）
 * ./te_demo
 * 
 * # 以 root 身份运行以加载实际的内核事件
 * sudo ./te_demo
 *
 * 说明: 本示例展示 libtraceevent 核心 API 的使用，包括:
 *       - 分配/释放 tep_handle
 *       - 使用 libtracefs 加载系统事件
 *       - 遍历系统事件
 *       - 字段解析
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <traceevent/event-parse.h>
#include <traceevent/event-utils.h>
#include <traceevent/trace-seq.h>
#include <libtracefs/tracefs.h>

int main(int argc, char **argv)
{
    struct tep_handle *tep;
    struct tep_event *event;
    struct tep_format_field **fields;
    struct tep_format_field *field;
    struct trace_seq seq;
    const char *tracing_dir;
    int i, j, count, failures;

    printf("=== libtraceevent Demo ===\n\n");

    /* 1. 初始化 tep 句柄 */
    tep = tep_alloc();
    if (!tep) {
        perror("tep_alloc failed");
        return 1;
    }
    printf("[1] tep_handle allocated\n");

    /* 2. 设置解析参数（根据当前系统） */
    tep_set_page_size(tep, 4096);
    tep_set_long_size(tep, sizeof(long));
    printf("[2] Set page_size=4096, long_size=%zu\n", sizeof(long));

    /* 3. 查找 tracing 目录 */
    tracing_dir = tracefs_tracing_dir();
    if (!tracing_dir) {
        fprintf(stderr, "    Error: tracefs tracing dir not found\n");
        tep_free(tep);
        return 1;
    }
    printf("    Tracing dir: %s\n", tracing_dir);

    /* 4. 加载系统事件 */
    printf("\n[3] Loading system events...\n");
    if (tracefs_fill_local_events(tracing_dir, tep, &failures) < 0) {
        fprintf(stderr, "    Failed to load events (need root?)\n");
        /* 继续演示其他功能 */
    } else {
        printf("    Events loaded successfully\n");
        if (failures > 0) {
            printf("    (with %d parsing failures)\n", failures);
        }
    }

    /* 5. 加载 headers */
    if (tracefs_load_headers(tracing_dir, tep) == 0) {
        printf("    Headers loaded\n");
    }

    /* 6. 获取系统中的事件列表 */
    printf("\n[4] Event information:\n");
    count = tep_get_events_count(tep);
    printf("    Total events: %d\n", count);

    /* 7. 展示事件详情 */
    if (count > 0) {
        printf("\n[5] First 10 events:\n");
        for (i = 0; i < count && i < 10; i++) {
            event = tep_get_event(tep, i);
            if (!event) continue;

            printf("    [%d] %s:%s (id=%d)\n",
                   i, event->system, event->name, event->id);

            /* 显示字段信息 (最多3个) */
            fields = tep_event_fields(event);
            if (fields) {
                for (j = 0; fields[j] && j < 3; j++) {
                    field = fields[j];
                    printf("         - %s: %s\n", field->name, field->type);
                }
                free(fields);
            }
        }

        /* 8. 查找特定事件 */
        printf("\n[6] Looking for sched:sched_switch...\n");
        event = tep_find_event_by_name(tep, "sched", "sched_switch");
        if (event) {
            printf("    Found: %s:%s (id=%d)\n",
                   event->system, event->name, event->id);

            fields = tep_event_fields(event);
            if (fields) {
                printf("    Fields:\n");
                for (i = 0; fields[i]; i++) {
                    field = fields[i];
                    printf("      %s: %s (offset=%d, size=%d)\n",
                           field->name, field->type, field->offset, field->size);
                }
                free(fields);
            }
        } else {
            printf("    Not found\n");
        }
    } else {
        printf("\n[5] No events loaded\n");
        printf("    Run with root privileges to load kernel events\n");
    }

    /* 9. 演示 trace_seq */
    printf("\n[7] trace_seq demo:\n");
    trace_seq_init(&seq);
    trace_seq_printf(&seq, "    libtraceevent loaded %d events", count);
    trace_seq_terminate(&seq);
    printf("%s\n", seq.buffer);
    trace_seq_destroy(&seq);

    /* 10. Handle 属性 */
    printf("\n[8] Handle attributes:\n");
    printf("    cpus: %d\n", tep_get_cpus(tep));
    printf("    long_size: %d\n", tep_get_long_size(tep));
    printf("    page_size: %d\n", tep_get_page_size(tep));
    printf("    header_page_size: %d\n", tep_get_header_page_size(tep));

    /* 11. 插件信息 */
    printf("\n[9] Plugins:\n");
    trace_seq_init(&seq);
    tep_print_plugins(&seq, "    ", "\n", NULL);
    trace_seq_terminate(&seq);
    if (seq.len > 0)
        printf("%s\n", seq.buffer);
    else
        printf("    (no plugins loaded)\n");
    trace_seq_destroy(&seq);

    /* 清理 */
    tep_free(tep);

    printf("\n=== Demo completed ===\n");
    return 0;
}
