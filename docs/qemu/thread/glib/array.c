#include <glib.h>
#include <stdio.h>

// 注意区分 : GArray 和 GPtrArray
// g_ptr_array_new

struct student {
	char name[20];
	int id;
};

int main()
{
	GArray *arr = g_array_new(FALSE, FALSE, sizeof(struct student));
	uint a = g_array_get_element_size(arr);
	printf("element size=%d\n", a);

	struct student s = { "martin", 10 };
	g_array_append_val(arr, s);

	struct student b = { "s3", 12 };
	g_array_append_val(arr, b);

	for (guint i = 0; i < arr->len; i++) {
		struct student stu = g_array_index(arr, struct student, i);
		g_print("[%u] %s %d\n", i, stu.name, stu.id);
	}
	return 0;
}
