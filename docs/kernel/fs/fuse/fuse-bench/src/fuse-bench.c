#include "common.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fuse.h>
#include <linux/io_uring.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define FB_FILE_NODE 2
#define FB_FH 1
#define FB_MAX_WRITE (1024U * 1024U)
#define FB_READ_BUF (FB_MAX_WRITE + 65536U)
#define FB_URING_PAYLOAD FB_READ_BUF
#define FB_CQE_SIZE 32U
#define FB_SQE_SIZE 128U

enum fb_reply_mode {
	FB_REPLY_LEGACY,
	FB_REPLY_URING,
};

struct fb_options {
	const char *backing;
	const char *mountpoint;
	int direct_io;
	int writeback_cache;
	int debug;
	int uring;
};

struct fb_io_uring {
	int fd;
	struct io_uring_params params;
	void *sq_ring;
	void *cq_ring;
	void *sqes;
	size_t sq_ring_sz;
	size_t cq_ring_sz;
	size_t sqes_sz;
	int single_mmap;
	unsigned int *sq_head;
	unsigned int *sq_tail;
	unsigned int *sq_ring_mask;
	unsigned int *sq_ring_entries;
	unsigned int *sq_flags;
	unsigned int *sq_array;
	unsigned int *cq_head;
	unsigned int *cq_tail;
	unsigned int *cq_ring_mask;
	unsigned int *cq_ring_entries;
	struct io_uring_cqe *cqes;
};

struct fb_uring_entry {
	struct fuse_uring_req_header *header;
	char *payload;
	struct iovec iov[2];
	uint16_t qid;
};

struct fb_state {
	struct fb_options opts;
	int fuse_fd;
	int backing_fd;
	uint64_t backing_size;
	uid_t owner_uid;
	gid_t owner_gid;
	int mounted;
	volatile sig_atomic_t stop;
	enum fb_reply_mode reply_mode;
	struct fb_uring_entry *reply_entry;
	int uring_negotiated;
};

static struct fb_state *global_state;

static void usage(FILE *out)
{
	fprintf(out,
		"usage: fuse-bench.out --backing FILE --mountpoint DIR [options]\n"
		"\n"
		"options:\n"
		"  --uring          accepted for clarity; io_uring is mandatory\n"
		"  --direct-io       return FOPEN_DIRECT_IO from OPEN\n"
		"  --writeback-cache request FUSE_WRITEBACK_CACHE if kernel offers it\n"
		"  --debug           print each handled opcode\n"
		"  -h, --help        show this help\n");
}

static void log_debug(const struct fb_state *st, const char *fmt, ...)
{
	va_list ap;

	if (!st->opts.debug)
		return;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void on_signal(int signo)
{
	(void)signo;
	if (global_state)
		global_state->stop = 1;
}

static int parse_args(int argc, char **argv, struct fb_options *opts)
{
	memset(opts, 0, sizeof(*opts));
	opts->uring = 1;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--backing") == 0 && i + 1 < argc) {
			opts->backing = argv[++i];
		} else if (strcmp(argv[i], "--mountpoint") == 0 && i + 1 < argc) {
			opts->mountpoint = argv[++i];
		} else if (strcmp(argv[i], "--uring") == 0) {
			opts->uring = 1;
		} else if (strcmp(argv[i], "--direct-io") == 0) {
			opts->direct_io = 1;
		} else if (strcmp(argv[i], "--writeback-cache") == 0) {
			opts->writeback_cache = 1;
		} else if (strcmp(argv[i], "--debug") == 0) {
			opts->debug = 1;
		} else if (strcmp(argv[i], "-h") == 0 ||
			   strcmp(argv[i], "--help") == 0) {
			usage(stdout);
			exit(0);
		} else {
			fprintf(stderr, "unknown or incomplete option: %s\n", argv[i]);
			return -1;
		}
	}

	if (!opts->backing || !opts->mountpoint) {
		usage(stderr);
		return -1;
	}
	return 0;
}

static uint32_t env_id_or_current(const char *name, uint32_t fallback)
{
	const char *text = getenv(name);
	char *end = NULL;
	unsigned long value;

	if (!text || text[0] == '\0')
		return fallback;
	errno = 0;
	value = strtoul(text, &end, 10);
	if (errno != 0 || end == text || *end != '\0' || value > UINT32_MAX)
		return fallback;
	return (uint32_t)value;
}

static void fill_attr(const struct fb_state *st, uint64_t nodeid,
		      struct fuse_attr *attr)
{
	memset(attr, 0, sizeof(*attr));
	attr->ino = nodeid;
	attr->uid = st->owner_uid;
	attr->gid = st->owner_gid;
	attr->blksize = 4096;
	attr->atime = attr->mtime = attr->ctime = (uint64_t)time(NULL);

	if (nodeid == FUSE_ROOT_ID) {
		attr->mode = S_IFDIR | 0755;
		attr->nlink = 2;
		return;
	}

	attr->mode = S_IFREG | 0644;
	attr->nlink = 1;
	attr->size = st->backing_size;
	attr->blocks = (st->backing_size + 511) / 512;
}

static int full_write(int fd, const void *buf, size_t len)
{
	const char *p = buf;

	while (len > 0) {
		ssize_t n = write(fd, p, len);

		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		p += n;
		len -= (size_t)n;
	}
	return 0;
}

static int reply_payload(struct fb_state *st, uint64_t unique, const void *payload,
			 size_t payload_len)
{
	struct fuse_out_header out = {
		.len = (uint32_t)(sizeof(out) + payload_len),
		.error = 0,
		.unique = unique,
	};
	char *buf = malloc(out.len);
	int ret;

	if (st->reply_mode == FB_REPLY_URING) {
		struct fb_uring_entry *ent = st->reply_entry;

		if (!ent || payload_len > FB_URING_PAYLOAD)
			return -1;
		memcpy(ent->header->in_out, &out, sizeof(out));
		if (payload_len > 0)
			memcpy(ent->payload, payload, payload_len);
		ent->header->ring_ent_in_out.payload_sz = (uint32_t)payload_len;
		return 0;
	}

	if (!buf)
		return -1;
	memcpy(buf, &out, sizeof(out));
	if (payload_len > 0)
		memcpy(buf + sizeof(out), payload, payload_len);
	ret = full_write(st->fuse_fd, buf, out.len);
	free(buf);
	return ret;
}

static int reply_empty(struct fb_state *st, uint64_t unique)
{
	return reply_payload(st, unique, NULL, 0);
}

static int reply_error(struct fb_state *st, uint64_t unique, int err)
{
	struct fuse_out_header out = {
		.len = sizeof(out),
		.error = -err,
		.unique = unique,
	};

	if (st->reply_mode == FB_REPLY_URING) {
		struct fb_uring_entry *ent = st->reply_entry;

		if (!ent)
			return -1;
		memcpy(ent->header->in_out, &out, sizeof(out));
		ent->header->ring_ent_in_out.payload_sz = 0;
		return 0;
	}

	return full_write(st->fuse_fd, &out, sizeof(out));
}

static void print_caps(const char *label, uint64_t caps)
{
	char buf[256];

	fb_format_caps(caps, buf, sizeof(buf));
	fprintf(stderr, "%s: %s\n", label, buf);
}

static int handle_init(struct fb_state *st, const struct fuse_in_header *hdr,
		       const char *payload, size_t payload_len)
{
	const struct fuse_init_in *in;
	struct fuse_init_out out;
	uint64_t kernel_caps;
	uint64_t supported = FB_CAP_ASYNC_READ | FB_CAP_BIG_WRITES |
			     FB_CAP_ASYNC_DIO | FB_CAP_OVER_IO_URING;
	uint64_t enabled;

	if (payload_len < sizeof(*in))
		return reply_error(st, hdr->unique, EINVAL);

	in = (const struct fuse_init_in *)payload;
	kernel_caps = (uint64_t)in->flags | ((uint64_t)in->flags2 << 32);
	if ((kernel_caps & FB_CAP_OVER_IO_URING) == 0) {
		fprintf(stderr, "io_uring required but kernel did not offer FUSE_OVER_IO_URING\n");
		st->stop = 1;
		return reply_error(st, hdr->unique, EOPNOTSUPP);
	}

	if (st->opts.writeback_cache)
		supported |= FB_CAP_WRITEBACK_CACHE;
	enabled = kernel_caps & supported;

	memset(&out, 0, sizeof(out));
	out.major = FUSE_KERNEL_VERSION;
	out.minor = in->minor < FUSE_KERNEL_MINOR_VERSION ?
		    in->minor : FUSE_KERNEL_MINOR_VERSION;
	out.max_readahead = in->max_readahead;
	out.flags = (uint32_t)enabled;
	out.flags2 = (uint32_t)(enabled >> 32);
	out.max_background = 64;
	out.congestion_threshold = 48;
	out.max_write = FB_MAX_WRITE;
	out.time_gran = 1;
	out.max_pages = FB_MAX_WRITE / 4096;

	fprintf(stderr, "FUSE protocol: kernel=%u.%u daemon=%u.%u\n",
		in->major, in->minor, out.major, out.minor);
	print_caps("kernel_caps", kernel_caps);
	print_caps("enabled_caps", enabled);
	if (kernel_caps & FB_CAP_PASSTHROUGH)
		fprintf(stderr, "passthrough: kernel offers it, v1 reports only\n");
	fprintf(stderr, "io_uring: required and enabled after FUSE_INIT\n");
	st->uring_negotiated = 1;

	return reply_payload(st, hdr->unique, &out, sizeof(out));
}

static int handle_lookup(struct fb_state *st, const struct fuse_in_header *hdr,
			 const char *payload, size_t payload_len)
{
	struct fuse_entry_out out;

	if (hdr->nodeid != FUSE_ROOT_ID)
		return reply_error(st, hdr->unique, ENOENT);
	if (payload_len != strlen("file") + 1 ||
	    strcmp(payload, "file") != 0)
		return reply_error(st, hdr->unique, ENOENT);

	memset(&out, 0, sizeof(out));
	out.nodeid = FB_FILE_NODE;
	out.generation = 1;
	out.entry_valid = 1;
	out.attr_valid = 1;
	fill_attr(st, FB_FILE_NODE, &out.attr);
	return reply_payload(st, hdr->unique, &out, sizeof(out));
}

static int handle_getattr(struct fb_state *st, const struct fuse_in_header *hdr)
{
	struct fuse_attr_out out;

	if (hdr->nodeid != FUSE_ROOT_ID && hdr->nodeid != FB_FILE_NODE)
		return reply_error(st, hdr->unique, ENOENT);

	memset(&out, 0, sizeof(out));
	out.attr_valid = 1;
	fill_attr(st, hdr->nodeid, &out.attr);
	return reply_payload(st, hdr->unique, &out, sizeof(out));
}

static int handle_setattr(struct fb_state *st, const struct fuse_in_header *hdr,
			  const char *payload, size_t payload_len)
{
	const struct fuse_setattr_in *in;

	if (hdr->nodeid != FB_FILE_NODE)
		return reply_error(st, hdr->unique, EINVAL);
	if (payload_len < sizeof(*in))
		return reply_error(st, hdr->unique, EINVAL);

	in = (const struct fuse_setattr_in *)payload;
	if ((in->valid & FATTR_SIZE) != 0) {
		if (ftruncate(st->backing_fd, (off_t)in->size) != 0)
			return reply_error(st, hdr->unique, errno);
		st->backing_size = in->size;
	}
	return handle_getattr(st, hdr);
}

static int handle_open(struct fb_state *st, const struct fuse_in_header *hdr)
{
	struct fuse_open_out out;

	if (hdr->nodeid != FB_FILE_NODE && hdr->nodeid != FUSE_ROOT_ID)
		return reply_error(st, hdr->unique, ENOENT);

	memset(&out, 0, sizeof(out));
	out.fh = FB_FH;
	out.backing_id = -1;
	if (st->opts.direct_io)
		out.open_flags = FOPEN_DIRECT_IO | FOPEN_PARALLEL_DIRECT_WRITES;
	else
		out.open_flags = FOPEN_KEEP_CACHE;
	return reply_payload(st, hdr->unique, &out, sizeof(out));
}

static size_t add_dirent(char *buf, size_t size, const char *name, uint64_t ino,
			 uint64_t off, uint32_t type)
{
	size_t namelen = strlen(name);
	size_t entsize = FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET + namelen);
	struct fuse_dirent *de;

	if (entsize > size)
		return 0;
	de = (struct fuse_dirent *)buf;
	memset(de, 0, entsize);
	de->ino = ino;
	de->off = off;
	de->namelen = (uint32_t)namelen;
	de->type = type;
	memcpy(de->name, name, namelen);
	return entsize;
}

static int handle_readdir(struct fb_state *st, const struct fuse_in_header *hdr,
			  const char *payload, size_t payload_len)
{
	const struct fuse_read_in *in;
	char *out;
	size_t used = 0;
	struct {
		const char *name;
		uint64_t ino;
		uint64_t off;
		uint32_t type;
	} entries[] = {
		{ ".", FUSE_ROOT_ID, 1, DT_DIR },
		{ "..", FUSE_ROOT_ID, 2, DT_DIR },
		{ "file", FB_FILE_NODE, 3, DT_REG },
	};

	if (hdr->nodeid != FUSE_ROOT_ID)
		return reply_error(st, hdr->unique, ENOTDIR);
	if (payload_len < sizeof(*in))
		return reply_error(st, hdr->unique, EINVAL);

	in = (const struct fuse_read_in *)payload;
	out = calloc(1, in->size);
	if (!out)
		return reply_error(st, hdr->unique, ENOMEM);

	for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
		size_t added;

		if (in->offset >= entries[i].off)
			continue;
		added = add_dirent(out + used, in->size - used,
				   entries[i].name, entries[i].ino,
				   entries[i].off, entries[i].type);
		if (added == 0)
			break;
		used += added;
	}

	int ret = reply_payload(st, hdr->unique, out, used);
	free(out);
	return ret;
}

static int handle_read(struct fb_state *st, const struct fuse_in_header *hdr,
		       const char *payload, size_t payload_len)
{
	const struct fuse_read_in *in;
	char *out;
	ssize_t n;

	if (hdr->nodeid != FB_FILE_NODE)
		return reply_error(st, hdr->unique, EISDIR);
	if (payload_len < sizeof(*in))
		return reply_error(st, hdr->unique, EINVAL);

	in = (const struct fuse_read_in *)payload;
	out = malloc(in->size);
	if (!out)
		return reply_error(st, hdr->unique, ENOMEM);

	n = pread(st->backing_fd, out, in->size, (off_t)in->offset);
	if (n < 0) {
		int err = errno;

		free(out);
		return reply_error(st, hdr->unique, err);
	}

	int ret = reply_payload(st, hdr->unique, out, (size_t)n);
	free(out);
	return ret;
}

static int handle_write(struct fb_state *st, const struct fuse_in_header *hdr,
			const char *payload, size_t payload_len)
{
	const struct fuse_write_in *in;
	const char *data;
	struct fuse_write_out out;
	ssize_t n;

	if (hdr->nodeid != FB_FILE_NODE)
		return reply_error(st, hdr->unique, EISDIR);
	if (payload_len < sizeof(*in))
		return reply_error(st, hdr->unique, EINVAL);

	in = (const struct fuse_write_in *)payload;
	if (payload_len - sizeof(*in) < in->size)
		return reply_error(st, hdr->unique, EINVAL);

	data = payload + sizeof(*in);
	n = pwrite(st->backing_fd, data, in->size, (off_t)in->offset);
	if (n < 0)
		return reply_error(st, hdr->unique, errno);
	if (in->offset + (uint64_t)n > st->backing_size)
		st->backing_size = in->offset + (uint64_t)n;

	memset(&out, 0, sizeof(out));
	out.size = (uint32_t)n;
	return reply_payload(st, hdr->unique, &out, sizeof(out));
}

static int handle_statfs(struct fb_state *st, const struct fuse_in_header *hdr)
{
	struct fuse_statfs_out out;

	memset(&out, 0, sizeof(out));
	out.st.bsize = 4096;
	out.st.frsize = 4096;
	out.st.namelen = 255;
	out.st.blocks = (st->backing_size + 4095) / 4096;
	out.st.bfree = out.st.bavail = 0;
	out.st.files = 2;
	return reply_payload(st, hdr->unique, &out, sizeof(out));
}

static int dispatch_request(struct fb_state *st, const char *buf, size_t len)
{
	const struct fuse_in_header *hdr = (const struct fuse_in_header *)buf;
	const char *payload = buf + sizeof(*hdr);
	size_t payload_len = len - sizeof(*hdr);

	if (len < sizeof(*hdr))
		return -1;
	log_debug(st, "opcode=%u unique=%llu nodeid=%llu len=%u\n",
		  hdr->opcode, (unsigned long long)hdr->unique,
		  (unsigned long long)hdr->nodeid, hdr->len);

	switch (hdr->opcode) {
	case FUSE_LOOKUP:
		return handle_lookup(st, hdr, payload, payload_len);
	case FUSE_FORGET:
		return 0;
	case FUSE_GETATTR:
		return handle_getattr(st, hdr);
	case FUSE_SETATTR:
		return handle_setattr(st, hdr, payload, payload_len);
	case FUSE_OPEN:
	case FUSE_OPENDIR:
		return handle_open(st, hdr);
	case FUSE_READ:
		return handle_read(st, hdr, payload, payload_len);
	case FUSE_WRITE:
		return handle_write(st, hdr, payload, payload_len);
	case FUSE_READDIR:
		return handle_readdir(st, hdr, payload, payload_len);
	case FUSE_STATFS:
		return handle_statfs(st, hdr);
	case FUSE_RELEASE:
	case FUSE_RELEASEDIR:
	case FUSE_FLUSH:
	case FUSE_FSYNC:
	case FUSE_FSYNCDIR:
	case FUSE_ACCESS:
		return reply_empty(st, hdr->unique);
	case FUSE_INIT:
		return handle_init(st, hdr, payload, payload_len);
	case FUSE_DESTROY:
		st->stop = 1;
		return reply_empty(st, hdr->unique);
	default:
		log_debug(st, "unsupported opcode=%u\n", hdr->opcode);
		return reply_error(st, hdr->unique, ENOSYS);
	}
}

static int receive_fuse_fd(int sock)
{
	struct msghdr msg;
	struct iovec iov;
	char byte;
	union {
		struct cmsghdr hdr;
		char buf[CMSG_SPACE(sizeof(int))];
	} control;
	struct cmsghdr *cmsg;
	ssize_t n;

	memset(&msg, 0, sizeof(msg));
	memset(&control, 0, sizeof(control));
	iov.iov_base = &byte;
	iov.iov_len = sizeof(byte);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control.buf;
	msg.msg_controllen = sizeof(control.buf);

	do {
		n = recvmsg(sock, &msg, 0);
	} while (n < 0 && errno == EINTR);
	if (n <= 0) {
		perror("recvmsg fusermount3");
		return -1;
	}

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		int fd;

		if (cmsg->cmsg_level != SOL_SOCKET ||
		    cmsg->cmsg_type != SCM_RIGHTS ||
		    cmsg->cmsg_len < CMSG_LEN(sizeof(int)))
			continue;
		memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
		return fd;
	}

	fprintf(stderr, "fusermount3 did not pass a fuse fd\n");
	return -1;
}

static int mount_fuse(struct fb_state *st)
{
	char opts[256];
	char commfd[32];
	int sv[2];
	pid_t pid;
	int status;

	if (fb_build_fuse_mount_opts(st->opts.backing, opts, sizeof(opts)) != 0) {
		fprintf(stderr, "invalid fsname for fusermount3 options: %s\n",
			st->opts.backing);
		return -1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) != 0) {
		perror("socketpair");
		return -1;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		close(sv[0]);
		close(sv[1]);
		return -1;
	}

	if (pid == 0) {
		close(sv[0]);
		if (fcntl(sv[1], F_SETFD, 0) != 0)
			_exit(127);
		snprintf(commfd, sizeof(commfd), "%d", sv[1]);
		setenv("_FUSE_COMMFD", commfd, 1);
		execlp("fusermount3", "fusermount3", "-o", opts,
		       st->opts.mountpoint, (char *)NULL);
		perror("exec fusermount3");
		_exit(127);
	}

	close(sv[1]);
	st->fuse_fd = receive_fuse_fd(sv[0]);
	close(sv[0]);

	if (waitpid(pid, &status, 0) < 0) {
		perror("waitpid fusermount3");
		if (st->fuse_fd >= 0) {
			close(st->fuse_fd);
			st->fuse_fd = -1;
		}
		return -1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || st->fuse_fd < 0) {
		fprintf(stderr, "fusermount3 failed for %s\n", st->opts.mountpoint);
		if (st->fuse_fd >= 0) {
			close(st->fuse_fd);
			st->fuse_fd = -1;
		}
		return -1;
	}

	st->mounted = 1;
	return 0;
}

static int unmount_fuse(const char *mountpoint)
{
	pid_t pid = fork();
	int status;

	if (pid < 0) {
		perror("fork fusermount3 -u");
		return -1;
	}
	if (pid == 0) {
		execlp("fusermount3", "fusermount3", "-u", "-z", "-q",
		       mountpoint, (char *)NULL);
		perror("exec fusermount3 -u");
		_exit(127);
	}

	if (waitpid(pid, &status, 0) < 0) {
		perror("waitpid fusermount3 -u");
		return -1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		fprintf(stderr, "fusermount3 -u failed for %s\n", mountpoint);
		return -1;
	}
	return 0;
}

static struct io_uring_sqe *fb_sqe_at(struct fb_io_uring *ring,
				      unsigned int index)
{
	return (struct io_uring_sqe *)((char *)ring->sqes +
				       (size_t)index * FB_SQE_SIZE);
}

static struct io_uring_cqe *fb_cqe_at(struct fb_io_uring *ring,
				      unsigned int index)
{
	return (struct io_uring_cqe *)((char *)ring->cqes +
				       (size_t)index * FB_CQE_SIZE);
}

static int fb_io_uring_setup(struct fb_io_uring *ring, unsigned int entries)
{
	struct io_uring_params *p = &ring->params;
	size_t sq_ring_sz;
	size_t cq_ring_sz;

	memset(ring, 0, sizeof(*ring));
	ring->fd = -1;
	memset(p, 0, sizeof(*p));
	p->flags = IORING_SETUP_SQE128 | IORING_SETUP_CQE32 |
		   IORING_SETUP_CQSIZE;
	p->cq_entries = entries * 2;

	ring->fd = (int)syscall(SYS_io_uring_setup, entries, p);
	if (ring->fd < 0) {
		perror("io_uring_setup");
		return -1;
	}

	sq_ring_sz = p->sq_off.array + p->sq_entries * sizeof(unsigned int);
	cq_ring_sz = p->cq_off.cqes + p->cq_entries * FB_CQE_SIZE;
	if (p->features & IORING_FEAT_SINGLE_MMAP) {
		if (cq_ring_sz > sq_ring_sz)
			sq_ring_sz = cq_ring_sz;
		ring->single_mmap = 1;
	}

	ring->sq_ring = mmap(NULL, sq_ring_sz, PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_POPULATE, ring->fd,
			     IORING_OFF_SQ_RING);
	if (ring->sq_ring == MAP_FAILED) {
		perror("mmap sq ring");
		goto err;
	}
	ring->sq_ring_sz = sq_ring_sz;

	if (ring->single_mmap) {
		ring->cq_ring = ring->sq_ring;
		ring->cq_ring_sz = ring->sq_ring_sz;
	} else {
		ring->cq_ring = mmap(NULL, cq_ring_sz, PROT_READ | PROT_WRITE,
				     MAP_SHARED | MAP_POPULATE, ring->fd,
				     IORING_OFF_CQ_RING);
		if (ring->cq_ring == MAP_FAILED) {
			perror("mmap cq ring");
			goto err;
		}
		ring->cq_ring_sz = cq_ring_sz;
	}

	ring->sqes_sz = (size_t)p->sq_entries * FB_SQE_SIZE;
	ring->sqes = mmap(NULL, ring->sqes_sz, PROT_READ | PROT_WRITE,
			  MAP_SHARED | MAP_POPULATE, ring->fd,
			  IORING_OFF_SQES);
	if (ring->sqes == MAP_FAILED) {
		perror("mmap sqes");
		goto err;
	}

	ring->sq_head = (unsigned int *)((char *)ring->sq_ring + p->sq_off.head);
	ring->sq_tail = (unsigned int *)((char *)ring->sq_ring + p->sq_off.tail);
	ring->sq_ring_mask = (unsigned int *)((char *)ring->sq_ring + p->sq_off.ring_mask);
	ring->sq_ring_entries = (unsigned int *)((char *)ring->sq_ring + p->sq_off.ring_entries);
	ring->sq_flags = (unsigned int *)((char *)ring->sq_ring + p->sq_off.flags);
	ring->sq_array = (unsigned int *)((char *)ring->sq_ring + p->sq_off.array);
	ring->cq_head = (unsigned int *)((char *)ring->cq_ring + p->cq_off.head);
	ring->cq_tail = (unsigned int *)((char *)ring->cq_ring + p->cq_off.tail);
	ring->cq_ring_mask = (unsigned int *)((char *)ring->cq_ring + p->cq_off.ring_mask);
	ring->cq_ring_entries = (unsigned int *)((char *)ring->cq_ring + p->cq_off.ring_entries);
	ring->cqes = (struct io_uring_cqe *)((char *)ring->cq_ring + p->cq_off.cqes);
	return 0;

err:
	if (ring->sqes && ring->sqes != MAP_FAILED)
		munmap(ring->sqes, ring->sqes_sz);
	if (!ring->single_mmap && ring->cq_ring && ring->cq_ring != MAP_FAILED)
		munmap(ring->cq_ring, ring->cq_ring_sz);
	if (ring->sq_ring && ring->sq_ring != MAP_FAILED)
		munmap(ring->sq_ring, ring->sq_ring_sz);
	if (ring->fd >= 0)
		close(ring->fd);
	ring->fd = -1;
	return -1;
}

static void fb_io_uring_destroy(struct fb_io_uring *ring)
{
	if (ring->sqes && ring->sqes != MAP_FAILED)
		munmap(ring->sqes, ring->sqes_sz);
	if (!ring->single_mmap && ring->cq_ring && ring->cq_ring != MAP_FAILED)
		munmap(ring->cq_ring, ring->cq_ring_sz);
	if (ring->sq_ring && ring->sq_ring != MAP_FAILED)
		munmap(ring->sq_ring, ring->sq_ring_sz);
	if (ring->fd >= 0)
		close(ring->fd);
}

static int fb_io_uring_submit_cmd(struct fb_io_uring *ring, int fuse_fd,
				  enum fuse_uring_cmd cmd_op,
				  struct fb_uring_entry *ent,
				  uint64_t commit_id)
{
	unsigned int tail = *ring->sq_tail;
	unsigned int head = *ring->sq_head;
	unsigned int index;
	struct io_uring_sqe *sqe;
	struct fuse_uring_cmd_req *cmd;
	int ret;

	if (tail - head >= *ring->sq_ring_entries) {
		fprintf(stderr, "io_uring SQ is full\n");
		return -1;
	}

	index = tail & *ring->sq_ring_mask;
	sqe = fb_sqe_at(ring, index);
	memset(sqe, 0, FB_SQE_SIZE);
	sqe->opcode = IORING_OP_URING_CMD;
	sqe->fd = fuse_fd;
	sqe->cmd_op = (uint32_t)cmd_op;
	sqe->addr = (uint64_t)(uintptr_t)ent->iov;
	sqe->len = 2;
	sqe->user_data = (uint64_t)(uintptr_t)ent;
	cmd = (struct fuse_uring_cmd_req *)sqe->cmd;
	memset(cmd, 0, sizeof(*cmd));
	cmd->qid = ent->qid;
	cmd->commit_id = commit_id;

	ring->sq_array[index] = index;
	__atomic_thread_fence(__ATOMIC_RELEASE);
	*ring->sq_tail = tail + 1;

	ret = (int)syscall(SYS_io_uring_enter, ring->fd, 1, 0, 0, NULL, 0);
	if (ret < 0) {
		perror("io_uring_enter submit");
		return -1;
	}
	if (ret != 1) {
		fprintf(stderr, "io_uring_enter submitted %d entries, expected 1\n",
			ret);
		return -1;
	}
	return 0;
}

static struct io_uring_cqe *fb_io_uring_wait_cqe(struct fb_io_uring *ring,
						 const struct fb_state *st)
{
	unsigned int head;
	unsigned int tail;

	for (;;) {
		head = __atomic_load_n(ring->cq_head, __ATOMIC_ACQUIRE);
		tail = __atomic_load_n(ring->cq_tail, __ATOMIC_ACQUIRE);
		if (head != tail)
			return fb_cqe_at(ring, head & *ring->cq_ring_mask);
		if (syscall(SYS_io_uring_enter, ring->fd, 0, 1,
			    IORING_ENTER_GETEVENTS, NULL, 0) < 0) {
			if (errno == EINTR && !st->stop)
				continue;
			if (errno == EINTR && st->stop)
				return NULL;
			perror("io_uring_enter wait");
			return NULL;
		}
	}
}

static void fb_io_uring_cqe_seen(struct fb_io_uring *ring)
{
	unsigned int head = *ring->cq_head;

	__atomic_store_n(ring->cq_head, head + 1, __ATOMIC_RELEASE);
}

static size_t uring_op_header_size(uint32_t opcode)
{
	switch (opcode) {
	case FUSE_GETATTR:
		return sizeof(struct fuse_getattr_in);
	case FUSE_SETATTR:
		return sizeof(struct fuse_setattr_in);
	case FUSE_OPEN:
	case FUSE_OPENDIR:
		return sizeof(struct fuse_open_in);
	case FUSE_READ:
	case FUSE_READDIR:
		return sizeof(struct fuse_read_in);
	case FUSE_WRITE:
		return sizeof(struct fuse_write_in);
	case FUSE_RELEASE:
	case FUSE_RELEASEDIR:
		return sizeof(struct fuse_release_in);
	case FUSE_FLUSH:
		return sizeof(struct fuse_flush_in);
	case FUSE_FSYNC:
	case FUSE_FSYNCDIR:
		return sizeof(struct fuse_fsync_in);
	case FUSE_ACCESS:
		return sizeof(struct fuse_access_in);
	default:
		return 0;
	}
}

static int dispatch_uring_request(struct fb_state *st,
				  struct fb_uring_entry *ent)
{
	const struct fuse_in_header *hdr =
		(const struct fuse_in_header *)ent->header->in_out;
	size_t op_size = uring_op_header_size(hdr->opcode);
	size_t payload_size = ent->header->ring_ent_in_out.payload_sz;
	size_t len = sizeof(*hdr) + op_size + payload_size;
	char *buf;
	int ret;

	if (payload_size > FB_URING_PAYLOAD || len < sizeof(*hdr))
		return -1;

	buf = malloc(len);
	if (!buf)
		return -1;
	memcpy(buf, hdr, sizeof(*hdr));
	if (op_size > 0)
		memcpy(buf + sizeof(*hdr), ent->header->op_in, op_size);
	if (payload_size > 0)
		memcpy(buf + sizeof(*hdr) + op_size, ent->payload, payload_size);

	st->reply_mode = FB_REPLY_URING;
	st->reply_entry = ent;
	ret = dispatch_request(st, buf, len);
	st->reply_entry = NULL;
	st->reply_mode = FB_REPLY_LEGACY;
	free(buf);
	return ret;
}

static int run_uring_loop(struct fb_state *st)
{
	struct fb_io_uring ring;
	struct fb_uring_entry *entries = NULL;
	unsigned int nqueues = fb_possible_cpu_count();
	int ret = -1;

	entries = calloc(nqueues, sizeof(*entries));
	if (!entries)
		return -1;
	if (fb_io_uring_setup(&ring, nqueues) != 0)
		goto out_entries;

	for (unsigned int i = 0; i < nqueues; i++) {
		entries[i].qid = (uint16_t)i;
		entries[i].header = calloc(1, sizeof(*entries[i].header));
		entries[i].payload = calloc(1, FB_URING_PAYLOAD);
		if (!entries[i].header || !entries[i].payload) {
			fprintf(stderr, "failed to allocate uring entry %u\n", i);
			goto out_ring;
		}
		entries[i].iov[0].iov_base = entries[i].header;
		entries[i].iov[0].iov_len = sizeof(*entries[i].header);
		entries[i].iov[1].iov_base = entries[i].payload;
		entries[i].iov[1].iov_len = FB_URING_PAYLOAD;
		if (fb_io_uring_submit_cmd(&ring, st->fuse_fd,
					    FUSE_IO_URING_CMD_REGISTER,
					    &entries[i], 0) != 0)
			goto out_ring;
	}

	fprintf(stderr, "io_uring: registered %u FUSE queues\n", nqueues);

	while (!st->stop) {
		struct io_uring_cqe *cqe = fb_io_uring_wait_cqe(&ring, st);
		struct fb_uring_entry *ent;
		uint64_t commit_id;
		int cqe_res;

		if (!cqe) {
			if (st->stop)
				break;
			goto out_ring;
		}
		ent = (struct fb_uring_entry *)(uintptr_t)cqe->user_data;
		cqe_res = cqe->res;
		fb_io_uring_cqe_seen(&ring);
		if (cqe_res < 0) {
			fprintf(stderr, "FUSE uring command failed: %s\n",
				strerror(-cqe_res));
			goto out_ring;
		}

		commit_id = ent->header->ring_ent_in_out.commit_id;
		if (dispatch_uring_request(st, ent) != 0)
			goto out_ring;
		if (fb_io_uring_submit_cmd(&ring, st->fuse_fd,
					    FUSE_IO_URING_CMD_COMMIT_AND_FETCH,
					    ent, commit_id) != 0)
			goto out_ring;
		if (st->stop)
			break;
	}

	ret = 0;

out_ring:
	fb_io_uring_destroy(&ring);
out_entries:
	for (unsigned int i = 0; i < nqueues; i++) {
		free(entries[i].header);
		free(entries[i].payload);
	}
	free(entries);
	return ret;
}

static int run_loop(struct fb_state *st)
{
	char *buf = malloc(FB_READ_BUF);

	if (!buf)
		return -1;

	while (!st->stop) {
		ssize_t n = read(st->fuse_fd, buf, FB_READ_BUF);
		const struct fuse_in_header *hdr;

		if (n < 0) {
			if (errno == EINTR)
				continue;
			perror("read /dev/fuse");
			free(buf);
			return -1;
		}
		if (n == 0)
			break;
		if ((size_t)n < sizeof(struct fuse_in_header))
			continue;
		hdr = (const struct fuse_in_header *)buf;
		dispatch_request(st, buf, (size_t)n);
		if (hdr->opcode == FUSE_INIT) {
			free(buf);
			if (!st->uring_negotiated)
				return -1;
			return run_uring_loop(st);
		}
	}

	free(buf);
	return 0;
}

int main(int argc, char **argv)
{
	struct fb_state st;
	struct stat sb;
	struct sigaction sa;
	int ret = 1;

	memset(&st, 0, sizeof(st));
	st.fuse_fd = -1;
	st.backing_fd = -1;
	if (parse_args(argc, argv, &st.opts) != 0)
		return 2;
	st.owner_uid = env_id_or_current("SUDO_UID", getuid());
	st.owner_gid = env_id_or_current("SUDO_GID", getgid());

	st.backing_fd = open(st.opts.backing, O_RDWR | O_CLOEXEC);
	if (st.backing_fd < 0 && errno == ENOENT)
		st.backing_fd = open(st.opts.backing,
				     O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC,
				     0644);
	if (st.backing_fd < 0) {
		perror("open backing");
		return 1;
	}
	if (fstat(st.backing_fd, &sb) != 0) {
		perror("fstat backing");
		goto out;
	}
	st.backing_size = (uint64_t)sb.st_size;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	global_state = &st;

	if (mount_fuse(&st) != 0)
		goto out;

	fprintf(stderr, "mounted %s on %s as /file\n",
		st.opts.backing, st.opts.mountpoint);
	ret = run_loop(&st);

out:
	if (st.mounted && unmount_fuse(st.opts.mountpoint) != 0)
		fprintf(stderr, "hint: fusermount3 -u -z %s\n", st.opts.mountpoint);
	if (st.fuse_fd >= 0)
		close(st.fuse_fd);
	if (st.backing_fd >= 0)
		close(st.backing_fd);
	return ret;
}
