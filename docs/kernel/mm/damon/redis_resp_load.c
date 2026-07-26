// SPDX-License-Identifier: GPL-2.0
/*
 * Minimal Redis RESP workload generator for DAMON locality experiments.
 *
 * Build:
 *   gcc -O2 -Wall -Wextra -o redis_resp_load.out redis_resp_load.c
 *
 * Examples:
 *   ./redis_resp_load.out --port 6380 --mode prepare
 *   ./redis_resp_load.out --port 6380 --mode get-hotset --seconds 30
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 6380
#define DEFAULT_TOTAL_KEYS 262144UL
#define DEFAULT_HOT_KEYS 65536UL
#define DEFAULT_VALUE_BYTES 4096UL
#define DEFAULT_PIPELINE 64UL
#define DEFAULT_SECONDS 30U

enum mode {
	MODE_PREPARE,
	MODE_GET_HOTSET,
	MODE_SET_HOTSET,
};

struct options {
	const char *host;
	unsigned int port;
	enum mode mode;
	unsigned long total_keys;
	unsigned long hot_keys;
	unsigned long value_bytes;
	unsigned long pipeline;
	unsigned int seconds;
};

static uint64_t nsec_now(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		return 0;
	return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static uint64_t xorshift64(uint64_t *state)
{
	uint64_t x = *state;

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*state = x;
	return x;
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

static enum mode parse_mode(const char *value)
{
	if (!strcmp(value, "prepare"))
		return MODE_PREPARE;
	if (!strcmp(value, "get-hotset"))
		return MODE_GET_HOTSET;
	if (!strcmp(value, "set-hotset"))
		return MODE_SET_HOTSET;
	fprintf(stderr, "invalid mode: %s\n", value);
	exit(EXIT_FAILURE);
}

static const char *mode_name(enum mode mode)
{
	switch (mode) {
	case MODE_PREPARE:
		return "prepare";
	case MODE_GET_HOTSET:
		return "get-hotset";
	case MODE_SET_HOTSET:
		return "set-hotset";
	}
	return "unknown";
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s [--host IP] [--port PORT] --mode MODE [options]\n"
		"\n"
		"modes: prepare, get-hotset, set-hotset\n"
		"defaults: --host 127.0.0.1 --port 6380 --total-keys 262144 "
		"--hot-keys 65536 --value-bytes 4096 --pipeline 64 --seconds 30\n",
		argv0);
}

static void parse_args(int argc, char **argv, struct options *opts)
{
	*opts = (struct options) {
		.host = DEFAULT_HOST,
		.port = DEFAULT_PORT,
		.mode = MODE_PREPARE,
		.total_keys = DEFAULT_TOTAL_KEYS,
		.hot_keys = DEFAULT_HOT_KEYS,
		.value_bytes = DEFAULT_VALUE_BYTES,
		.pipeline = DEFAULT_PIPELINE,
		.seconds = DEFAULT_SECONDS,
	};

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		const char *val = i + 1 < argc ? argv[i + 1] : NULL;

		if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		}
		if (!val) {
			fprintf(stderr, "missing value for %s\n", arg);
			exit(EXIT_FAILURE);
		}
		if (!strcmp(arg, "--host")) {
			opts->host = val;
			i++;
		} else if (!strcmp(arg, "--port")) {
			opts->port = (unsigned int)parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--mode")) {
			opts->mode = parse_mode(val);
			i++;
		} else if (!strcmp(arg, "--total-keys")) {
			opts->total_keys = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--hot-keys")) {
			opts->hot_keys = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--value-bytes")) {
			opts->value_bytes = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--pipeline")) {
			opts->pipeline = parse_ulong_arg(arg, val);
			i++;
		} else if (!strcmp(arg, "--seconds")) {
			opts->seconds = (unsigned int)parse_ulong_arg(arg, val);
			i++;
		} else {
			fprintf(stderr, "unknown option: %s\n", arg);
			exit(EXIT_FAILURE);
		}
	}

	if (!opts->port || opts->port > 65535)
		die_msg("port must be in 1..65535");
	if (!opts->total_keys || !opts->hot_keys || !opts->value_bytes ||
	    !opts->pipeline)
		die_msg("key counts, value size, and pipeline must be non-zero");
	if (opts->hot_keys > opts->total_keys)
		opts->hot_keys = opts->total_keys;
}

static int redis_connect(const struct options *opts)
{
	struct sockaddr_in addr = {};
	int fd;
	int one = 1;

	fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		die("socket");

	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)opts->port);
	if (inet_pton(AF_INET, opts->host, &addr.sin_addr) != 1)
		die_msg("only numeric IPv4 --host is supported");
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		die("connect");
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	return fd;
}

static void send_all(int fd, const void *buf, size_t len)
{
	const char *p = buf;

	while (len) {
		ssize_t n = send(fd, p, len, MSG_NOSIGNAL);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			die("send");
		}
		if (!n)
			die_msg("short send");
		p += n;
		len -= (size_t)n;
	}
}

static char read_byte(int fd)
{
	char c;

	for (;;) {
		ssize_t n = recv(fd, &c, 1, 0);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			die("recv");
		}
		if (!n)
			die_msg("redis closed connection");
		return c;
	}
}

static void read_line(int fd, char *buf, size_t bufsz)
{
	size_t pos = 0;

	if (!bufsz)
		die_msg("zero line buffer");
	for (;;) {
		char c = read_byte(fd);

		if (c == '\r') {
			if (read_byte(fd) != '\n')
				die_msg("malformed redis line ending");
			buf[pos] = '\0';
			return;
		}
		if (pos + 1 >= bufsz)
			die_msg("redis line too long");
		buf[pos++] = c;
	}
}

static void discard_exact(int fd, unsigned long len)
{
	char buf[8192];

	while (len) {
		size_t want = len < sizeof(buf) ? len : sizeof(buf);
		ssize_t n = recv(fd, buf, want, 0);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			die("recv");
		}
		if (!n)
			die_msg("redis closed connection");
		len -= (unsigned long)n;
	}
}

static void read_reply(int fd)
{
	char type = read_byte(fd);
	char line[256];
	long bulk_len;

	read_line(fd, line, sizeof(line));
	switch (type) {
	case '+':
	case ':':
		return;
	case '-':
		fprintf(stderr, "redis error: %s\n", line);
		exit(EXIT_FAILURE);
	case '$':
		errno = 0;
		bulk_len = strtol(line, NULL, 10);
		if (errno || bulk_len < -1)
			die_msg("bad redis bulk length");
		if (bulk_len >= 0)
			discard_exact(fd, (unsigned long)bulk_len + 2);
		return;
	default:
		fprintf(stderr, "unexpected redis reply type: %c\n", type);
		exit(EXIT_FAILURE);
	}
}

static void send_get(int fd, unsigned long key)
{
	char cmd[128];
	int len = snprintf(cmd, sizeof(cmd),
			   "*2\r\n$3\r\nGET\r\n$%zu\r\nkey:%lu\r\n",
			   strlen("key:") + snprintf(NULL, 0, "%lu", key), key);

	if (len < 0 || (size_t)len >= sizeof(cmd))
		die_msg("GET command too long");
	send_all(fd, cmd, (size_t)len);
}

static void send_set(int fd, unsigned long key, const char *value,
		     unsigned long value_bytes)
{
	char head[160];
	int key_len = snprintf(NULL, 0, "key:%lu", key);
	int len;

	if (key_len < 0)
		die_msg("key formatting failed");
	len = snprintf(head, sizeof(head),
		       "*3\r\n$3\r\nSET\r\n$%d\r\nkey:%lu\r\n$%lu\r\n",
		       key_len, key, value_bytes);
	if (len < 0 || (size_t)len >= sizeof(head))
		die_msg("SET command too long");
	send_all(fd, head, (size_t)len);
	send_all(fd, value, value_bytes);
	send_all(fd, "\r\n", 2);
}

static void run_prepare(int fd, const struct options *opts, const char *value)
{
	unsigned long inflight = 0;

	for (unsigned long key = 0; key < opts->total_keys; key++) {
		send_set(fd, key, value, opts->value_bytes);
		inflight++;
		if (inflight == opts->pipeline) {
			for (unsigned long i = 0; i < inflight; i++)
				read_reply(fd);
			inflight = 0;
		}
	}
	for (unsigned long i = 0; i < inflight; i++)
		read_reply(fd);
}

static uint64_t run_hotset(int fd, const struct options *opts, const char *value)
{
	uint64_t seed = 0x6d6f6e69746f72ULL ^ (uint64_t)getpid();
	uint64_t end_ns = nsec_now() + (uint64_t)opts->seconds * 1000000000ULL;
	uint64_t ops = 0;

	while (nsec_now() < end_ns) {
		unsigned long inflight = 0;

		for (; inflight < opts->pipeline && nsec_now() < end_ns; inflight++) {
			unsigned long key = xorshift64(&seed) % opts->hot_keys;

			if (opts->mode == MODE_GET_HOTSET)
				send_get(fd, key);
			else
				send_set(fd, key, value, opts->value_bytes);
		}
		for (unsigned long i = 0; i < inflight; i++)
			read_reply(fd);
		ops += inflight;
	}
	return ops;
}

int main(int argc, char **argv)
{
	struct options opts;
	char *value;
	int fd;
	uint64_t start_ns;
	uint64_t ops = 0;

	parse_args(argc, argv, &opts);
	value = malloc(opts.value_bytes);
	if (!value)
		die("malloc");
	for (unsigned long i = 0; i < opts.value_bytes; i++)
		value[i] = (char)('a' + (i % 26));

	fd = redis_connect(&opts);
	printf("mode=%s host=%s port=%u total_keys=%lu hot_keys=%lu "
	       "value_bytes=%lu pipeline=%lu seconds=%u pid=%d\n",
	       mode_name(opts.mode), opts.host, opts.port, opts.total_keys,
	       opts.hot_keys, opts.value_bytes, opts.pipeline, opts.seconds,
	       getpid());
	fflush(stdout);

	start_ns = nsec_now();
	if (opts.mode == MODE_PREPARE)
		run_prepare(fd, &opts, value);
	else
		ops = run_hotset(fd, &opts, value);
	printf("done mode=%s ops=%" PRIu64 " elapsed=%.3fs\n",
	       mode_name(opts.mode), opts.mode == MODE_PREPARE ?
	       opts.total_keys : ops,
	       (double)(nsec_now() - start_ns) / 1000000000.0);

	close(fd);
	free(value);
	return EXIT_SUCCESS;
}
