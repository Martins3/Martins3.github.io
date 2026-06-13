#include "user_nfsd_core.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define UNFS_MAGIC "UNFS"
#define UNFS_MAGIC_LEN 4

static int component_is_dotdot(const char *start, size_t len)
{
	return len == 2 && start[0] == '.' && start[1] == '.';
}

int unfs_path_is_safe(const char *path)
{
	const char *component;
	const char *slash;

	if (!path)
		return 0;
	if (path[0] == '/')
		return 0;

	component = path;
	while (*component) {
		slash = strchr(component, '/');
		if (!slash)
			slash = component + strlen(component);
		if (component_is_dotdot(component, (size_t)(slash - component)))
			return 0;
		component = *slash ? slash + 1 : slash;
	}

	return 1;
}

int unfs_fh_from_path(const char *path, struct unfs_fh *fh)
{
	size_t len;

	if (!fh || !unfs_path_is_safe(path))
		return -EINVAL;

	len = strlen(path);
	if (UNFS_MAGIC_LEN + len + 1 > UNFS_FHSIZE)
		return -ENAMETOOLONG;

	memset(fh, 0, sizeof(*fh));
	memcpy(fh->data, UNFS_MAGIC, UNFS_MAGIC_LEN);
	memcpy(fh->data + UNFS_MAGIC_LEN, path, len + 1);
	fh->len = (unsigned int)(UNFS_MAGIC_LEN + len + 1);
	return 0;
}

int unfs_path_from_fh(const struct unfs_fh *fh, char *path, size_t path_len)
{
	const char *encoded;
	size_t encoded_len;

	if (!fh || !path || path_len == 0)
		return -EINVAL;
	if (fh->len <= UNFS_MAGIC_LEN || fh->len > UNFS_FHSIZE)
		return -EINVAL;
	if (memcmp(fh->data, UNFS_MAGIC, UNFS_MAGIC_LEN) != 0)
		return -EINVAL;
	if (fh->data[fh->len - 1] != '\0')
		return -EINVAL;

	encoded = (const char *)fh->data + UNFS_MAGIC_LEN;
	encoded_len = strlen(encoded);
	if (encoded_len + 1 > path_len)
		return -ENAMETOOLONG;
	if (!unfs_path_is_safe(encoded))
		return -EINVAL;

	memcpy(path, encoded, encoded_len + 1);
	return 0;
}

int unfs_join_export_path(const char *export_root, const char *relpath,
			  char *out, size_t out_len)
{
	size_t root_len;
	int written;

	if (!export_root || !relpath || !out || out_len == 0)
		return -EINVAL;
	if (!unfs_path_is_safe(relpath))
		return -EINVAL;

	root_len = strlen(export_root);
	while (root_len > 1 && export_root[root_len - 1] == '/')
		root_len--;

	if (relpath[0] == '\0')
		written = snprintf(out, out_len, "%.*s", (int)root_len, export_root);
	else if (root_len == 1 && export_root[0] == '/')
		written = snprintf(out, out_len, "/%s", relpath);
	else
		written = snprintf(out, out_len, "%.*s/%s", (int)root_len,
				   export_root, relpath);

	if (written < 0 || (size_t)written >= out_len)
		return -ENAMETOOLONG;
	return 0;
}
