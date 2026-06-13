#include "user_nfsd_core.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void test_file_handle_round_trip(void)
{
	struct unfs_fh fh;
	char path[UNFS_MAX_PATH];

	assert(unfs_fh_from_path("dir/file", &fh) == 0);
	assert(unfs_path_from_fh(&fh, path, sizeof(path)) == 0);
	assert(strcmp(path, "dir/file") == 0);
}

static void test_root_handle_is_empty_path(void)
{
	struct unfs_fh fh;
	char path[UNFS_MAX_PATH];

	assert(unfs_fh_from_path("", &fh) == 0);
	assert(unfs_path_from_fh(&fh, path, sizeof(path)) == 0);
	assert(strcmp(path, "") == 0);
}

static void test_rejects_traversal(void)
{
	struct unfs_fh fh;

	assert(unfs_fh_from_path("../escape", &fh) == -EINVAL);
	assert(unfs_fh_from_path("a/../../escape", &fh) == -EINVAL);
	assert(unfs_fh_from_path("/absolute", &fh) == -EINVAL);
}

static void test_join_export_root(void)
{
	char out[UNFS_MAX_PATH];

	assert(unfs_join_export_path("/tmp/export", "dir/file", out, sizeof(out)) == 0);
	assert(strcmp(out, "/tmp/export/dir/file") == 0);
	assert(unfs_join_export_path("/tmp/export", "", out, sizeof(out)) == 0);
	assert(strcmp(out, "/tmp/export") == 0);
}

int main(void)
{
	test_file_handle_round_trip();
	test_root_handle_is_empty_path();
	test_rejects_traversal();
	test_join_export_root();
	puts("path tests passed");
	return 0;
}
