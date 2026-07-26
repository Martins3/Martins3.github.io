// SPDX-License-Identifier: GPL-2.0
/*
 * Observe a VM's QEMU process with DAMON and summarize the access locality.
 *
 * Build:
 *   gcc -O2 -Wall -Wextra -o damon_locality.out damon_locality.c
 *
 * Example:
 *   sudo ./damon_locality.out --vm yyds-fs --guest-ram --duration 30 --snapshot-ms 1000
 *   sudo ./damon_locality.out --vm yyds-fs --guest-ram --pageout-cold --duration 30
 *   ./damon_guest_load.out --mode seq-hotset --total-mb 2048 --hot-mb 256 --seconds 30
 *   sudo ./damon_locality.out --pid $(pidof damon_guest_load.out) --duration 10
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#define DAMON_ROOT "/sys/kernel/mm/damon/admin"
#define DEFAULT_VM_ROOT "/home/martins3/data/hack/vm"
#define MAX_LINE 4096

struct options {
	const char *vm;
	const char *vm_root;
	pid_t pid;
	unsigned int duration_s;
	unsigned int snapshot_ms;
	unsigned long sample_us;
	unsigned long aggr_us;
	unsigned long update_us;
	unsigned long min_regions;
	unsigned long max_regions;
	unsigned int top_n;
	const char *map_substr;
	const char *action;
	unsigned long sz_min;
	unsigned long sz_max;
	unsigned long access_min;
	unsigned long access_max;
	unsigned long age_min;
	unsigned long age_max;
	unsigned long quota_ms;
	unsigned long quota_bytes;
	unsigned long quota_reset_ms;
	unsigned long apply_interval_us;
	bool guest_ram;
	bool leave_running;
};

struct region {
	uint64_t start;
	uint64_t end;
	uint64_t size;
	uint64_t nr_accesses;
	uint64_t age;
};

struct region_vec {
	struct region *items;
	size_t nr;
	size_t cap;
};

static volatile sig_atomic_t stop_requested;
static bool damon_started;

static void vec_push(struct region_vec *vec, struct region r);

static void on_signal(int signo)
{
	(void)signo;
	stop_requested = 1;
}

static void die(const char *msg)
{
	fprintf(stderr, "error: %s: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

static void die_msg(const char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(EXIT_FAILURE);
}

static int write_filef(const char *path, const char *fmt, ...)
{
	char buf[MAX_LINE];
	va_list ap;
	int len;
	int fd;
	ssize_t written;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (len < 0 || (size_t)len >= sizeof(buf)) {
		errno = EOVERFLOW;
		return -1;
	}

	fd = open(path, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;

	written = write(fd, buf, len);
	if (written != len) {
		int saved = errno ? errno : EIO;
		close(fd);
		errno = saved;
		return -1;
	}
	if (close(fd) < 0)
		return -1;
	return 0;
}

static int read_file(const char *path, char *buf, size_t bufsz)
{
	int fd;
	ssize_t n;

	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;
	n = read(fd, buf, bufsz - 1);
	if (n < 0) {
		int saved = errno;
		close(fd);
		errno = saved;
		return -1;
	}
	buf[n] = '\0';
	if (close(fd) < 0)
		return -1;
	return 0;
}

static uint64_t read_u64(const char *path)
{
	char buf[128];
	char *end;
	uint64_t val;

	if (read_file(path, buf, sizeof(buf)) < 0)
		die(path);
	errno = 0;
	val = strtoull(buf, &end, 0);
	if (errno || end == buf)
		die_msg("failed to parse integer from sysfs");
	return val;
}

static void write_sysfs(const char *path, const char *fmt, ...)
{
	char buf[MAX_LINE];
	va_list ap;

	va_start(ap, fmt);
	if (vsnprintf(buf, sizeof(buf), fmt, ap) < 0) {
		va_end(ap);
		die_msg("formatting sysfs value failed");
	}
	va_end(ap);
	if (write_filef(path, "%s", buf) < 0)
		die(path);
}

static void sleep_ms(unsigned int ms)
{
	struct timespec ts = {
		.tv_sec = ms / 1000,
		.tv_nsec = (long)(ms % 1000) * 1000000L,
	};

	while (!stop_requested && nanosleep(&ts, &ts) < 0 && errno == EINTR)
		;
}

static bool pid_alive(pid_t pid)
{
	char path[64];

	snprintf(path, sizeof(path), "/proc/%d", pid);
	return access(path, F_OK) == 0;
}

static pid_t pid_from_vm_pidfile(const struct options *opts)
{
	char path[PATH_MAX];
	char buf[128];
	char *end;
	long pid;

	if (!opts->vm)
		return -1;
	if (snprintf(path, sizeof(path), "%s/%s/s/pid", opts->vm_root, opts->vm)
			>= (int)sizeof(path))
		die_msg("VM pidfile path is too long");
	if (read_file(path, buf, sizeof(buf)) < 0)
		return -1;
	errno = 0;
	pid = strtol(buf, &end, 10);
	if (errno || end == buf || pid <= 0 || pid > INT_MAX)
		return -1;
	return (pid_t)pid;
}

static bool cmdline_contains(pid_t pid, const char *needle)
{
	char path[64];
	char buf[32768];
	int fd;
	ssize_t n;

	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return false;
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return false;
	buf[n] = '\0';
	for (ssize_t i = 0; i < n; i++) {
		if (buf[i] == '\0')
			buf[i] = ' ';
	}
	return strstr(buf, needle) != NULL;
}

static pid_t find_qemu_pid_by_vm(const char *vm)
{
	DIR *proc;
	struct dirent *de;
	pid_t found = -1;

	proc = opendir("/proc");
	if (!proc)
		die("/proc");

	while ((de = readdir(proc))) {
		char *end;
		long pid;

		if (!isdigit((unsigned char)de->d_name[0]))
			continue;
		errno = 0;
		pid = strtol(de->d_name, &end, 10);
		if (errno || *end != '\0' || pid <= 0 || pid > INT_MAX)
			continue;
		if (!cmdline_contains((pid_t)pid, "qemu-system"))
			continue;
		if (!cmdline_contains((pid_t)pid, vm))
			continue;
		if (found != -1) {
			closedir(proc);
			die_msg("multiple QEMU processes matched the VM name");
		}
		found = (pid_t)pid;
	}

	closedir(proc);
	return found;
}

static pid_t resolve_target_pid(const struct options *opts)
{
	pid_t pid;

	if (opts->pid > 0) {
		if (!pid_alive(opts->pid))
			die_msg("given pid does not exist");
		return opts->pid;
	}

	pid = pid_from_vm_pidfile(opts);
	if (pid > 0 && pid_alive(pid) && cmdline_contains(pid, "qemu-system"))
		return pid;

	if (!opts->vm)
		die_msg("use --pid or --vm");

	pid = find_qemu_pid_by_vm(opts->vm);
	if (pid > 0)
		return pid;

	die_msg("VM QEMU process not found; start the VM first");
	return -1;
}

static void find_maps_by_substr(pid_t pid, const char *substr,
				struct region_vec *ranges)
{
	char path[64];
	char line[MAX_LINE];
	FILE *fp;

	snprintf(path, sizeof(path), "/proc/%d/maps", pid);
	fp = fopen(path, "r");
	if (!fp)
		die(path);

	while (fgets(line, sizeof(line), fp)) {
		unsigned long long start, end;

		if (!strstr(line, substr))
			continue;
		if (sscanf(line, "%llx-%llx", &start, &end) != 2 || end <= start)
			continue;
		vec_push(ranges, (struct region) {
			.start = start,
			.end = end,
			.size = end - start,
		});
	}
	if (ferror(fp)) {
		fclose(fp);
		die(path);
	}
	fclose(fp);

	if (!ranges->nr) {
		fprintf(stderr, "error: no mapping containing '%s' found in %s\n",
			substr, path);
		exit(EXIT_FAILURE);
	}
}

static void configure_monitor_regions(const struct region_vec *ranges)
{
	char path[PATH_MAX];

	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/targets/0/regions/nr_regions",
		    "%zu", ranges->nr);
	for (size_t i = 0; i < ranges->nr; i++) {
		if (snprintf(path, sizeof(path),
			     DAMON_ROOT "/kdamonds/0/contexts/0/targets/0/regions/%zu/start",
			     i) >= (int)sizeof(path))
			die_msg("region start path is too long");
		write_sysfs(path, "%" PRIu64, ranges->items[i].start);

		if (snprintf(path, sizeof(path),
			     DAMON_ROOT "/kdamonds/0/contexts/0/targets/0/regions/%zu/end",
			     i) >= (int)sizeof(path))
			die_msg("region end path is too long");
		write_sysfs(path, "%" PRIu64, ranges->items[i].end);
	}
}

static void configure_damon(const struct options *opts, pid_t pid,
			    const struct region_vec *ranges)
{
	if (access(DAMON_ROOT "/kdamonds/nr_kdamonds", W_OK) < 0)
		die_msg("DAMON sysfs is not writable; run as root");

	write_sysfs(DAMON_ROOT "/kdamonds/nr_kdamonds", "1");
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/nr_contexts", "1");
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/operations",
		    opts->guest_ram ? "fvaddr" : "vaddr");
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us",
		    "%lu", opts->sample_us);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us",
		    "%lu", opts->aggr_us);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us",
		    "%lu", opts->update_us);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min",
		    "%lu", opts->min_regions);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max",
		    "%lu", opts->max_regions);

	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/targets/nr_targets", "1");
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/targets/0/pid_target", "%d", pid);
	if (opts->guest_ram)
		configure_monitor_regions(ranges);
	else
		write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/targets/0/regions/nr_regions", "0");

	/*
	 * The scheme exposes matched regions through tried_regions. With the
	 * default stat action this is observation-only; pageout applies reclaim
	 * only to regions matching the configured access pattern.
	 */
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/nr_schemes", "1");
}

static void configure_scheme(const struct options *opts)
{
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/nr_schemes", "1");
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/min",
		    "%lu", opts->sz_min);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/max",
		    "%lu", opts->sz_max);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/min",
		    "%lu", opts->access_min);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/max",
		    "%lu", opts->access_max);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/access_pattern/age/min",
		    "%lu", opts->age_min);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/access_pattern/age/max",
		    "%lu", opts->age_max);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/action", "%s", opts->action);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/quotas/ms",
		    "%lu", opts->quota_ms);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/quotas/bytes",
		    "%lu", opts->quota_bytes);
	write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/quotas/reset_interval_ms",
		    "%lu", opts->quota_reset_ms);
	if (access(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/apply_interval_us", W_OK) == 0)
		write_sysfs(DAMON_ROOT "/kdamonds/0/contexts/0/schemes/0/apply_interval_us",
			    "%lu", opts->apply_interval_us);
}

static void start_damon(void)
{
	write_sysfs(DAMON_ROOT "/kdamonds/0/state", "on");
	damon_started = true;
}

static void stop_damon(bool leave_running)
{
	if (leave_running || !damon_started)
		return;

	if (access(DAMON_ROOT "/kdamonds/0/state", W_OK) == 0) {
		write_filef(DAMON_ROOT "/kdamonds/0/state", "off");
		write_filef(DAMON_ROOT "/kdamonds/0/state", "clear_schemes_tried_regions");
	}
	if (access(DAMON_ROOT "/kdamonds/nr_kdamonds", W_OK) == 0)
		write_filef(DAMON_ROOT "/kdamonds/nr_kdamonds", "0");
	damon_started = false;
}

static int cmp_region_dir(const void *a, const void *b)
{
	const char * const *sa = a;
	const char * const *sb = b;
	long ia = strtol(*sa, NULL, 10);
	long ib = strtol(*sb, NULL, 10);

	return (ia > ib) - (ia < ib);
}

static void vec_push(struct region_vec *vec, struct region r)
{
	if (vec->nr == vec->cap) {
		size_t new_cap = vec->cap ? vec->cap * 2 : 64;
		struct region *new_items = reallocarray(vec->items, new_cap,
							sizeof(*vec->items));

		if (!new_items)
			die("reallocarray");
		vec->items = new_items;
		vec->cap = new_cap;
	}
	vec->items[vec->nr++] = r;
}

static bool is_number_name(const char *s)
{
	if (!*s)
		return false;
	while (*s) {
		if (!isdigit((unsigned char)*s))
			return false;
		s++;
	}
	return true;
}

static void read_region_file(char *path, size_t pathsz, const char *dir,
			     const char *file, uint64_t *out)
{
	if (snprintf(path, pathsz, "%s/%s", dir, file) >= (int)pathsz)
		die_msg("region path is too long");
	*out = read_u64(path);
}

static struct region_vec read_snapshot(void)
{
	const char *tried = DAMON_ROOT
		"/kdamonds/0/contexts/0/schemes/0/tried_regions";
	struct region_vec vec = {};
	DIR *dir;
	struct dirent *de;
	char **names = NULL;
	size_t nr_names = 0, cap_names = 0;

	write_sysfs(DAMON_ROOT "/kdamonds/0/state", "update_schemes_tried_regions");

	dir = opendir(tried);
	if (!dir)
		die(tried);

	while ((de = readdir(dir))) {
		if (!is_number_name(de->d_name))
			continue;
		if (nr_names == cap_names) {
			size_t new_cap = cap_names ? cap_names * 2 : 64;
			char **new_names = reallocarray(names, new_cap, sizeof(*names));

			if (!new_names)
				die("reallocarray");
			names = new_names;
			cap_names = new_cap;
		}
		names[nr_names] = strdup(de->d_name);
		if (!names[nr_names])
			die("strdup");
		nr_names++;
	}
	closedir(dir);

	qsort(names, nr_names, sizeof(*names), cmp_region_dir);

	for (size_t i = 0; i < nr_names; i++) {
		char rdir[PATH_MAX];
		char path[PATH_MAX];
		struct region r = {};

		if (snprintf(rdir, sizeof(rdir), "%s/%s", tried, names[i])
				>= (int)sizeof(rdir))
			die_msg("region directory path is too long");
		read_region_file(path, sizeof(path), rdir, "start", &r.start);
		read_region_file(path, sizeof(path), rdir, "end", &r.end);
		read_region_file(path, sizeof(path), rdir, "nr_accesses", &r.nr_accesses);
		read_region_file(path, sizeof(path), rdir, "age", &r.age);
		if (r.end > r.start) {
			r.size = r.end - r.start;
			vec_push(&vec, r);
		}
		free(names[i]);
	}
	free(names);
	return vec;
}

static int cmp_access_desc(const void *a, const void *b)
{
	const struct region *ra = a;
	const struct region *rb = b;

	if (ra->nr_accesses != rb->nr_accesses)
		return (ra->nr_accesses < rb->nr_accesses) ? 1 : -1;
	if (ra->size != rb->size)
		return (ra->size < rb->size) ? 1 : -1;
	return 0;
}

static int cmp_access_asc(const void *a, const void *b)
{
	const struct region *ra = a;
	const struct region *rb = b;

	return (ra->nr_accesses > rb->nr_accesses) -
		(ra->nr_accesses < rb->nr_accesses);
}

static double bytes_to_gib(uint64_t bytes)
{
	return (double)bytes / 1024.0 / 1024.0 / 1024.0;
}

static double gini_accesses(const struct region_vec *vec)
{
	struct region *copy;
	long double weighted_sum = 0;
	long double total_access = 0;
	long double n;

	if (!vec->nr)
		return 0.0;
	copy = malloc(vec->nr * sizeof(*copy));
	if (!copy)
		die("malloc");
	memcpy(copy, vec->items, vec->nr * sizeof(*copy));
	qsort(copy, vec->nr, sizeof(*copy), cmp_access_asc);

	for (size_t i = 0; i < vec->nr; i++) {
		total_access += copy[i].nr_accesses;
		weighted_sum += (long double)(i + 1) * copy[i].nr_accesses;
	}
	free(copy);
	if (total_access == 0)
		return 0.0;
	n = vec->nr;
	return (double)((2.0L * weighted_sum) / (n * total_access) -
			(n + 1.0L) / n);
}

static void print_snapshot_summary(unsigned int idx, const struct options *opts,
				   const struct region_vec *vec)
{
	struct region *sorted;
	uint64_t total = 0, active = 0, cold = 0, hot = 0;
	uint64_t access_sum = 0, max_access = 0;
	uint64_t top_bytes = 0, top_accesses = 0;
	uint64_t hot_islands = 0;
	bool in_hot = false;
	double gini;
	size_t top_n;

	for (size_t i = 0; i < vec->nr; i++) {
		const struct region *r = &vec->items[i];

		total += r->size;
		access_sum += r->nr_accesses;
		if (r->nr_accesses > max_access)
			max_access = r->nr_accesses;
		if (r->nr_accesses > 0)
			active += r->size;
		else
			cold += r->size;
	}

	/*
	 * Use half of the snapshot's max access count as a simple hot threshold.
	 * DAMON nr_accesses is per aggregation window, so absolute values depend
	 * on sample_us/aggr_us; the relative threshold is more stable.
	 */
	for (size_t i = 0; i < vec->nr; i++) {
		bool is_hot = max_access > 0 &&
			vec->items[i].nr_accesses * 2 >= max_access;

		if (is_hot) {
			hot += vec->items[i].size;
			if (!in_hot)
				hot_islands++;
		}
		in_hot = is_hot;
	}

	sorted = malloc(vec->nr * sizeof(*sorted));
	if (!sorted)
		die("malloc");
	memcpy(sorted, vec->items, vec->nr * sizeof(*sorted));
	qsort(sorted, vec->nr, sizeof(*sorted), cmp_access_desc);
	top_n = opts->top_n < vec->nr ? opts->top_n : vec->nr;
	for (size_t i = 0; i < top_n; i++) {
		top_bytes += sorted[i].size;
		top_accesses += sorted[i].nr_accesses;
	}
	gini = gini_accesses(vec);

	printf("\n[snapshot %u] regions=%zu total=%.3f GiB active=%.3f GiB (%.1f%%) "
	       "cold=%.3f GiB hot=%.3f GiB hot_islands=%" PRIu64
	       " max_access=%" PRIu64 " access_sum=%" PRIu64 " gini=%.3f\n",
	       idx, vec->nr, bytes_to_gib(total), bytes_to_gib(active),
	       total ? 100.0 * (double)active / (double)total : 0.0,
	       bytes_to_gib(cold), bytes_to_gib(hot), hot_islands,
	       max_access, access_sum, gini);
	printf("top%zu: bytes=%.3f GiB (%.1f%% of monitored), accesses=%" PRIu64
	       " (%.1f%% of region-access sum)\n",
	       top_n, bytes_to_gib(top_bytes),
	       total ? 100.0 * (double)top_bytes / (double)total : 0.0,
	       top_accesses,
	       access_sum ? 100.0 * (double)top_accesses / (double)access_sum : 0.0);
	printf("top regions by access:\n");
	for (size_t i = 0; i < top_n; i++) {
		printf("  #%zu 0x%016" PRIx64 "-0x%016" PRIx64
		       " size=%8.3f MiB access=%" PRIu64 " age=%" PRIu64 "\n",
		       i, sorted[i].start, sorted[i].end,
		       (double)sorted[i].size / 1024.0 / 1024.0,
		       sorted[i].nr_accesses, sorted[i].age);
	}

	if (max_access == 0)
		printf("locality: no observed accesses in this snapshot\n");
	else if (active * 10 <= total && gini >= 0.60)
		printf("locality: strong hot-set concentration\n");
	else if (active * 3 <= total && gini >= 0.35)
		printf("locality: moderate concentration\n");
	else
		printf("locality: weak or broadly spread activity\n");

	free(sorted);
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s [--vm NAME | --pid PID] [options]\n"
		"\n"
		"options:\n"
		"  --vm NAME              collei VM name, e.g. yyds-fs\n"
		"  --vm-root PATH         VM root directory [%s]\n"
		"  --pid PID              monitor an explicit QEMU/process PID\n"
		"  --duration SECONDS     observation duration [30]\n"
		"  --snapshot-ms MS       snapshot interval [1000]\n"
		"  --sample-us US         DAMON sampling interval [5000]\n"
		"  --aggr-us US           DAMON aggregation interval [100000]\n"
		"  --update-us US         DAMON target update interval [1000000]\n"
		"  --min-regions N        DAMON min regions [10]\n"
		"  --max-regions N        DAMON max regions [1000]\n"
		"  --top N                print top N regions [8]\n"
		"  --guest-ram            use fvaddr and monitor only QEMU guest RAM mapping\n"
		"  --map-substr TEXT      maps substring for --guest-ram [/memfd:memory-backend-memfd]\n"
		"  --action ACTION        DAMOS action: stat or pageout [stat]\n"
		"  --pageout-cold         shortcut for cold guest/process memory pageout\n"
		"  --sz-min BYTES         scheme size lower bound [0]\n"
		"  --sz-max BYTES         scheme size upper bound [ULONG_MAX]\n"
		"  --access-min N         scheme nr_accesses lower bound [0]\n"
		"  --access-max N         scheme nr_accesses upper bound [UINT_MAX]\n"
		"  --age-min N            scheme age lower bound [0]\n"
		"  --age-max N            scheme age upper bound [UINT_MAX]\n"
		"  --quota-ms MS          quota runtime per reset interval [0]\n"
		"  --quota-bytes BYTES    quota bytes per reset interval [0]\n"
		"  --quota-reset-ms MS    quota reset interval [1000]\n"
		"  --apply-interval-us US scheme apply interval if supported [0]\n"
		"  --leave-running        do not stop DAMON on exit\n",
		argv0, DEFAULT_VM_ROOT);
}

static unsigned long parse_ulong_arg(const char *opt, const char *val)
{
	char *end;
	unsigned long parsed;

	errno = 0;
	parsed = strtoul(val, &end, 0);
	if (errno || end == val || *end != '\0') {
		fprintf(stderr, "invalid value for %s: %s\n", opt, val);
		exit(EXIT_FAILURE);
	}
	return parsed;
}

static void parse_args(int argc, char **argv, struct options *opts)
{
	*opts = (struct options) {
		.vm_root = DEFAULT_VM_ROOT,
		.pid = -1,
		.duration_s = 30,
		.snapshot_ms = 1000,
		.sample_us = 5000,
		.aggr_us = 100000,
		.update_us = 1000000,
		.min_regions = 10,
		.max_regions = 1000,
		.top_n = 8,
		.map_substr = "/memfd:memory-backend-memfd",
		.action = "stat",
		.sz_min = 0,
		.sz_max = ULONG_MAX,
		.access_min = 0,
		.access_max = UINT_MAX,
		.age_min = 0,
		.age_max = UINT_MAX,
		.quota_ms = 0,
		.quota_bytes = 0,
		.quota_reset_ms = 1000,
		.apply_interval_us = 0,
	};

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		const char *val = i + 1 < argc ? argv[i + 1] : NULL;

		if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--leave-running")) {
			opts->leave_running = true;
		} else if (!strcmp(arg, "--guest-ram")) {
			opts->guest_ram = true;
		} else if (!strcmp(arg, "--pageout-cold")) {
			opts->action = "pageout";
			opts->access_min = 0;
			opts->access_max = 0;
			opts->age_min = 50;
			opts->age_max = UINT_MAX;
			opts->quota_ms = 10;
			opts->quota_bytes = 1024UL * 1024UL * 1024UL;
			opts->quota_reset_ms = 1000;
		} else if (!val) {
			fprintf(stderr, "missing value for %s\n", arg);
			exit(EXIT_FAILURE);
		} else if (!strcmp(arg, "--vm")) {
			opts->vm = val;
			i++;
		} else if (!strcmp(arg, "--vm-root")) {
			opts->vm_root = val;
			i++;
		} else if (!strcmp(arg, "--pid")) {
			opts->pid = (pid_t)parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--duration")) {
			opts->duration_s = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--snapshot-ms")) {
			opts->snapshot_ms = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--sample-us")) {
			opts->sample_us = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--aggr-us")) {
			opts->aggr_us = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--update-us")) {
			opts->update_us = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--min-regions")) {
			opts->min_regions = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--max-regions")) {
			opts->max_regions = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--top")) {
			opts->top_n = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--map-substr")) {
			opts->map_substr = val;
			i++;
		} else if (!strcmp(arg, "--action")) {
			opts->action = val;
			i++;
		} else if (!strcmp(arg, "--sz-min")) {
			opts->sz_min = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--sz-max")) {
			opts->sz_max = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--access-min")) {
			opts->access_min = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--access-max")) {
			opts->access_max = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--age-min")) {
			opts->age_min = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--age-max")) {
			opts->age_max = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--quota-ms")) {
			opts->quota_ms = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--quota-bytes")) {
			opts->quota_bytes = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--quota-reset-ms")) {
			opts->quota_reset_ms = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--apply-interval-us")) {
			opts->apply_interval_us = parse_ulong_arg(arg, val);
			i++;
		} else {
			fprintf(stderr, "unknown option: %s\n", arg);
			exit(EXIT_FAILURE);
		}
	}
	if (!opts->vm && opts->pid <= 0)
		die_msg("use --vm or --pid");
	if (!opts->duration_s || !opts->snapshot_ms)
		die_msg("duration and snapshot interval must be non-zero");
	if (strcmp(opts->action, "stat") && strcmp(opts->action, "pageout"))
		die_msg("--action currently supports stat or pageout");
	if (!strcmp(opts->action, "pageout") &&
	    (opts->access_max != 0 || opts->age_min == 0))
		die_msg("pageout requires a cold pattern; use --pageout-cold or set --access-max 0 --age-min N");
	if (opts->sz_max < opts->sz_min)
		die_msg("--sz-max must be >= --sz-min");
	if (opts->access_max < opts->access_min)
		die_msg("--access-max must be >= --access-min");
	if (opts->age_max < opts->age_min)
		die_msg("--age-max must be >= --age-min");
}

int main(int argc, char **argv)
{
	struct options opts;
	struct region_vec monitor_ranges = {};
	pid_t pid;
	unsigned int snapshots;

	parse_args(argc, argv, &opts);
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	pid = resolve_target_pid(&opts);
	printf("target pid: %d%s%s\n", pid, opts.vm ? " vm=" : "",
	       opts.vm ? opts.vm : "");
	if (opts.guest_ram)
		find_maps_by_substr(pid, opts.map_substr, &monitor_ranges);
	printf("DAMON: op=%s sample=%luus aggr=%luus update=%luus regions=[%lu,%lu]\n",
	       opts.guest_ram ? "fvaddr" : "vaddr", opts.sample_us, opts.aggr_us,
	       opts.update_us, opts.min_regions, opts.max_regions);
	printf("scheme: action=%s sz=[%lu,%lu] accesses=[%lu,%lu] age=[%lu,%lu] "
	       "quota=%lums/%lu bytes reset=%lums\n",
	       opts.action, opts.sz_min, opts.sz_max, opts.access_min,
	       opts.access_max, opts.age_min, opts.age_max, opts.quota_ms,
	       opts.quota_bytes, opts.quota_reset_ms);
	for (size_t i = 0; i < monitor_ranges.nr; i++) {
		printf("monitor range %zu: 0x%016" PRIx64 "-0x%016" PRIx64
		       " %.3f GiB (%s)\n",
		       i, monitor_ranges.items[i].start, monitor_ranges.items[i].end,
		       bytes_to_gib(monitor_ranges.items[i].size), opts.map_substr);
	}

	configure_damon(&opts, pid, &monitor_ranges);
	configure_scheme(&opts);
	start_damon();

	/*
	 * Wait at least one aggregation interval before the first snapshot.
	 */
	sleep_ms((unsigned int)(opts.aggr_us / 1000 + opts.snapshot_ms));
	snapshots = opts.duration_s * 1000 / opts.snapshot_ms;
	if (!snapshots)
		snapshots = 1;

	for (unsigned int i = 0; i < snapshots && !stop_requested; i++) {
		struct region_vec vec = read_snapshot();

		print_snapshot_summary(i, &opts, &vec);
		free(vec.items);
		if (i + 1 < snapshots)
			sleep_ms(opts.snapshot_ms);
	}

	stop_damon(opts.leave_running);
	free(monitor_ranges.items);
	return 0;
}
